#ifndef NEO_IV_CLOCK_ALARM_H
#define NEO_IV_CLOCK_ALARM_H

#include <stdint.h>
#include <stdbool.h>

#define ALARM1_MAX_COUNT 10

// 整点报时闹钟
typedef struct _alarm0_t{
  uint8_t enabled; // 0: disabled, 1: enabled
} alarm0_t;

// 周期闹钟
typedef struct _alarm1_t{
  uint8_t enabled;     // 0: disabled, 1: enabled
  uint8_t day_mask;    // bit0->星期一，bit6->星期日
  uint8_t hour;        // 0~23
  uint8_t minute;      // 0~59
  uint8_t snd;         // 0~9, 0表示默认铃声
} alarm1_t;

void alarm_init(void);
void alarm_test(uint8_t day, uint8_t hour, uint8_t minute, uint8_t sec);

bool alarm1_get_enabled(uint8_t alarm1_index);
bool alarm1_enable(uint8_t alarm1_index, bool enable);

uint8_t alarm1_get_hour(uint8_t alarm1_index);
uint8_t alarm1_inc_hour(uint8_t alarm1_index, bool fast);
uint8_t alarm1_dec_hour(uint8_t alarm1_index, bool fast);

uint8_t alarm1_get_min(uint8_t alarm1_index, bool fast);
uint8_t alarm1_inc_min(uint8_t alarm1_index, bool fast);
uint8_t alarm1_dec_min(uint8_t alarm1_index, bool fast);

uint8_t alarm1_get_day_mask(uint8_t alarm1_index);
uint8_t alarm1_set_day_mask(uint8_t alarm1_index, uint8_t day_mask);

uint8_t alarm1_get_snd(uint8_t alarm1_index);
uint8_t alarm1_inc_snd(uint8_t alarm1_index, bool fast);
uint8_t alarm1_dec_snd(uint8_t alarm1_index, bool fast);

void alarm1_save_config(uint8_t alarm1_index);

bool alarm0_get_enabled(void);
bool alarm0_enable(bool enable);
void alarm0_save_config(void);

#endif // NEO_IV_CLOCK_ALARM_H
