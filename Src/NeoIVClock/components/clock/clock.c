#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "driver/gptimer.h"
#include "esp_clk_tree.h"

#include "clock.h"
#include "logger.h"
#include "alarm.h"
#include "ds3231_rtc.h"
#include "ec11.h"
#include "config.h"
#include "terminal.h"
#include "iv18.h"


#include "cext.h"


#define CLOCK_FACTORY_RESET_HOUR 12
#define CLOCK_FACTORY_RESET_MIN  11
#define CLOCK_FACTORY_RESET_SEC  0
#define CLOCK_FACTORY_RESET_CENTRY 1
#define CLOCK_FACTORY_RESET_YEAR 14
#define CLOCK_FACTORY_RESET_MON  8
#define CLOCK_FACTORY_RESET_DATA 19
#define CLOCK_FACTORY_RESET_DAY  2
      
static const char * TAG = "CLOCK";

static uint8_t date_table[2][12] = 
{
{31,29,31,30,31,30,31,31,30,31,30,31,}, // 2000
{31,28,31,30,31,30,31,31,30,31,30,31,},
};

clock_struct_t clk;
static clock_struct_t saved_clk;
static uint32_t now_sec; // 用于 time_diff
static bool clock_is_hour12;

bool clock_test_hour12(void)
{
  return clock_is_hour12;
}

void clock_set_hour12(bool enable)
{
  clock_is_hour12 = enable;
}

void clock_save_config(void)
{
  config_val_t val;
  val.val8 = clock_is_hour12;
    config_write("time_12", &val);
}

// 计算某年某月某日星期几,  经典的Zeller公式
// year 0-99
// mon 0-11
// date 0-30
// return 0-6, 0 is monday, 6 is sunday
static uint8_t clock_yymmdd_to_day(uint16_t year, uint8_t mon, uint8_t date)
{
  unsigned int d,m,y,c;
  d = date + 1;
  m = mon + 1;
  y = year;
  
  if(m < 3){
    y -= 1;
    m += 12;
  }
  
  c = (unsigned int)(y / 100);
  y = y - 100 * c;
  
  c = (unsigned int)(c / 4) - 2 * c + y + (unsigned int) ( y / 4 ) + (26 * (m + 1) / 10) + d - 1;
  c = (c % 7 + 7) % 7;

  if(c == 0) {
    return 6;
  }
  
  return c - 1;
}

//
// 在调整了year，mon，date之后都得调用
//
static void clock_recal_date_day(void)
{
  // 如果是2月并且不是闰年，需要保证不能出现2月29日
  // 以及不能出现4月31日这样的情况
  if(clk.date > clock_get_mon_date(clk.year, clk.mon) - 1)
    clk.date = clock_get_mon_date(clk.year, clk.mon) - 1;
  clk.day = clock_yymmdd_to_day(clk.year, clk.mon, clk.date);
}

//
// 核心时钟中断函数
//


// 1 / 512 = 1.953125ms
void clock_inc_ms19(void)
{
  clk.ms19 ++;
  
  if((clk.ms19 % 512) == 0 ) {
    clk.ms19 = 0;
    ++ clk.sec;
    clk.sec = clk.sec % 60;
    now_sec ++;
    if(clk.sec == 0) {
      ++ clk.min;
      clk.min %=  60;
      if(clk.min == 0) {
        ++ clk.hour;
        clk.hour %= 24;
        alarm_test(clk.day + 1, clk.hour, clk.min);
        if(clk.hour == 0) {
          if(cext_is_leap_year(clk.year)) {
            ++ clk.date;
            clk.date %= date_table[0][clk.mon];
          } else {
            ++ clk.date;
            clk.date %= date_table[1][clk.mon];
          }
          ++ clk.day;
          clk.day %= 7;
          if(clk.date == 0) {
            ++ clk.mon;
            clk.mon %= 12;
            if(clk.mon == 0) {
              ++ clk.year;
              if(clk.year > 2099) {
                clk.year = 1901; // 看不到了吧，这个点我都120岁了，灰都没有了
                clock_recal_date_day();
                /*
                clock_enable_interrupt(false);
                clock_sync_to_rtc(CLOCK_SYNC_DATE); 
                clock_enable_interrupt(true);
                alarm_resync_rtc();
                */
              }
            }
          }
        }
      }
    } 
  }
}

void clock_show(void)
{
  terminal_printf("%04d-%02d-%02d  (%02d) %02d:%02d:%02d:%03d\r\n",
    clk.year, clk.mon + 1, clk.date + 1, clk.day + 1,
    clk.hour, clk.min, clk.sec, clk.ms19);
}

void clock_dump(void)
{
  NEO_LOGD(TAG, "clk.year = %u", clk.year);
  NEO_LOGD(TAG, "clk.mon  = %u", clk.mon);
  NEO_LOGD(TAG, "clk.date = %u", clk.date); 
  NEO_LOGD(TAG, "clk.day  = %u", clk.day);
  NEO_LOGD(TAG, "clk.hour = %u", clk.hour); 
  NEO_LOGD(TAG, "clk.min  = %u", clk.min);
  NEO_LOGD(TAG, "clk.sec  = %u", clk.sec);  
  NEO_LOGD(TAG, "clk.ms19 = %u", clk.ms19); 
}

uint16_t clock_get_ms19(void)
{
  return clk.ms19;
}

uint32_t clock_get_now_sec(void)
{
  return now_sec;
}

uint32_t clock_diff_now_sec(uint32_t sec)
{
  return (uint32_t)(now_sec - sec);
}

uint8_t clock_get_sec(void)
{
  return clk.sec;
}

void clock_set_sec(uint8_t sec)
{
  clk.sec = sec % 60;
}

void clock_clr_sec(void)
{
   clk.sec = 0;
}

uint8_t clock_get_min(void)
{
  return clk.min;
}

void clock_set_min(uint8_t min)
{
  clk.min = min % 60;
}

void clock_inc_min(void)
{
  ++ clk.min;
  clk.min %= 60;
}

uint8_t clock_get_hour(void)
{
  return clk.hour;
}

void clock_set_hour(uint8_t hour)
{
  clk.hour = hour % 24;
}

void clock_inc_hour(void)
{
  ++ clk.hour;
  clk.hour %= 24;
}


uint8_t clock_get_date(void)
{
  return clk.date + 1;
}


void clock_set_date(uint8_t date)
{
  if(date == 0)
    return;
  clk.date = (date - 1) % clock_get_mon_date(clk.year, clk.mon);
  clock_recal_date_day();
}

void clock_inc_date(void)
{
  ++ clk.date;
  clk.date %= clock_get_mon_date(clk.year, clk.mon);
  clock_recal_date_day();
}

uint8_t clock_get_day(void)
{
  return clk.day + 1;
}

uint8_t clock_get_month(void)
{
  return clk.mon + 1;
}


void clock_set_month(uint8_t mon)
{
  if(mon == 0)
    return;
  clk.mon = (mon - 1) % 12;
  clock_recal_date_day();
}

void clock_inc_month(void)
{
  ++ clk.mon;
  clk.mon %= 12;
  clock_recal_date_day();
}

uint16_t clock_get_year(void)
{
  return clk.year;
}

void clock_set_year(uint16_t year)
{
  if(year >= 1901 && year <= 2099) {
    clk.year = year;
    clock_recal_date_day();
  }
}

void clock_inc_year(void)
{
  ++ clk.year;
  if(clk.year > 2099)
    clk.year = 1901;
  clock_recal_date_day();
}

void clock_sync_from_rtc(clock_sync_type_t type)
{
  uint8_t year;
  bool centry;
  NEO_LOGD(TAG, "clock_sync_from_rtc = %u", type);
  if(type == CLOCK_SYNC_TIME) {
    ds3231_rtc_get_time(&clk.hour, &clk.min, &clk.sec);
    clk.ms19 = 255;   // 0 - 255
  } else if(type == CLOCK_SYNC_DATE) {
    centry = ds3231_rtc_get_date(&year, &clk.mon, &clk.date, &clk.day);
    clk.mon --; // rtc是1-12，clock是0-11 
    clk.date --; // rtc是1-31，clock是0-30
    clk.day --;  // rtc是1-7，clock是0-6
    if(centry) {
      clk.year = 2000 + year;
    } else {
      clk.year = 1900 + year;  
    }
  }
}

void clock_sync_to_rtc(clock_sync_type_t type)
{
  uint8_t year;
  bool centry = false;  
  NEO_LOGD(TAG, "clock_sync_to_rtc = %u\n", type);
  if(type == CLOCK_SYNC_TIME) {
  ds3231_rtc_set_time(clk.hour, clk.min, clk.sec);
  } else if(type == CLOCK_SYNC_DATE) {
    if(clk.year >= 2000) {
      centry = true;
    }
    year = clk.year % 100;
    ds3231_rtc_set_date(centry, year, clk.mon + 1, clk.date + 1, clk.day + 1);
  }
}

void clock_enable_interrupt(bool enable)
{
  ds3231_rtc_enable_32768HZ(enable);
}

// 辅助函数
bool clock_is_leap_year(uint16_t year)
{
  return cext_is_leap_year(year);
}

// 返回某一年某一月有几天
uint8_t clock_get_mon_date(uint16_t year, uint8_t mon)
{
  if(mon >= 12) mon = 11;
  if(cext_is_leap_year(year))
    return date_table[0][mon];
  else
    return date_table[1][mon];
}

// 每1/1024秒 = 0.9765625ms 触发一次中断，在中断里更新时钟
static bool clock_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx)
{
  clock_inc_ms19();

  iv18_scan();

  return false; // 返回false表示不需要yield
}


void clock_init(void)
{
  NEO_LOGI(TAG, "init");

  uint32_t freq_hz = 0;
  
  gptimer_handle_t gptimer = NULL;
  gptimer_config_t timer_config = {
      .clk_src   =  GPTIMER_CLK_SRC_XTAL,
      .direction =  GPTIMER_COUNT_UP,
      .resolution_hz = 32768, // 设置分辨率为 32768Hz，这样 1 个 tick 就是 1 个脉冲
  };

  gptimer_alarm_config_t alarm_config = {
      .alarm_count = 64,                 // 达到 64 次计数（即 0.9765625ms）触发中断
      .reload_count = 0,                  // 硬件自动重载回 0
      .flags.auto_reload_on_alarm = true, // 开启【硬件层面】自动重载
  };

  gptimer_event_callbacks_t timer_cbs = {
    .on_alarm = clock_cb, // register user callback
  };

  ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));

  ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer, &alarm_config));

  ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer, &timer_cbs, NULL));

  if(ec11_is_factory_reset()) {
    NEO_LOGI(TAG, "clock factory reset time\n");

    ds3231_rtc_set_time(
      CLOCK_FACTORY_RESET_HOUR,
      CLOCK_FACTORY_RESET_MIN,
      CLOCK_FACTORY_RESET_SEC);

    NEO_LOGI(TAG, "clock factory reset date\n");
    ds3231_rtc_set_date(
      CLOCK_FACTORY_RESET_CENTRY,
      CLOCK_FACTORY_RESET_YEAR,
      CLOCK_FACTORY_RESET_MON,
      CLOCK_FACTORY_RESET_DATA,
      CLOCK_FACTORY_RESET_DAY);
  }
  // 初始化时钟
  clock_sync_from_rtc(CLOCK_SYNC_TIME);
  clock_sync_from_rtc(CLOCK_SYNC_DATE); 
  clock_enable_interrupt(true);
  clock_is_hour12 = config_read_int("time_12");
  clock_dump();

  esp_clk_tree_src_get_freq_hz(SOC_MOD_CLK_XTAL32K, ESP_CLK_TREE_SRC_FREQ_PRECISION_EXACT, &freq_hz);
  NEO_LOGI(TAG, "clock freq is %u Hz", freq_hz);


  ESP_ERROR_CHECK(gptimer_enable(gptimer));
  ESP_ERROR_CHECK(gptimer_start(gptimer));
}


void clock_enter_powersave(void)
{
  NEO_LOGD(TAG, "clock_enter_powersave\n");
  clock_enable_interrupt(false);
  
  // 保存时间，当醒来的时候看看有没有从20xx->1900
  // 是的话，得跨越到1901，因为rtc把1900当闰年
  // 同时如果穿越回去的话，得重新计算day保证形式上逻辑正确
  memcpy(&saved_clk, &clk, sizeof(saved_clk));
}

void clock_leave_powersave(void)
{
  NEO_LOGD(TAG, "clock_leave_powersave\n");
  
  clock_sync_from_rtc(CLOCK_SYNC_TIME);
  clock_sync_from_rtc(CLOCK_SYNC_DATE);
  
  if(saved_clk.year >=2000 && clk.year <= 1999) {
    if(clk.year == 1900) {
      clk.year = 1901;
    }
    clock_recal_date_day();
    clock_sync_to_rtc(CLOCK_SYNC_DATE);   
  }    
  clock_enable_interrupt(true);
}

void clock_enter_console(void)
{
  clock_enable_interrupt(false);
}

void clock_leave_console(void)
{
  clock_sync_from_rtc(CLOCK_SYNC_TIME);
  clock_sync_from_rtc(CLOCK_SYNC_DATE);
  clock_enable_interrupt(true);
}
