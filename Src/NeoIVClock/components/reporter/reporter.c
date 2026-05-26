#include "reporter.h"
#include "logger.h"
#include "config.h"

static const char * TAG = "REPORTER";

// 传感器数据上报间隔，0:10s，1:30s，2:1分钟，3:10分钟
static uint8_t reporter_sec;

void reporter_init(void)
{
  NEO_LOGI(TAG, "init");
  reporter_sec = config_read_int("reporter_sec") % 4;
}

uint16_t reporter_get_interval_sec(void)
{
  switch(reporter_sec) {
    case 0: return 10;
    case 1: return 30;
    case 2: return 60;
    case 3: return 600;
  }
  return 0;
}

uint8_t reporter_get_interval(void)
{
  return reporter_sec;
}

uint8_t reporter_inc_interval(void)
{
  reporter_sec = (reporter_sec + 1) % 4;
  return reporter_sec;
}

void reporter_save_config(void)
{
  config_write_int("reporter_sec", reporter_sec);
}

bool reporter_report_data(const sensor_data_t * data)
{
  return false;
}