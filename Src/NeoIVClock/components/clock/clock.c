#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "driver/gpio.h"

#include "clock.h"
#include "logger.h"
#include "alarm.h"
#include "ds3231_rtc.h"
#include "ec11.h"
#include "config.h"
#include "sensor_data.h"
#include "terminal.h"
#include "iv18.h"
#include "gpio_wrapper.h"
#include "task.h"
#include "cext.h"
#include "timer.h"
#include "ws2812b.h"

#include <stdatomic.h>

static atomic_flag clock_lock = ATOMIC_FLAG_INIT;

#define CLOCK_FACTORY_RESET_HOUR 12
#define CLOCK_FACTORY_RESET_MIN  11
#define CLOCK_FACTORY_RESET_SEC  0
#define CLOCK_FACTORY_RESET_YEAR 14
#define CLOCK_FACTORY_RESET_MON  8
#define CLOCK_FACTORY_RESET_DATE 19
#define CLOCK_FACTORY_RESET_DAY  2

#define CLOCK_FAST_STEP 5
#define CLOCK_MIN_YEAR  1900
#define CLOCK_MAX_YEAR  9999
      
static const char * TAG = "CLOCK";

static uint8_t date_table[2][12] = 
{
{31,29,31,30,31,30,31,31,30,31,30,31,}, // 2000
{31,28,31,30,31,30,31,31,30,31,30,31,},
};

static clock_struct_t clk;
static uint32_t now_sec; // 用于 time_diff
static clock_display_mode_t clock_display_mode; // 显示模式
static uint8_t clock_date_index; // 用于滚动显示日期
static uint8_t clock_update_cnt; // 用于计数更新次数
static bool clock_ntp_en;        // 是否接受NTP对时
static uint32_t clock_lost_ticks; // 由于各种原因禁止时钟中断导致的ticks损失
static uint8_t clock_saved_century; // 用来避免频繁写入ROM

bool clock_test_hour12(void)
{
  return clk.is_hour12;
}

void clock_set_hour12(bool enable)
{
  atomic_flag_test_and_set(&clock_lock);
  clk.is_hour12 = enable;
  atomic_flag_clear(&clock_lock);
}

void clock_save_config(void)
{
  config_write_int("time_12", clk.is_hour12 ? 1 : 0);
  config_write_int("clock_ntp_en", clock_ntp_en ? 1 : 0);
}

// 计算某年某月某日星期几,  经典的Zeller公式
// year 0-99
// mon 0-11
// date 0-30
// return 0-6, 0 is sunday
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
  return c;
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

static void clock_recal_rtc(void)
{
  NEO_EARLY_LOGW(TAG, "clock_recal_rtc");
  // 发送事件，触发RTC校准(写入时间日期，清除century标志)，并保存century到ROM中
  task_set(EV_CAL_RTC);
}

// 处理EV_CAL_RTC, 终于不在中断上下文中了。。。
void clock_recal_rtc_proc(task_event_t ev)
{
  NEO_LOGW(TAG, "clock_recal_rtc_proc");
  clock_sync_to_rtc(CLOCK_SYNC_DATE);
  clock_sync_to_rtc(CLOCK_SYNC_TIME);
}

void clock_get_timeinfo(struct tm * timeinfo)
{
  memset(timeinfo, 0, sizeof(struct tm));
  atomic_flag_test_and_set(&clock_lock);
  timeinfo->tm_year = clk.year - 1900;
  timeinfo->tm_mon  = clk.mon;
  timeinfo->tm_mday = clk.date + 1;
  timeinfo->tm_hour = clk.hour;
  timeinfo->tm_min  = clk.min;
  timeinfo->tm_sec  = clk.sec;
  atomic_flag_clear(&clock_lock);
}

void clock_time_sync_proc(task_event_t ev)
{
  struct timeval tv;
  struct tm timeinfo;
  
  gettimeofday(&tv, NULL); // 获取系统硬件 RTC 的时间戳
  localtime_r(&tv.tv_sec, &timeinfo); // 转换为年月日时分秒结构体

  NEO_LOGI(TAG, "clock_time_sync_proc");

  ws2812b_blink(255, 0, 255);

  if(!clock_ntp_en) {
    NEO_LOGW(TAG, "skip ntp time sync due config!");
    return;
  }

  atomic_flag_test_and_set(&clock_lock);
  clk.year = timeinfo.tm_year + 1900;
  clk.mon  = timeinfo.tm_mon;
  clk.date = timeinfo.tm_mday - 1;
  clk.day  = timeinfo.tm_wday; // sunday == 0
  clk.hour = timeinfo.tm_hour;
  clk.min  = timeinfo.tm_min;
  clk.sec  = timeinfo.tm_sec;
  clk.ms19 = tv.tv_usec * 512 / 1000000;
  atomic_flag_clear(&clock_lock);

  clock_sync_to_rtc(CLOCK_SYNC_DATE);
  clock_sync_to_rtc(CLOCK_SYNC_TIME);
}

static void clock_update_display(void)
{
#define CLOCK_DISPLAY_DATE_BUFFER_SIZE 13  
  bool is_pm = false;
  uint8_t hour, day;
  char date_buffer[CLOCK_DISPLAY_DATE_BUFFER_SIZE];
  clock_update_cnt  = (clock_update_cnt + 1) % 16;
  switch (clock_display_mode) {
    case CLOCK_DISPLAY_MODE_DISABLE:
      break;
    case CLOCK_DISPLAY_MODE_DATE:
      // 显示完整日期需要13个字符，我们只有8个，所以只能滚动了
      date_buffer[0] = clock_get_year() / 1000 + 0x30;
      date_buffer[1] = (clock_get_year() % 1000 )/ 100  + 0x30;
      date_buffer[2] = (clock_get_year() % 100 )/ 10  + 0x30;
      date_buffer[3] = clock_get_year() % 10  + 0x30;
      date_buffer[4] = '-';
      date_buffer[5] = clock_get_month() / 10 + 0x30;
      date_buffer[6] = clock_get_month() % 10 + 0x30;  
      date_buffer[7] = '-';
      date_buffer[8] = clock_get_date() / 10 + 0x30;
      date_buffer[9] = clock_get_date() % 10 + 0x30; 
      date_buffer[10] = '-';
      day = clock_get_day();
      if(day != 0)
        date_buffer[11] = clock_get_day() + 0x30;
      else
        date_buffer[11] = 8 + 0x30;
      date_buffer[12] = IV18_BLANK;

      iv18_set_dig(1, date_buffer[clock_date_index % CLOCK_DISPLAY_DATE_BUFFER_SIZE]);
      iv18_set_dig(2, date_buffer[(clock_date_index + 1) % CLOCK_DISPLAY_DATE_BUFFER_SIZE]);
      iv18_set_dig(3, date_buffer[(clock_date_index + 2) % CLOCK_DISPLAY_DATE_BUFFER_SIZE]);
      iv18_set_dig(4, date_buffer[(clock_date_index + 3) % CLOCK_DISPLAY_DATE_BUFFER_SIZE]);
      iv18_set_dig(5, date_buffer[(clock_date_index + 4) % CLOCK_DISPLAY_DATE_BUFFER_SIZE]);
      iv18_set_dig(6, date_buffer[(clock_date_index + 5) % CLOCK_DISPLAY_DATE_BUFFER_SIZE]);
      iv18_set_dig(7, date_buffer[(clock_date_index + 6) % CLOCK_DISPLAY_DATE_BUFFER_SIZE]);
      iv18_set_dig(8, date_buffer[(clock_date_index + 7) % CLOCK_DISPLAY_DATE_BUFFER_SIZE]);
      if(clock_update_cnt == 0)
        clock_date_index = (clock_date_index + 1) % CLOCK_DISPLAY_DATE_BUFFER_SIZE;
      break;

    case CLOCK_DISPLAY_MODE_TIME:
      if(clk.is_hour12) {
        is_pm = cext_cal_hour12(clock_get_hour(), &hour);
        // 第0位只有一个横线和一个圆点，我们用圆点表示PM/AM
        if(is_pm) {
          iv18_set_dp(0);
        } else {
          iv18_clr_dp(0);
        }
      } else {
        hour = clock_get_hour();
        iv18_clr_dp(0);
      }
      iv18_set_dig(1, (hour / 10) + 0x30);
      iv18_set_dig(2, (hour % 10) + 0x30);
      iv18_set_dig(3, '-');
      iv18_set_dig(4, (clock_get_min() / 10) + 0x30);
      iv18_set_dig(5, (clock_get_min() % 10) + 0x30);  
      iv18_set_dig(6, '-');  
      iv18_set_dig(7, (clock_get_sec() / 10) + 0x30);
      iv18_set_dig(8, (clock_get_sec() % 10) + 0x30); 
      break;
    
    case CLOCK_DISPLAY_MODE_COMPOUND_DATE:
      iv18_set_dig(1, clock_get_year() / 1000 + 0x30);
      iv18_set_dig(2, (clock_get_year() % 1000 )/ 100  + 0x30);
      iv18_set_dig(3, (clock_get_year() % 100 )/ 10  + 0x30);
      iv18_set_dig(4, clock_get_year() % 10  + 0x30);
      iv18_set_dig(5, clock_get_month() / 10 + 0x30);
      iv18_set_dig(6, clock_get_month() % 10 + 0x30);      
      iv18_set_dig(7, clock_get_date() / 10 + 0x30);
      iv18_set_dig(8, clock_get_date() % 10 + 0x30);
      break;
  }
}
// 每当新的一分钟开始时会被调用，用来实现crontab逻辑
static void clock_test(uint8_t day, uint8_t hour, uint8_t min)
{
  // 有闹钟吗？
  alarm_test(day, hour, min);

  // 应该调整传感器吗？
  sensor_data_test(day, hour, min);
}

//
// 核心时钟中断函数
//
// 1 / 512 = 1.953125ms
// 需要特别处理2.28日到2.29/3.1日的情况：ds3231只能正确处理2000到2100之间的闰年
// 需要特别处理99年12月31日到100年1月1日的世纪跳变
// 这两种情况，都需要重新刷写RTC和ROM，注意世纪跳变需要清除century标志
void clock_inc_ms19(void)
{

  if (!atomic_flag_test_and_set(&clock_lock)) {
    clk.ms19 ++;

    if(clock_lost_ticks) {
      NEO_EARLY_LOGW(TAG, "lost ticks %u", clock_lost_ticks);
      clock_lost_ticks = 0;
      clk.ms19 += clock_lost_ticks;
    }

    if(clk.ms19 % 128 == 0) {
      task_set(EV_250MS);
    }

    // 每秒钟扫描16次按键
    if(clk.ms19 % 32 == 0) {
      task_set(EV_EC11_SCAN);
    }

    if((clk.ms19 % 512) == 0 ) {
      clk.ms19 = 0;
      ++ clk.sec;
      clk.sec = clk.sec % 60;
      now_sec ++;
      task_set(EV_1S);
      if(clk.sec % 10 == 0) {
        task_set(EV_10S);
      }
      if(clk.sec == 0) {
        ++ clk.min;
        clk.min %=  60;
        if(clk.min == 0) {
          ++ clk.hour;
          clk.hour %= 24;
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
            if(clk.mon == 1 && clk.date == 28) { // 2月29日，可能需要重新校准RTC, RTC中可能已经3月1日了
              clock_recal_rtc();
            }
            if(clk.date == 0) {
              ++ clk.mon;
              clk.mon %= 12;
              if(clk.mon == 2) { // 3月1日，可能需要重新校准RTC， RTC中可能还是2月29日
                clock_recal_rtc();
              }
              if(clk.mon == 0) {
                ++ clk.year;
                if(clk.year % 100 == 0) { // 世纪跳变，可能需要重新校准RTC
                  clock_recal_rtc();
                }
              }
            }
          }
        }
        clock_test(clk.day, clk.hour, clk.min);
      } 
    }
    atomic_flag_clear(&clock_lock);
  } else {
    clock_lost_ticks ++;
  }
  if(clk.ms19 % 32 == 0)
    clock_update_display();
}

void clock_set_display_mode(clock_display_mode_t mode)
{
  NEO_LOGD(TAG, "clock_set_display_mode %d", mode);
  clock_display_mode = mode;
  clock_date_index = 0;
  clock_update_cnt = 0;
  iv18_clr();
  clock_update_display();
}

void clock_show(void)
{
  terminal_printf("%04d-%02d-%02d  (%02d) %02d:%02d:%02d:%03d\r\n",
    clk.year, clk.mon + 1, clk.date + 1, clk.day + 1,
    clk.hour, clk.min, clk.sec, clk.ms19);
}

void clock_sync_to_local(void)
{
  struct timeval tv = {0};
  struct tm timeinfo = {0};

  clock_get_timeinfo(&timeinfo);

  tv.tv_sec = mktime(&timeinfo);
  tv.tv_usec = clk.ms19 * 1000000 / 512 ;

  settimeofday(&tv, NULL);
}

void clock_dump(void)
{
  uint16_t year;
  uint8_t mon, date, day, hour, min, sec, ms19, is_hour12;
  atomic_flag_test_and_set(&clock_lock);
  year = clk.year;
  mon  = clk.mon;
  date = clk.date;
  day  = clk.day;
  hour = clk.hour;
  min  = clk.min;
  sec  = clk.sec;
  ms19 = clk.ms19;
  is_hour12 = clk.is_hour12;
  atomic_flag_clear(&clock_lock);
  NEO_LOGD(TAG, "dump clock (%u-%02u-%02u [%02u] %02u:%02u:%02u:%04u):", year, mon + 1, date + 1, day, hour, min, sec, ms19);
  NEO_LOGD(TAG, "clk.year = %u", year);
  NEO_LOGD(TAG, "clk.mon  = %u", mon);
  NEO_LOGD(TAG, "clk.date = %u", date); 
  NEO_LOGD(TAG, "clk.day  = %u", day);
  NEO_LOGD(TAG, "clk.hour = %u", hour); 
  NEO_LOGD(TAG, "clk.min  = %u", min);
  NEO_LOGD(TAG, "clk.sec  = %u", sec);  
  NEO_LOGD(TAG, "clk.ms19 = %u", ms19); 
  NEO_LOGD(TAG, "clk.is_hour12 = %u", is_hour12); 
  NEO_LOGD(TAG, "clock_ntp_en = %u", clock_ntp_en); 
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
  atomic_flag_test_and_set(&clock_lock);
  clk.sec = sec % 60;
  atomic_flag_clear(&clock_lock);
}

void clock_clr_sec(void)
{
  
  atomic_flag_test_and_set(&clock_lock);
  clk.sec = 0;
  atomic_flag_clear(&clock_lock);
}

uint8_t clock_get_min(void)
{
  return clk.min;
}

void clock_set_min(uint8_t min)
{
  atomic_flag_test_and_set(&clock_lock);
  clk.min = min % 60;
  atomic_flag_clear(&clock_lock);
}

void clock_inc_min(bool fast)
{
  atomic_flag_test_and_set(&clock_lock);
  clk.min += fast ? CLOCK_FAST_STEP : 1;
  clk.min %= 60;
  atomic_flag_clear(&clock_lock);
}

void clock_dec_min(bool fast)
{
  atomic_flag_test_and_set(&clock_lock);
  clk.min += fast ? (60 - CLOCK_FAST_STEP) : 59;
  clk.min %= 60;
  atomic_flag_clear(&clock_lock);
}

uint8_t clock_get_hour(void)
{
  return clk.hour;
}

void clock_set_hour(uint8_t hour)
{
  atomic_flag_test_and_set(&clock_lock);
  clk.hour = hour % 24;
  atomic_flag_clear(&clock_lock);
}

void clock_inc_hour(bool fast)
{
  atomic_flag_test_and_set(&clock_lock);
  clk.hour += fast ? CLOCK_FAST_STEP : 1;
  clk.hour %= 24;
  atomic_flag_clear(&clock_lock);
}

void clock_dec_hour(bool fast)
{
  atomic_flag_test_and_set(&clock_lock);
  clk.hour += fast ? (24 - CLOCK_FAST_STEP) : 23;
  clk.hour %= 24;
  atomic_flag_clear(&clock_lock);
}


uint8_t clock_get_date(void)
{
  return clk.date + 1;
}


void clock_set_date(uint8_t date)
{
  if(date == 0)
    return;
  atomic_flag_test_and_set(&clock_lock);
  clk.date = (date - 1) % clock_get_mon_date(clk.year, clk.mon);
  clock_recal_date_day();
  atomic_flag_clear(&clock_lock);
}

void clock_inc_date(bool fast)
{
  atomic_flag_test_and_set(&clock_lock);
  clk.date += fast ? CLOCK_FAST_STEP : 1;
  clk.date %= clock_get_mon_date(clk.year, clk.mon);
  clock_recal_date_day();
  atomic_flag_clear(&clock_lock);
}

void clock_dec_date(bool fast)
{
  atomic_flag_test_and_set(&clock_lock);
  clk.date += clock_get_mon_date(clk.year, clk.mon) - (fast ? CLOCK_FAST_STEP : 1);
  clk.date %= clock_get_mon_date(clk.year, clk.mon);
  clock_recal_date_day();
  atomic_flag_clear(&clock_lock);
}

uint8_t clock_get_day(void)
{
  return clk.day;
}

uint8_t clock_get_month(void)
{
  return clk.mon + 1;
}


void clock_set_month(uint8_t mon)
{
  if(mon == 0)
    return;
  atomic_flag_test_and_set(&clock_lock);
  clk.mon = (mon - 1) % 12;
  clock_recal_date_day();
  atomic_flag_clear(&clock_lock);
}

void clock_inc_month(void)
{
  atomic_flag_test_and_set(&clock_lock);
  ++ clk.mon;
  clk.mon %= 12;
  clock_recal_date_day();
  atomic_flag_clear(&clock_lock);
}

void clock_dec_month(void)
{
  atomic_flag_test_and_set(&clock_lock);
  clk.mon += 11;
  clk.mon %= 12;
  clock_recal_date_day();
  atomic_flag_clear(&clock_lock);
}

uint16_t clock_get_year(void)
{
  return clk.year;
}

void clock_set_year(uint16_t year)
{
  if(year >= 1901 && year <= 2099) {
    atomic_flag_test_and_set(&clock_lock);
    clk.year = cext_limit(year, CLOCK_MIN_YEAR, CLOCK_MAX_YEAR);
    clock_recal_date_day();
    atomic_flag_clear(&clock_lock);
  }
}

void clock_inc_year(bool fast)
{
  atomic_flag_test_and_set(&clock_lock);
  clk.year = cext_ring_add(clk.year, (fast ? CLOCK_FAST_STEP : 1), CLOCK_MIN_YEAR, CLOCK_MAX_YEAR);
  clock_recal_date_day();
  atomic_flag_clear(&clock_lock);
}

void clock_dec_year(bool fast)
{
  atomic_flag_test_and_set(&clock_lock);
  clk.year = cext_ring_sub(clk.year, (fast ? CLOCK_FAST_STEP : 1), CLOCK_MIN_YEAR, CLOCK_MAX_YEAR);
  clock_recal_date_day();
  atomic_flag_clear(&clock_lock);
}

void clock_sync_from_rtc(clock_sync_type_t type)
{
  uint8_t year;
  uint8_t centry;
  uint8_t hour, min, sec, mon, date, day;
  NEO_LOGI(TAG, "clock_sync_from_rtc = %s", 
    type == CLOCK_SYNC_TIME ? "time" : "date");
  if(type == CLOCK_SYNC_TIME) {
    ds3231_rtc_get_time(&hour, &min, &sec);
    atomic_flag_test_and_set(&clock_lock);
    clk.ms19 = 255;   // 0 - 255
    clk.hour = hour;
    clk.min = min;
    clk.sec = sec;
    atomic_flag_clear(&clock_lock);
    NEO_LOGI(TAG, "clock_sync_from_rtc %02u:%02u:%02u:%02u", hour, min, sec);
  } else if(type == CLOCK_SYNC_DATE) {
    centry = config_read_int("century");
    clock_saved_century = centry;
    ds3231_rtc_get_date(&year, &mon, &date, &day);
    atomic_flag_test_and_set(&clock_lock);
    clk.mon  = mon - 1; // rtc是1-12，clock是0-11 
    clk.date = date - 1; // rtc是1-31，clock是0-30
    clk.day  = day - 1;  // rtc是1-7，clock是0-6
    clk.year = centry * 100 + year;
    atomic_flag_clear(&clock_lock);
    NEO_LOGI(TAG, "clock_sync_from_rtc (%02u)%2u-%02u-%02u-%02u", centry, year, 
      clk.mon + 1, clk.date + 1, clk.day + 1);
  }
}

void clock_sync_to_rtc(clock_sync_type_t type)
{
  uint8_t hour, min, sec, mon, date, day;
  uint16_t year;

  NEO_LOGI(TAG, "clock_sync_to_rtc = %s", 
    type == CLOCK_SYNC_TIME ? "time" : "date");
  if(type == CLOCK_SYNC_TIME) {
    atomic_flag_test_and_set(&clock_lock);
    hour = clk.hour;
    min  = clk.min;
    sec  = clk.sec;
    atomic_flag_clear(&clock_lock);
    ds3231_rtc_set_time(hour, min, sec);
    NEO_LOGI(TAG, "clock_sync_to_rtc %02u:%02u:%02u:%02u", hour, min, sec);
  } else if(type == CLOCK_SYNC_DATE) {
    atomic_flag_test_and_set(&clock_lock);
    mon  = clk.mon + 1;
    date = clk.date + 1;
    day  = clk.day + 1;
    year = clk.year;
    atomic_flag_clear(&clock_lock);
    NEO_LOGI(TAG, "clock_sync_to_rtc (%02u)%2u-%02u-%02u %02u", year / 100, year % 100, mon, date, day);
    ds3231_rtc_set_date(0, year % 100, mon, date, day);
    if(clock_saved_century != year / 100) {
      config_write_int("century", year / 100);
      clock_saved_century = year / 100;
    }
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

// 每1/512秒 触发一次中断，在中断里更新时钟&timer，刷新iv18
static void IRAM_ATTR clock_isr_handler (void* param)
{
  clock_inc_ms19();
  timer_inc_ms19();
  iv18_scan(); 
}

void clock_init(void)
{

  NEO_LOGI(TAG, "init");

  clock_saved_century = 0;

  if(ec11_is_factory_reset()) {
    NEO_LOGI(TAG, "clock factory reset time");

    ds3231_rtc_set_time(
      CLOCK_FACTORY_RESET_HOUR,
      CLOCK_FACTORY_RESET_MIN,
      CLOCK_FACTORY_RESET_SEC);

    NEO_LOGI(TAG, "clock factory reset date");
    ds3231_rtc_set_date(
      0,
      CLOCK_FACTORY_RESET_YEAR,
      CLOCK_FACTORY_RESET_MON,
      CLOCK_FACTORY_RESET_DATE,
      CLOCK_FACTORY_RESET_DAY);
  }
  // 初始化时钟
  clock_sync_from_rtc(CLOCK_SYNC_TIME);
  clock_sync_from_rtc(CLOCK_SYNC_DATE); 
  clock_enable_interrupt(true);
  clk.is_hour12 = config_read_int("time_12");

  clock_ntp_en  = config_read_int("clock_ntp_en");

  ESP_ERROR_CHECK(gpio_intr_enable(DS3231_CLK_GPIO_PIN));
  ESP_ERROR_CHECK(gpio_set_intr_type(DS3231_CLK_GPIO_PIN, GPIO_INTR_NEGEDGE));
  ESP_ERROR_CHECK(gpio_isr_handler_add(DS3231_CLK_GPIO_PIN, clock_isr_handler, NULL));

  clock_display_mode = CLOCK_DISPLAY_MODE_DISABLE;

  clock_date_index = 0;

  clock_lost_ticks = 0;

  clock_sync_to_local();

  clock_dump();
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

bool clock_test_ntp(void)
{
  return clock_ntp_en;
}

void clock_set_ntp(bool enable)
{
  clock_ntp_en = enable;
}