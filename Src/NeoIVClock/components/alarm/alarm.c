#include "alarm.h"
#include "logger.h"
#include "config.h"
#include "task.h"

#include <stdio.h>

static const char * TAG = "ALARM";
static uint8_t alarm1_trigger_index;

#define ALARM1_MAX_COUNT 10

static alarm0_t alarm0;
static alarm1_t alarm1[ALARM1_MAX_COUNT];

static void alarm_dump(void)
{
  NEO_LOGD(TAG, "alarm0.enabled = %u", alarm0.enabled);
  NEO_LOGD(TAG, "-----------------------------------");
  for (int i = 0; i < ALARM1_MAX_COUNT; i++) {
    NEO_LOGD(TAG, "alarm1[%d].enabled     = %u", i, alarm1[i].enabled);
    NEO_LOGD(TAG, "alarm1[%d].day_of_week = %u", i, alarm1[i].day_of_week);
    NEO_LOGD(TAG, "alarm1[%d].hour        = %u", i, alarm1[i].hour);
    NEO_LOGD(TAG, "alarm1[%d].minute      = %u", i, alarm1[i].minute);
    NEO_LOGD(TAG, "alarm1[%d].snd_index   = %u", i, alarm1[i].snd_index);
    NEO_LOGD(TAG, "-----------------------------------");
  }
}


static void alarm_load_config(void)
{
  config_val_t val = {};
  //"alm00_cfg"
  char cfg_name[10] = {};
  uint8_t i;

  NEO_LOGD(TAG, "alarm_load_config");

  memset(&alarm0, 0, sizeof(alarm0));
  memset(alarm1, 0, sizeof(alarm1));

  alarm0.enabled = config_read_int("hourly_chime_en");

  for(i = 0 ; i < ALARM1_MAX_COUNT ; i++) {
    snprintf(cfg_name, sizeof(cfg_name), "alm%02d_cfg", i);
    cfg_name[sizeof(cfg_name) - 1] = 0;
    val.valblob.body = (uint8_t *)&alarm1[i];
    config_read(cfg_name, &val);
  }

  alarm_dump();
}

static void alarm_save_config(void)
{
  config_val_t val = {};
  //"alm00_cfg"
  char cfg_name[10] = {};
  uint8_t i;

  NEO_LOGD(TAG, "alarm_save_config");

  config_write_int("hourly_chime_en", alarm0.enabled);

  for(i = 0 ; i < ALARM1_MAX_COUNT ; i++) {
    snprintf(cfg_name, sizeof(cfg_name), "alm%02d_cfg", i);
    cfg_name[sizeof(cfg_name) - 1] = 0;
    val.valblob.body = (uint8_t *)&alarm1[i];
    val.valblob.len  = sizeof(alarm1_t);
    config_write(cfg_name, &val);
  }

}

void alarm_init(void)
{
  NEO_LOGI(TAG, "init");

  // load alarm0, alarm1 from config
  alarm_load_config();

  alarm1_trigger_index = 0;
}

void alarm_test(uint8_t day, uint8_t hour, uint8_t minute, uint8_t sec)
{
  NEO_EARLY_LOGD(TAG, "alarm_test: %u %02u:%02u:%02u", day,  hour, minute, sec);

  if (alarm0.enabled) {
    if (minute == 0 && sec == 0) {
      NEO_EARLY_LOGI(TAG, "alarm0 triggered!");
      task_set(EV_ALARM0);
    }
  }
  for (int i = 0; i < ALARM1_MAX_COUNT; i++) {
    if (alarm1[i].enabled && alarm1[i].day_of_week == day) {
      if (alarm1[i].hour == hour && alarm1[i].minute == minute && sec == 0) {
        NEO_EARLY_LOGI(TAG, "alarm1[%d] triggered!", i);
        alarm1_trigger_index = i;
        task_set(EV_ALARM1);
      }
    }
  }
}

