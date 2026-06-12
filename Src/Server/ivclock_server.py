#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import sys
import json
import argparse
import configparser
import logging
import pidfile
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

class ENS160Data(BaseModel):
    tvoc: float; eco2: float; iaq: int;

class BMP280Data(BaseModel):
    temp: float; press: float

class AHT20Data(BaseModel):
    temp: float; mol: float

class MultiSensorPayload(BaseModel):
    pms5003st_data: PMS5003STData
    ens160_data: ENS160Data
    bmp280_data: BMP280Data
    aht20_data: AHT20Data
    time_stamp: int
    device_id: str

# Global configuration dictionary
CONFIG: Dict[str, Any] = {}

# ==============================================================================
# 2. Configuration and Arguments Parser with Short Form & Help Options
# ==============================================================================
def parse_arguments_and_config():
    # Enforce standard formatted help output using the native -h/--help mechanism
    parser = argparse.ArgumentParser(
        description="ivclock data receiver service",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )

    # Core Server Flags
    parser.add_argument("-c", "--config", type=str, default="/etc/ivclock/ivclock.conf", help="Configuration file path")
    parser.add_argument("-P", "--protocol", type=str, choices=["http", "https"], help="Service network protocol")
    parser.add_argument("-H", "--host", type=str, help="Service network listen bind address")
    parser.add_argument("-p", "--port", type=int, help="Service network listen bind port")
    parser.add_argument("-d", "--daemon", action="store_true", default=argparse.SUPPRESS, help="Run in POSIX background daemon mode")
    parser.add_argument("-f", "--log-file", type=str, help="Log storage target absolute file path")
    parser.add_argument("-l", "--log-level", type=str, choices=["debug", "info", "warning", "error"], help="Logging filter severity index")
    
    # PID File Flag with short form -m
    parser.add_argument("-m", "--pid-file", type=str, help="Path to save runtime process ID file (.pid)")

    # HTTP Basic Authentication Flags
    parser.add_argument("-u", "--auth-user", type=str, help="HTTP Basic Auth gate gateway verification username")
    parser.add_argument("-s", "--auth-pass", type=str, help="HTTP Basic Auth gate gateway verification password")

    # Native SSL/TLS Certificates Path Flags (Newly Added)
    parser.add_argument("-K", "--ssl-key", type=str, help="Path to target SSL private key file (.key)")
    parser.add_argument("-C", "--ssl-cert", type=str, help="Path to target SSL certificate bundle file (.crt/.pem)")

    # InfluxDB Pipeline Flags
    parser.add_argument("-U", "--influx-url", type=str, help="InfluxDB endpoint connection context URL")
    parser.add_argument("-T", "--influx-token", type=str, help="InfluxDB core security token key value")
    parser.add_argument("-O", "--influx-org", type=str, help="InfluxDB platform identity organization name")
    parser.add_argument("-B", "--influx-bucket", type=str, help="InfluxDB isolated target destination storage bucket name")

    args = parser.parse_args()
    cmd_line_values = vars(args)

    defaults = {
        "protocol": "http", "host": "0.0.0.0", "port": 8989, "daemon": False,
        "log_file": "/var/log/ivclock/ivclock.log", "log_level": "info", "pid_file": "/var/run/ivclock_server.pid",
        "auth_user": "admin", "auth_pass": "password", "ssl_key": "", "ssl_cert": "",
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
            if "pid_file" in cfg["server"]: config_file_values["pid_file"] = cfg["server"].get("pid_file")
            if "auth_user" in cfg["server"]: config_file_values["auth_user"] = cfg["server"].get("auth_user")
            if "auth_pass" in cfg["server"]: config_file_values["auth_pass"] = cfg["server"].get("auth_pass")
            if "ssl_key" in cfg["server"]: config_file_values["ssl_key"] = cfg["server"].get("ssl_key")
            if "ssl_cert" in cfg["server"]: config_file_values["ssl_cert"] = cfg["server"].get("ssl_cert")
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

        # Fully expanded mapping following the strict naming convention: component_data_field
        point = Point("air_quality_data") \
                .tag("device_id", payload.device_id) \
                .field("pms5003st_data_pm10", payload.pms5003st_data.pm_10) \
                .field("pms5003st_data_pm25", payload.pms5003st_data.pm_25) \
                .field("pms5003st_data_pm100", payload.pms5003st_data.pm_100) \
                .field("pms5003st_data_pm10a", payload.pms5003st_data.pm_10a) \
                .field("pms5003st_data_pm25a", payload.pms5003st_data.pm_25a) \
                .field("pms5003st_data_pm100a", payload.pms5003st_data.pm_100a) \
                .field("pms5003st_data_pm03cnt", payload.pms5003st_data.pm_03cnt) \
                .field("pms5003st_data_pm05cnt", payload.pms5003st_data.pm_05cnt) \
                .field("pms5003st_data_pm10cnt", payload.pms5003st_data.pm_10cnt) \
                .field("pms5003st_data_pm25cnt", payload.pms5003st_data.pm_25cnt) \
                .field("pms5003st_data_pm50cnt", payload.pms5003st_data.pm_50cnt) \
                .field("pms5003st_data_pm100cnt", payload.pms5003st_data.pm_100cnt) \
                .field("pms5003st_data_form", payload.pms5003st_data.form) \
                .field("pms5003st_data_temp", payload.pms5003st_data.temp) \
                .field("pms5003st_data_mol", payload.pms5003st_data.mol) \
                .field("ens160_data_tvoc", payload.ens160_data.tvoc) \
                .field("ens160_data_eco2", payload.ens160_data.eco2) \
                .field("ens160_data_iaq", payload.ens160_data.iaq) \
                .field("bmp280_data_temp", payload.bmp280_data.temp) \
                .field("bmp280_data_press", payload.bmp280_data.press) \
                .field("aht20_data_temp", payload.aht20_data.temp) \
                .field("aht20_data_mol", payload.aht20_data.mol) \
                .time(payload.time_stamp, WritePrecision.S)

        write_api.write(bucket=CONFIG["influx_bucket"], org=CONFIG["influx_org"], record=point)
        client.close()

        logging.info("Successfully parsed and committed payload to InfluxDB.")
        return {"status": "success", "message": "Tree data written to InfluxDB"}
    except Exception as e:
        logging.error(f"Failed to process payload or write to InfluxDB: {str(e)}")
        raise HTTPException(status_code=500, detail=str(e))

def run_web_server():
    # 💡 Core Dynamic Logic: Inject SSL context attributes when protocol is explicitly set to https
    if CONFIG["protocol"].lower() == "https" and CONFIG["ssl_key"] and CONFIG["ssl_cert"]:
        logging.info("Enabling Native SSL/TLS Pipeline encryption for Uvicorn.")
        uvicorn.run(
            app,
            host=CONFIG["host"],
            port=CONFIG["port"],
            log_config=None,
            ssl_keyfile=CONFIG["ssl_key"],
            ssl_certfile=CONFIG["ssl_cert"]
        )
    else:
        if CONFIG["protocol"].lower() == "https":
            logging.warning("Protocol is set to https but SSL credentials are missing. Falling back to HTTP.")
        uvicorn.run(app, host=CONFIG["host"], port=CONFIG["port"], log_config=None)

# ==============================================================================
# 5. Execution Entrance
# ==============================================================================
if __name__ == "__main__":
    parse_arguments_and_config()
    setup_logging()

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
        
        # Ensure the parent directory for the PID file exists before locking context
        pid_file_path = CONFIG["pid_file"]
        pid_dir = os.path.dirname(pid_file_path)
        if pid_dir and not os.path.exists(pid_dir):
            try:
                os.makedirs(pid_dir, exist_ok=True)
            except Exception:
                logging.error(f"Execution aborted. os.makedirs failed")
                pass
        context = daemon.DaemonContext(pidfile=pidfile.PidFile(pid_file_path))
        if file_handler and file_handler.stream:
            context.files_preserve = [file_handler.stream]
        with context:
            register_signal_handler()
            run_web_server()
    else:
        logging.info(f"Starting service in foreground. URL: {CONFIG['protocol']}://{CONFIG['host']}:{CONFIG['port']}")
        register_signal_handler()
        run_web_server()

