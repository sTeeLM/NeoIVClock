#include "timer.h"
#include "logger.h"
#include "config.h"
#include "player.h"
#include "task.h"
#include "iv18.h"

#include <string.h>

#include <stdatomic.h>

static const char * TAG = "TIMER";

#define TIMER_FAST_STEP 5

#define TIMER_SLOT_CNT 10
EXT_RAM_BSS_ATTR timer_struct_t tmr[TIMER_SLOT_CNT]; // slot0 当前timer，剩下是纪录的瞬时值

static timer_mode_t timer_mode;
static bool timer_refresh;
static bool timer_countdown_stop;
static bool timer_started;
static uint8_t timer_current_slot;
static uint8_t timer_current_history_slot;
static uint8_t timer_snd_index;

static atomic_flag timer_lock = ATOMIC_FLAG_INIT;

void timer_inc_ms19(void)
{

  if(!timer_started)
    return;
  
  if (!atomic_flag_test_and_set(&timer_lock)) {
    if(timer_mode == TIMER_MODE_INC) {
      tmr[0].ms19 = (tmr[0].ms19 + 1) % 512;
      if(tmr[0].ms19 == 0 ) {
        tmr[0].sec = ( tmr[0].sec + 1) % 60;
        if(tmr[0].sec == 0) {
          tmr[0].min = ( tmr[0].min + 1) % 60;
          if(tmr[0].min == 0) {
            tmr[0].hour = (tmr[0].hour + 1) % 100;
          }
        }
      }
    } else {
      if(!timer_countdown_stop) {
        if(tmr[0].ms19 != 0
          || tmr[0].sec != 0
          || tmr[0].min != 0
          || tmr[0].hour != 0) {
          tmr[0].ms19 = (tmr[0].ms19 + 512 - 1) % 512;
          if(tmr[0].ms19 == 511 ) {
            tmr[0].sec --;
            if(tmr[0].sec == 255){
              tmr[0].sec = 59;
              tmr[0].min --;
              if(tmr[0].min == 255){ 
                tmr[0].min = 59;
                if(tmr[0].hour > 0) {
                  tmr[0].hour --;
                }
              }
            }
          }
        } else {
          timer_countdown_stop = true;
          task_set(EV_TIMER);
        }
      }
    }
    atomic_flag_clear(&timer_lock);
  } else {
    NEO_EARLY_LOGW(TAG, "lost ticket!");
  } 
  timer_refresh_display(0);
}

void timer_init(void)
{
  NEO_LOGI(TAG, "init");
  timer_mode = TIMER_MODE_DEC;
  timer_refresh = false;
  timer_countdown_stop = true;
  timer_started = false;
  timer_current_slot = 1;
  timer_current_history_slot = 0;
  timer_snd_index = config_read_int("timer_snd");
}

void timer_display_enable(bool enable)
{
  timer_refresh = enable;
}

void timer_refresh_display(uint8_t slot)
{
  if(!timer_refresh)
    return;

  slot %= TIMER_SLOT_CNT;

  iv18_set_dig(1, tmr[slot].hour / 10 + 0x30);
  iv18_set_dig(2, tmr[slot].hour % 10 + 0x30);
  iv18_set_dp(2);
  iv18_set_dig(3, tmr[slot].min / 10 + 0x30);
  iv18_set_dig(4, tmr[slot].min % 10 + 0x30);
  iv18_set_dp(4);
  iv18_set_dig(5, tmr[slot].sec / 10 + 0x30);
  iv18_set_dig(6, tmr[slot].sec % 10 + 0x30);
  iv18_set_dp(6);
  iv18_set_dig(7, (tmr[slot].ms19 * 100 / 512) / 10 + 0x30);
  iv18_set_dig(8, (tmr[slot].ms19 * 100 / 512) % 10 + 0x30);  
}


void timer_set_mode(timer_mode_t mode)
{
  timer_mode = mode;
}

void timer_start(void)
{
  NEO_LOGD(TAG, "timer_start mode is %u", timer_mode);
  timer_countdown_stop = false;
  timer_started = 1;
}

uint8_t timer_save(void)
{
  atomic_flag_test_and_set(&timer_lock);
  timer_current_history_slot = timer_current_slot;
  memcpy(&tmr[timer_current_slot], &tmr[0], sizeof(tmr[0]));
  atomic_flag_clear(&timer_lock);
  timer_current_slot = ((timer_current_slot) % (TIMER_SLOT_CNT - 1)) + 1;
  return timer_current_history_slot;
}

uint8_t timer_get_current_slot(void)
{
  return timer_current_history_slot;
}

uint8_t timer_get_slot_cnt(void)
{
  return TIMER_SLOT_CNT;
}

void timer_set(uint8_t hour, uint8_t min, uint8_t sec)
{
  tmr[0].hour = hour % 100;
  tmr[0].min  = min % 60;
  tmr[0].sec  = sec % 60;
  tmr[0].ms19 = 0;
}

uint8_t timer_get_hour(uint8_t slot)
{
  slot %= TIMER_SLOT_CNT;
  return tmr[slot].hour;
}
uint8_t timer_inc_hour(bool fast)
{
  tmr[0].hour = (tmr[0].hour + (fast ? TIMER_FAST_STEP : 1)) % 100;
  return tmr[0].hour;
}

uint8_t timer_dec_hour(bool fast)
{
  tmr[0].hour = (tmr[0].hour + 100 - (fast ? TIMER_FAST_STEP : 1)) % 100;
  return tmr[0].hour;
}

uint8_t timer_get_min(uint8_t slot)
{
  slot %= TIMER_SLOT_CNT;
  return tmr[slot].min;
}
uint8_t timer_inc_min(bool fast)
{
  tmr[0].min = (tmr[0].min + (fast ? TIMER_FAST_STEP : 1)) % 60;
  return tmr[0].min;
}

uint8_t timer_dec_min(bool fast)
{
  tmr[0].min = (tmr[0].min + 60 - (fast ? TIMER_FAST_STEP : 1)) % 60;
  return tmr[0].min;
}

uint8_t timer_get_sec(uint8_t slot)
{
  slot %= TIMER_SLOT_CNT;
  return tmr[slot].sec;
}

uint8_t timer_inc_sec(bool fast)
{
  tmr[0].sec = (tmr[0].sec + (fast ? TIMER_FAST_STEP : 1)) % 60;
  return tmr[0].sec;
}

uint8_t timer_dec_sec(bool fast)
{
  tmr[0].sec = (tmr[0].sec + 60 - (fast ? TIMER_FAST_STEP : 1)) % 60;
  return tmr[0].sec;
}

uint8_t timer_get_10ms(uint8_t slot)
{
  slot %= TIMER_SLOT_CNT;
  return tmr[slot].ms19 * 100 / 512;
}

void timer_stop(void)
{
  timer_started = false;
}

void timer_resume(void)
{
  timer_started = true;
}

void timer_clr(void)
{
  memset(tmr, 0, sizeof(tmr));
  timer_countdown_stop = true;
  timer_started = false;
  timer_current_slot = 1;
  timer_current_history_slot = 0;
}

uint8_t timer_get_snd(void)
{
  return timer_snd_index;
}
uint8_t timer_inc_snd(void)
{
  uint8_t snd_cnt = player_get_snd_cnt(PLAYER_SND_DIR_EFFETS);
  timer_snd_index = (timer_snd_index + 1) % snd_cnt;
  return timer_snd_index;
}
uint8_t timer_dec_snd(void)
{
  uint8_t snd_cnt = player_get_snd_cnt(PLAYER_SND_DIR_EFFETS);
  timer_snd_index = (timer_snd_index + (snd_cnt - 1)) % snd_cnt;
  return timer_snd_index;
}
void timer_save_config(void)
{
  config_write_int("timer_snd", timer_snd_index);
}