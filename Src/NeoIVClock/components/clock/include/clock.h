#ifndef NEO_IV_CLOCK_CLOCK_H
#define NEO_IV_CLOCK_CLOCK_H

#include <stdint.h>
#include <stdbool.h>

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
  uint16_t year;  // 1901 ~ 2099
  uint8_t mon;    // 0 - 11
  uint8_t date;   // 0 - 30(29/28/27)
  uint8_t day;    // 0 - 6
  uint8_t hour;   // 0 - 23
  uint8_t min;    // 0 - 59
  uint8_t sec;    // 0 - 59
  uint8_t ms39;   // 0 - 255
} clock_struct_t;

extern clock_struct_t clk;

void clock_show(void);

uint8_t clock_get_sec(void);
void clock_set_sec(uint8_t sec);
uint8_t clock_get_sec_256(void);
void clock_clr_sec(void);
uint8_t clock_get_min(void);
void clock_set_min(uint8_t min);
void clock_inc_min(void);
uint8_t clock_get_hour(void);
void clock_set_hour(uint8_t hour);
void clock_inc_hour(void);
uint8_t clock_get_date(void);
void clock_set_date(uint8_t date);
void clock_inc_date(void);
uint8_t clock_get_day(void);
uint8_t clock_get_month(void);
void clock_set_month(uint8_t month);
void clock_inc_month(void);
uint16_t clock_get_year(void);
void clock_set_year(uint16_t year);
void clock_inc_year(void);
void clock_sync_from_rtc(clock_sync_type_t type);
void clock_sync_to_rtc(clock_sync_type_t type);
void clock_dump(void);
uint8_t clock_get_ms39(void);
void clock_inc_ms39(void);

void clock_enable_interrupt(bool enable);
  
bool clock_is_leap_year(uint16_t year); // year 1901~2099
uint8_t clock_get_mon_date(uint16_t year, uint8_t mon); // mon 0-11

uint32_t clock_get_now_sec(void);
uint32_t clock_diff_now_sec(uint32_t sec);

bool clock_test_hour12(void);
void clock_set_hour12(bool enable);
void clock_save_config(void);

#endif // NEO_IV_CLOCK_CLOCK_H
