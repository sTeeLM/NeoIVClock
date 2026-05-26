#ifndef NEO_IV_CLOCK_TIMER_H
#define NEO_IV_CLOCK_TIMER_H

#include <stdint.h>
#include <stdbool.h>

void timer_init(void);
uint8_t timer_get_snd(void);
uint8_t timer_inc_snd(void);
uint8_t timer_dec_snd(void);
void timer_save_config(void);

#endif  // NEO_IV_CLOCK_TIMER_H
