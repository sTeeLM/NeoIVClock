#include "alarm.h"
#include "logger.h"
#include "config.h"
#include "sm.h"
#include "task.h"
#include "player.h"

static const char * TAG = "ALARM";
static uint8_t alarm1_trigger_index;

static alarm0_t alarm0;
static alarm1_t alarm1[ALARM1_MAX_COUNT];

#define ALARM_HOUR_FAST_STEP 5
#define ALARM_MIN_FAST_STEP  5
#define ALARM_SND_FAST_STEP  1

static void alarm_dump(void)
{
  NEO_LOGD(TAG, "alarm0.begin_hour = %u", alarm0.begin_hour);
  NEO_LOGD(TAG, "alarm0.end_hour   = %u", alarm0.end_hour);
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

  val.valblob.len  = sizeof(alarm0_t);
  val.valblob.body = (const uint8_t *)&alarm0;
  config_read("hourly_chime", &val);

  for(i = 0 ; i < ALARM1_MAX_COUNT ; i++) {
    snprintf(cfg_name, sizeof(cfg_name), "alm%02d_cfg", i);
    cfg_name[sizeof(cfg_name) - 1] = 0;
    val.valblob.body = (uint8_t *)&alarm1[i];
    config_read(cfg_name, &val);
  }

  alarm_dump();
}


void alarm_init(void)
{
  NEO_LOGI(TAG, "init");

  // load alarm0, alarm1 from config
  alarm_load_config();

  alarm1_trigger_index = 0;
}

static bool alarm0_test(uint8_t begin, uint8_t end, uint8_t current) 
{
    if (begin < end) {
        // 普通区间：例如 08:00 到 17:00
        return (current >= begin && current < end);
    } else if (begin > end) {
        // 跨零点区间：例如 22:00 到 06:00
        return (current >= begin || current < end);
    } else {
        // begin == end，区间为空（左闭右开排除end）
        return false;
    }
}

void alarm_test(uint8_t day, uint8_t hour, uint8_t minute)
{
  NEO_EARLY_LOGI(TAG, "alarm_test: %u %02u:%02u", day,  hour, minute);

  if (minute == 0) {
    NEO_EARLY_LOGI(TAG, "alarm0 test [%u][%u][%u]", alarm0.begin_hour, hour, alarm0.end_hour);
    if(alarm0_test(alarm0.begin_hour, alarm0.end_hour, hour)) {
      NEO_EARLY_LOGI(TAG, "alarm0 triggered!");
      task_set(EV_ALARM0);
    }
  }

  if(day >= 7) {
    NEO_EARLY_LOGE(TAG, "invalid day = %d", day);
    day %= 7;
  }
  
  for (int i = 0; i < ALARM1_MAX_COUNT; i++) {
    if ((alarm1[i].enabled && alarm1[i].day_mask & 1 << day )
    || (alarm1[i].enabled && alarm1[i].day_mask == 0)) { // 如果alarm的“重复”没有配置，响一次
      if (alarm1[i].hour == hour && alarm1[i].minute == minute) {
        NEO_EARLY_LOGI(TAG, "alarm1[%d] triggered, snd %u!", i, alarm1[i].snd);
        alarm1_trigger_index = i;
        task_set_ev_arg(EV_ALARM1, alarm1[i].snd);
      }
    }
  }
}

void alarm_proc(task_event_t ev)
{
  // 不要忘了对于一次性闹钟，关闭它
  if(ev == EV_ALARM1) {
    if(alarm1[alarm1_trigger_index].day_mask == 0) {
      NEO_LOGI(TAG, "disable one short alarm1 %u", alarm1_trigger_index);
      alarm1[alarm1_trigger_index].enabled = false;
      alarm1_save_config(alarm1_trigger_index);
    }
  }
  sm_run(ev);
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
  uint8_t snd_cnt = player_get_snd_cnt(PLAYER_SND_DIR_ALARM);
  alarm1_index %= ALARM1_MAX_COUNT;
  alarm1[alarm1_index].snd = 
    (alarm1[alarm1_index].snd + (fast ? ALARM_SND_FAST_STEP : 1)) % snd_cnt;
  return alarm1[alarm1_index].snd;
}
uint8_t alarm1_dec_snd(uint8_t alarm1_index, bool fast)
{
  uint8_t snd_cnt = player_get_snd_cnt(PLAYER_SND_DIR_ALARM);
  alarm1_index %= ALARM1_MAX_COUNT;
  alarm1[alarm1_index].snd = 
    (alarm1[alarm1_index].snd + snd_cnt - (fast ? ALARM_SND_FAST_STEP : 1)) % snd_cnt;
  return alarm1[alarm1_index].snd;
}

void alarm0_save_config(void)
{
  config_val_t val = {};
  NEO_LOGI(TAG, "alarm0_save_config");
  val.valblob.len  = sizeof(alarm0_t);
  val.valblob.body = (const uint8_t *)&alarm0;
  config_write("hourly_chime", &val);
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
  return alarm0.begin_hour != alarm0.end_hour;
}

uint8_t alarm0_get_begin(void)
{
  return alarm0.begin_hour;
}

uint8_t alarm0_inc_begin(bool fast)
{
  alarm0.begin_hour = 
    (alarm0.begin_hour + (fast ? ALARM_HOUR_FAST_STEP : 1)) % 24;
  return alarm0.begin_hour;
}

uint8_t alarm0_dec_begin(bool fast)
{
  alarm0.begin_hour = 
    (alarm0.begin_hour + 24 - (fast ? ALARM_HOUR_FAST_STEP : 1)) % 24;
  return alarm0.begin_hour;
}

uint8_t alarm0_get_end(void)
{
  return alarm0.end_hour;
}

uint8_t alarm0_inc_end(bool fast)
{
  alarm0.end_hour = 
    (alarm0.end_hour + (fast ? ALARM_HOUR_FAST_STEP : 1)) % 24;
  return alarm0.end_hour;
}

uint8_t alarm0_dec_end(bool fast)
{
  alarm0.end_hour = 
    (alarm0.end_hour + 24 - (fast ? ALARM_HOUR_FAST_STEP : 1)) % 24;
  return alarm0.end_hour;
}
