#ifndef NEO_IV_CLOCK_ALARM_H
#define NEO_IV_CLOCK_ALARM_H

#include <stdint.h>

// 整点报时闹钟
typedef struct _alarm0_t{
  uint8_t enabled; // 0: disabled, 1: enabled
} alarm0_t;

// 周期闹钟
typedef struct _alarm1_t{
  uint8_t enabled;     // 0: disabled, 1: enabled
  uint8_t day_of_week; // 1~7 (Sunday = 7)
  uint8_t hour;        // 0~23
  uint8_t minute;      // 0~59
  uint8_t snd_index;   // 0~9, 0表示默认铃声
} alarm1_t;

void alarm_init(void);
void alarm_test(uint8_t day, uint8_t hour, uint8_t minute);

#endif // NEO_IV_CLOCK_ALARM_H
