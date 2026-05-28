#include "reporter.h"
#include "logger.h"
#include "config.h"

static const char * TAG = "REPORTER";

// 传感器数据上报间隔，0:30分钟，1:1小时
static uint8_t reporter_interval;

void reporter_init(void)
{
  NEO_LOGI(TAG, "init");
  reporter_interval = config_read_int("reporter_interval") % 2;
}

uint8_t reporter_get_interval(void)
{
  return reporter_interval;
}

uint8_t reporter_inc_interval(void)
{
  reporter_interval = (reporter_interval + 1) % 2;
  return reporter_interval;
}

void reporter_save_config(void)
{
  config_write_int("reporter_interval", reporter_interval);
}

bool reporter_report_data(const sensor_data_t * data)
{
  return false;
}