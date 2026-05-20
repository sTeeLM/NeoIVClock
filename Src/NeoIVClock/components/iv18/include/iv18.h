#ifndef NEO_IV_CLOCK_IV18_H
#define NEO_IV_CLOCK_IV18_H

#include "task.h"
#include <stdint.h>
#include <stdbool.h>

void iv18_init(void);

// 特殊字符
#define IV18_BLANK   0
#define IV18_DEGREE  1

#define IV18_PROGRESS_BASE 2
#define IV18_PROGRESS0  2
#define IV18_PROGRESS1  3
#define IV18_PROGRESS2  4
#define IV18_PROGRESS3  5
#define IV18_PROGRESS4  6
#define IV18_PROGRESS5  7
#define IV18_PROGRESS6  8
#define IV18_PROGRESS7  9
#define IV18_PROGRESS_CNT 8

#define IV18_SPECIAL_MAX  9

void iv18_clr(void);
void iv18_load_data(uint8_t index);
void iv18_clr_data(uint8_t index);
void iv18_set_dig(uint8_t index, uint8_t ascii);
void iv18_set_dp(uint8_t index);
void iv18_clr_dp(uint8_t index);
void iv18_set_blink(uint8_t index);
void iv18_clr_blink(uint8_t index);
void iv18_scan(void);
void iv18_enable(bool enable);
void iv18_set_brightness(uint8_t brightness);

void iv18_proc(task_event_t ev);

#endif // NEO_IV_CLOCK_IV18_H