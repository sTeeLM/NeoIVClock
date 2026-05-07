#include "alarm.h"
#include "logger.h"

static const char * TAG = "ALARM";

#define ALARM1_MAX_COUNT 5

static alarm0_t alarm0;
static alarm1_t alarm1[ALARM1_MAX_COUNT];

static void alarm_dump(void)
{
  NEO_LOGD(TAG, "alarm0.enabled = %u", alarm0.enabled);
  NEO_LOGD(TAG, "alarm0.hour    = %u", alarm0.hour);
  NEO_LOGD(TAG, "alarm0.minute  = %u", alarm0.minute);

  for (int i = 0; i < ALARM1_MAX_COUNT; i++) {
    NEO_LOGD(TAG, "alarm1[%d].enabled     = %u", i, alarm1[i].enabled);
    NEO_LOGD(TAG, "alarm1[%d].day_of_week = %u", i, alarm1[i].day_of_week);
    NEO_LOGD(TAG, "alarm1[%d].hour        = %u", i, alarm1[i].hour);
    NEO_LOGD(TAG, "alarm1[%d].minute      = %u", i, alarm1[i].minute);
    NEO_LOGD(TAG, "alarm1[%d].snd_index   = %u", i, alarm1[i].snd_index);
  }
}


static void alarm_load_config(void)
{
  NEO_LOGD(TAG, "alarm_load_config");
  memset(&alarm0, 0, sizeof(alarm0));
  memset(alarm1, 0, sizeof(alarm1));

  // load config

  alarm_dump();
}

static void alarm_save_config(void)
{
  NEO_LOGD(TAG, "alarm_save_config");
}

void alarm_init(void)
{
  NEO_LOGI(TAG, "init");

  // load alarm0, alarm1 from config
  alarm_load_config();

}

void alarm_test(uint8_t day, uint8_t hour, uint8_t minute)
{
  NEO_LOGD(TAG, "alarm_test: %d %02d:%02d", day,  hour, minute);

  if (alarm0.enabled) {
    if (alarm0.hour == hour && alarm0.minute == minute) {
      NEO_LOGI(TAG, "alarm0 triggered!");
    }
  }
  for (int i = 0; i < ALARM1_MAX_COUNT; i++) {
    if (alarm1[i].enabled && alarm1[i].day_of_week == day) {
      if (alarm1[i].hour == hour && alarm1[i].minute == minute) {
        NEO_LOGI(TAG, "alarm1[%d] triggered!", i);
      }
    }
  }
}

