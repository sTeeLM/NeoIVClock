#ifndef NEO_IV_CLOCK_DS3231_RTC_H
#define NEO_IV_CLOCK_DS3231_RTC_H

#include <stdint.h>
#include <stdbool.h>

void ds3231_rtc_init(void);
void ds3231_rtc_get_time(uint8_t * hour, uint8_t * min, uint8_t * sec);
bool ds3231_rtc_get_date(uint8_t * year, uint8_t * mon, uint8_t * date, uint8_t * day);
void ds3231_rtc_set_time(uint8_t hour, uint8_t min, uint8_t sec);
void ds3231_rtc_set_date(bool centry, uint8_t year, uint8_t mon, uint8_t date, uint8_t day);
void ds3231_rtc_enable_32768HZ(bool enable);

#endif  // NEO_IV_CLOCK_DS3231_RTC_H
