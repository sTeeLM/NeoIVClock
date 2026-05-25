#include "alarm.h"
#include "logger.h"
#include "config.h"
#include "task.h"
#include "player.h"

#include <stdio.h>

static const char * TAG = "ALARM";
static uint8_t alarm1_trigger_index;

static alarm0_t alarm0;
static alarm1_t alarm1[ALARM1_MAX_COUNT];

#define ALARM_HOUR_FAST_STEP 5
#define ALARM_MIN_FAST_STEP  5
#define ALARM_SND_FAST_STEP  1

static void alarm_dump(void)
{
  NEO_LOGD(TAG, "alarm0.enabled = %u", alarm0.enabled);
  NEO_LOGD(TAG, "-----------------------------------");
  for (int i = 0; i < ALARM1_MAX_COUNT; i++) {
    NEO_LOGD(TAG, "alarm1[%d].enabled     = %u", i, alarm1[i].enabled);
    NEO_LOGD(TAG, "alarm1[%d].day_mask    = %02x", i, alarm1[i].day_mask);
    NEO_LOGD(TAG, "alarm1[%d].hour        = %u", i, alarm1[i].hour);
    NEO_LOGD(TAG, "alarm1[%d].minute      = %u", i, alarm1[i].minute);
    NEO_LOGD(TAG, "alarm1[%d].snd         = %u", i, alarm1[i].snd);
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
  uint8_t i;

  NEO_LOGI(TAG, "alarm_save_config");

  alarm0_save_config();

  for(i = 0 ; i < ALARM1_MAX_COUNT ; i++) {
    alarm1_save_config(i);
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
    if(day > 7 || day == 0) {
      NEO_EARLY_LOGW(TAG, "invalid day = %d", day);
    }
    day = (day - 1) % 8;
    if (alarm1[i].enabled && alarm1[i].day_mask & 1 << day ) {
      if (alarm1[i].hour == hour && alarm1[i].minute == minute && sec == 0) {
        NEO_EARLY_LOGI(TAG, "alarm1[%d] triggered!", i);
        alarm1_trigger_index = i;
        task_set(EV_ALARM1);
      }
    }
  }
}


bool alarm1_get_enabled(uint8_t alarm1_index)
{
  alarm1_index %= ALARM1_MAX_COUNT;
  return alarm1[alarm1_index].enabled;
}
bool alarm1_enable(uint8_t alarm1_index, bool enable)
{
  alarm1_index %= ALARM1_MAX_COUNT;
  alarm1[alarm1_index].enabled = enable;
  return alarm1[alarm1_index].enabled;
}

uint8_t alarm1_get_hour(uint8_t alarm1_index)
{
  alarm1_index %= ALARM1_MAX_COUNT;
  return alarm1[alarm1_index].hour;
}
uint8_t alarm1_inc_hour(uint8_t alarm1_index, bool fast)
{
  alarm1_index %= ALARM1_MAX_COUNT;
  alarm1[alarm1_index].hour = 
    (alarm1[alarm1_index].hour + (fast ? ALARM_HOUR_FAST_STEP : 1)) % 24;
  return alarm1[alarm1_index].hour;
}
uint8_t alarm1_dec_hour(uint8_t alarm1_index, bool fast)
{
  alarm1_index %= ALARM1_MAX_COUNT;
  alarm1[alarm1_index].hour = 
    (alarm1[alarm1_index].hour + 24 - (fast ? ALARM_HOUR_FAST_STEP : 1)) % 24;
  return alarm1[alarm1_index].hour;
}

uint8_t alarm1_get_min(uint8_t alarm1_index)
{
  alarm1_index %= ALARM1_MAX_COUNT;
  return alarm1[alarm1_index].minute;
}
uint8_t alarm1_inc_min(uint8_t alarm1_index, bool fast)
{
  alarm1_index %= ALARM1_MAX_COUNT;
  alarm1[alarm1_index].minute = 
    (alarm1[alarm1_index].minute + (fast ? ALARM_MIN_FAST_STEP : 1)) % 60;
  return alarm1[alarm1_index].minute;
}
uint8_t alarm1_dec_min(uint8_t alarm1_index, bool fast)
{
  alarm1_index %= ALARM1_MAX_COUNT;
  alarm1[alarm1_index].minute = 
    (alarm1[alarm1_index].minute + 60 - (fast ? ALARM_MIN_FAST_STEP : 1)) % 60;
  return alarm1[alarm1_index].minute;
}

uint8_t alarm1_get_day_mask(uint8_t alarm1_index)
{
  alarm1_index %= ALARM1_MAX_COUNT;
  return alarm1[alarm1_index].day_mask;
}
uint8_t alarm1_set_day_mask(uint8_t alarm1_index, uint8_t day_mask)
{
  alarm1_index %= ALARM1_MAX_COUNT;
  alarm1[alarm1_index].day_mask = day_mask & 0x7F;
  return alarm1[alarm1_index].day_mask;
}

uint8_t alarm1_get_snd(uint8_t alarm1_index)
{
  alarm1_index %= ALARM1_MAX_COUNT;
  return alarm1[alarm1_index].snd;
}
uint8_t alarm1_inc_snd(uint8_t alarm1_index, bool fast)
{
  uint16_t snd_cnt = player_get_snd_cnt(PLAYER_SND_DIR_ALARM);
  alarm1_index %= ALARM1_MAX_COUNT;
  alarm1[alarm1_index].snd = 
    (alarm1[alarm1_index].snd + (fast ? ALARM_SND_FAST_STEP : 1)) % snd_cnt;
  return alarm1[alarm1_index].snd;
}
uint8_t alarm1_dec_snd(uint8_t alarm1_index, bool fast)
{
  uint16_t snd_cnt = player_get_snd_cnt(PLAYER_SND_DIR_ALARM);
  alarm1_index %= ALARM1_MAX_COUNT;
  alarm1[alarm1_index].snd = 
    (alarm1[alarm1_index].snd + snd_cnt - (fast ? ALARM_SND_FAST_STEP : 1)) % snd_cnt;
  return alarm1[alarm1_index].snd;
}

void alarm1_save_config(uint8_t alarm1_index)
{
  
  config_val_t val = {};
  //"alm00_cfg"
  char cfg_name[10] = {};
  NEO_LOGI(TAG, "alarm1_save_config %d", alarm1_index);
  
  alarm1_index %= ALARM1_MAX_COUNT;
  snprintf(cfg_name, sizeof(cfg_name), "alm%02d_cfg", alarm1_index);
  cfg_name[sizeof(cfg_name) - 1] = 0;
  val.valblob.body = (uint8_t *)&alarm1[alarm1_index];
  val.valblob.len  = sizeof(alarm1_t);
  config_write(cfg_name, &val);
}

bool alarm0_get_enabled(void)
{
  return alarm0.enabled;
}
bool alarm0_enable(bool enable)
{
  alarm0.enabled = enable;
  return alarm0.enabled;
}
void alarm0_save_config(void)
{
  NEO_LOGI(TAG, "alarm0_save_config");
  config_write_int("hourly_chime_en", alarm0.enabled);
}
