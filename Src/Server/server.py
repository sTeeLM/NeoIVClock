#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import sys
import json
import argparse
import configparser
import logging
import signal
import secrets
from typing import Dict, Any
from pydantic import BaseModel
from fastapi import FastAPI, HTTPException, Request, Depends, status
from fastapi.security import HTTPBasic, HTTPBasicCredentials
import uvicorn
import daemon

# ==============================================================================
# 1. Tree Structure Data Model Definition
# ==============================================================================
class PMS5003STData(BaseModel):
    pm_10: int; pm_25: int; pm_100: int
    pm_10a: int; pm_25a: int; pm_100a: int
    pm_03cnt: int; pm_05cnt: int; pm_10cnt: int; pm_25cnt: int; pm_50cnt: int; pm_100cnt: int
    form: float; temp: float; mol: float

class TPM300Data(BaseModel):
    tvoc: float

class BMP280Data(BaseModel):
    temp: float; press: float

class AHT20Data(BaseModel):
    temp: float; mol: float

class MultiSensorPayload(BaseModel):
    pms5003st_data: PMS5003STData
    tpm300_data: TPM300Data
    bmp280_data: BMP280Data
    aht20_data: AHT20Data
    time_stamp: int

# Global configuration dictionary
CONFIG: Dict[str, Any] = {}

# ==============================================================================
# 2. Configuration and Arguments Parser (Command Line > Config File > Defaults)
# ==============================================================================
def parse_arguments_and_config():
    parser = argparse.ArgumentParser(description="ivclock data receiver service")
    parser.add_argument("-c", "--config", type=str, default="/etc/ivclock/ivclock.conf", help="Configuration file path")
    parser.add_argument("-P", "--protocol", type=str, choices=["http", "https"], help="Service protocol (http/https)")
    parser.add_argument("-H", "--host", type=str, help="Service listen address (e.g., 0.0.0.0 or 127.0.0.1)")
    parser.add_argument("-p", "--port", type=int, help="Service listen port")
    parser.add_argument("-d", "--daemon", action="store_true", default=argparse.SUPPRESS, help="Run in daemon background mode")
    parser.add_argument("-f", "--log-file", type=str, help="Log file path")
    parser.add_argument("-l", "--log-level", type=str, choices=["debug", "info", "warning", "error"], help="Log level")
    parser.add_argument("-u", "--auth-user", type=str, help="HTTP Basic Auth Username")
    parser.add_argument("-s", "--auth-pass", type=str, help="HTTP Basic Auth Password")
    parser.add_argument("-U", "--influx-url", type=str, help="InfluxDB connection URL")
    parser.add_argument("-T", "--influx-token", type=str, help="InfluxDB authentication Token")
    parser.add_argument("-O", "--influx-org", type=str, help="InfluxDB organization name")
    parser.add_argument("-B", "--influx-bucket", type=str, help="InfluxDB bucket name")
    
    args = parser.parse_args()
    cmd_line_values = vars(args)

    defaults = {
        "protocol": "http", "host": "0.0.0.0", "port": 8989, "daemon": False,
        "log_file": "/var/log/ivclock/ivclock.log", "log_level": "info",
        "auth_user": "admin", "auth_pass": "password",
        "influx_url": "http://127.0.0.1:8086", "influx_token": "",
        "influx_org": "my_org", "influx_bucket": "esp32_data"
    }

    config_file_values = {}
    config_path = cmd_line_values.get("config", "/etc/ivclock/ivclock.conf")
    if os.path.exists(config_path):
        cfg = configparser.ConfigParser()
        cfg.read(config_path)
        if "server" in cfg:
            if "protocol" in cfg["server"]: config_file_values["protocol"] = cfg["server"].get("protocol")
            if "host" in cfg["server"]: config_file_values["host"] = cfg["server"].get("host")
            if "port" in cfg["server"]: config_file_values["port"] = cfg["server"].getint("port")
            if "daemon" in cfg["server"]: config_file_values["daemon"] = cfg["server"].getboolean("daemon")
            if "log_file" in cfg["server"]: config_file_values["log_file"] = cfg["server"].get("log_file")
            if "log_level" in cfg["server"]: config_file_values["log_level"] = cfg["server"].get("log_level")
            if "auth_user" in cfg["server"]: config_file_values["auth_user"] = cfg["server"].get("auth_user")
            if "auth_pass" in cfg["server"]: config_file_values["auth_pass"] = cfg["server"].get("auth_pass")
        if "influxdb" in cfg:
            if "url" in cfg["influxdb"]: config_file_values["influx_url"] = cfg["influxdb"].get("url")
            if "token" in cfg["influxdb"]: config_file_values["influx_token"] = cfg["influxdb"].get("token")
            if "org" in cfg["influxdb"]: config_file_values["influx_org"] = cfg["influxdb"].get("org")
            if "bucket" in cfg["influxdb"]: config_file_values["influx_bucket"] = cfg["influxdb"].get("bucket")

    for key in defaults:
        if key in cmd_line_values and cmd_line_values[key] is not None:
            CONFIG[key] = cmd_line_values[key]
        elif key in config_file_values and config_file_values[key] is not None:
            CONFIG[key] = config_file_values[key]
        else:
            CONFIG[key] = defaults[key]


# ==============================================================================
# 3. Millisecond-level Logging & HUP Rotate System
# ==============================================================================
file_handler: logging.FileHandler = None
formatter: logging.Formatter = None

def setup_logging():
    global file_handler, formatter
    level_map = {
        "debug": logging.DEBUG, "info": logging.INFO,
        "warning": logging.WARNING, "error": logging.ERROR
    }
    log_level = level_map.get(CONFIG["log_level"].lower(), logging.INFO)

    logger = logging.getLogger()
    logger.setLevel(log_level)

    # Format with explicit '%(asctime)s.%(msecs)03d' millisecond mapping
    formatter = logging.Formatter(
        fmt='%(asctime)s.%(msecs)03d - %(levelname)s - %(message)s',
        datefmt='%Y-%m-%d %H:%M:%S'
    )

    if CONFIG["daemon"]:
        log_dir = os.path.dirname(CONFIG["log_file"])
        if log_dir and not os.path.exists(log_dir):
            try:
                os.makedirs(log_dir, exist_ok=True)
            except Exception:
                pass
        file_handler = logging.FileHandler(CONFIG["log_file"])
        file_handler.setFormatter(formatter)
        logger.addHandler(file_handler)
    else:
        stream_handler = logging.StreamHandler(sys.stdout)
        stream_handler.setFormatter(formatter)
        logger.addHandler(stream_handler)

def handle_hup_signal(signum, frame):
    global file_handler, formatter
    if CONFIG["daemon"] and file_handler:
        logger = logging.getLogger()
        logger.info("SIGHUP received. Rotating log files...")
        logger.removeHandler(file_handler)
        file_handler.close()

        # Logrotate safely moves old log file, we reopen the primary target
        file_handler = logging.FileHandler(CONFIG["log_file"])
        file_handler.setFormatter(formatter)
        logger.addHandler(file_handler)
        logger.info("Log file has been successfully re-opened.")

def register_signal_handler():
    signal.signal(signal.SIGHUP, handle_hup_signal)


# ==============================================================================
# 4. FastAPI Web Service Core Application with HTTP Basic Auth Protection
# ==============================================================================
app = FastAPI()
security = HTTPBasic()

def verify_basic_auth(credentials: HTTPBasicCredentials = Depends(security)):
    current_username_bytes = credentials.username.encode("utf-8")
    correct_username_bytes = CONFIG["auth_user"].encode("utf-8")
    is_correct_username = secrets.compare_digest(current_username_bytes, correct_username_bytes)

    current_password_bytes = credentials.password.encode("utf-8")
    correct_password_bytes = CONFIG["auth_pass"].encode("utf-8")
    is_correct_password = secrets.compare_digest(current_password_bytes, correct_password_bytes)

    if not (is_correct_username and is_correct_password):
        logging.warning(f"Unauthorized access attempt rejected for username: {credentials.username}")
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="Incorrect HTTP Basic Auth username or password",
            headers={"WWW-Authenticate": "Basic"},
        )
    return credentials.username

@app.post("/report")
async def receive_sensor_data(payload: MultiSensorPayload, request: Request, username: str = Depends(verify_basic_auth)):
    raw_body = await request.body()
    logging.info(f"Authorized user [{username}] uploaded raw request payload: {raw_body.decode('utf-8')}")

    try:
        from influxdb_client import InfluxDBClient, Point, WritePrecision
        from influxdb_client.client.write_api import SYNCHRONOUS

        client = InfluxDBClient(url=CONFIG["influx_url"], token=CONFIG["influx_token"], org=CONFIG["influx_org"])
        write_api = client.write_api(write_options=SYNCHRONOUS)

        # Flatten tree nodes and map keys directly into InfluxDB column fields
        point = Point("air_quality_measurements") \
            .tag("device_id", "esp32_s3_payload") \
            .field("pms_pm10", payload.pms5003st_data.pm_10) \
            .field("pms_pm25", payload.pms5003st_data.pm_25) \
            .field("pms_pm100", payload.pms5003st_data.pm_100) \
            .field("pms_temp", payload.pms5003st_data.temp) \
            .field("pms_mol", payload.pms5003st_data.mol) \
            .field("tpm_tvoc", payload.tpm300_data.tvoc) \
            .field("bmp_temp", payload.bmp280_data.temp) \
            .field("bmp_press", payload.bmp280_data.press) \
            .field("aht_temp", payload.aht20_data.temp) \
            .field("aht_mol", payload.aht20_data.mol) \
            .time(payload.time_stamp, WritePrecision.S)

        write_api.write(bucket=CONFIG["influx_bucket"], org=CONFIG["influx_org"], record=point)
        client.close()

        logging.info("Successfully parsed and committed payload to InfluxDB.")
        return {"status": "success", "message": "Tree data written to InfluxDB"}
    except Exception as e:
        logging.error(f"Failed to process payload or write to InfluxDB: {str(e)}")
        raise HTTPException(status_code=500, detail=str(e))

def run_web_server():
    uvicorn.run(app, host=CONFIG["host"], port=CONFIG["port"], log_config=None)

# ==============================================================================
# 5. Execution Entrance
# ==============================================================================
if __name__ == "__main__":
    parse_arguments_and_config()
    setup_logging()

    # Mask sensitive secret details in startup logging
    log_config = CONFIG.copy()
    if log_config.get("influx_token"):
        token = log_config["influx_token"]
        log_config["influx_token"] = f"{token[:6]}...{token[-6:]}" if len(token) > 12 else "******"
    if log_config.get("auth_pass"):
        log_config["auth_pass"] = "******"

    logging.info("--- Core Configurations Multi-level Merging Results ---")
    logging.info(json.dumps(log_config, indent=4, ensure_ascii=False))
    logging.info("-------------------------------------------------------")

    if CONFIG["daemon"]:
        logging.info(f"Switching service to background Daemon mode. Log: {CONFIG['log_file']}")
        context = daemon.DaemonContext()
        if file_handler and file_handler.stream:
            context.files_preserve = [file_handler.stream]
        with context:
            register_signal_handler()
            run_web_server()
    else:
        logging.info(f"Starting service in foreground. URL: {CONFIG['protocol']}://{CONFIG['host']}:{CONFIG['port']}")
        register_signal_handler()
        run_web_server()

