#ifndef NEO_IV_CLOCK_CLOCK_H
#define NEO_IV_CLOCK_CLOCK_H

#include <stdint.h>
#include <stdbool.h>

#include "task.h"

void clock_init (void);
void clock_enter_powersave(void);
void clock_leave_powersave(void);
void clock_enter_console(void);
void clock_leave_console(void);

typedef enum _clock_sync_type_t
{
  CLOCK_SYNC_TIME = 0,
  CLOCK_SYNC_DATE = 1
} clock_sync_type_t;

typedef struct _clock_struct_t
{
  uint16_t year;  // 1901 ~ 2099?
  uint8_t mon;    // 0 - 11
  uint8_t date;   // 0 - 30(29/28/27)
  uint8_t day;    // 0 - 6
  uint8_t hour;   // 0 - 23
  uint8_t min;    // 0 - 59
  uint8_t sec;    // 0 - 59
  uint16_t ms19;   // 0 - 511
  uint8_t is_hour12; // 0: 24-hour mode, 1: 12-hour mode
} clock_struct_t;

extern clock_struct_t clk;

void clock_show(void);

uint8_t clock_get_sec(void);
void clock_set_sec(uint8_t sec);
void clock_clr_sec(void);
uint8_t clock_get_min(void);
void clock_set_min(uint8_t min);
void clock_inc_min(bool fast);
void clock_dec_min(bool fast);
uint8_t clock_get_hour(void);
void clock_set_hour(uint8_t hour);
void clock_inc_hour(bool fast);
void clock_dec_hour(bool fast);
uint8_t clock_get_date(void);
void clock_set_date(uint8_t date);
void clock_inc_date(bool fast);
void clock_dec_date(bool fast);
uint8_t clock_get_day(void);
uint8_t clock_get_month(void);
void clock_set_month(uint8_t month);
void clock_inc_month(void);
void clock_dec_month(void);
uint16_t clock_get_year(void);
void clock_set_year(uint16_t year);
void clock_inc_year(bool fast);
void clock_dec_year(bool fast);
void clock_sync_from_rtc(clock_sync_type_t type);
void clock_sync_to_rtc(clock_sync_type_t type);
void clock_dump(void);
uint16_t clock_get_ms19(void);
void clock_inc_ms19(void);

void clock_enable_interrupt(bool enable);
  
bool clock_is_leap_year(uint16_t year); // year 1901~2099
uint8_t clock_get_mon_date(uint16_t year, uint8_t mon); // mon 0-11

uint32_t clock_get_now_sec(void);
uint32_t clock_diff_now_sec(uint32_t sec);

bool clock_test_hour12(void);
void clock_set_hour12(bool enable);

bool clock_test_ntp(void);
void clock_set_ntp(bool enable);

void clock_save_config(void);

void clock_recal_rtc_proc(task_event_t ev);
void clock_time_sync_proc(task_event_t ev);

typedef enum _clock_display_mode_t
{
  CLOCK_DISPLAY_MODE_DISABLE = 0,
  CLOCK_DISPLAY_MODE_TIME,         // HH-MM-SS，用来设置和显示
  CLOCK_DISPLAY_MODE_DATE,         // YYYY-MM-DD-D滚动，用来显示
  CLOCK_DISPLAY_MODE_COMPOUND_DATE // YYYYMMDD，用来设置
} clock_display_mode_t;

void clock_set_display_mode(clock_display_mode_t mode);

#endif // NEO_IV_CLOCK_CLOCK_H
