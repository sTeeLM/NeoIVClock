#include "reporter.h"
#include "logger.h"
#include "config.h"

static const char * TAG = "REPORTER";

static uint16_t reporter_sec;

void reporter_init(void)
{
  NEO_LOGI(TAG, "init");
  reporter_sec = config_read_int("reporter_sec");
}

uint16_t reporter_get_interval(void)
{
  return reporter_sec;
}

void reporter_set_interval(uint16_t val)
{
  reporter_sec = val;
  config_write_int("reporter_sec", val);
}

bool reporter_report_data(const sensor_data_t * data)
{
  return false;
}