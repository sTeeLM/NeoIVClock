#ifndef NEO_IV_CLOCK_TIMER_H
#define NEO_IV_CLOCK_TIMER_H

#include <stdint.h>
#include <stdbool.h>


void timer_inc_ms19(void);

typedef enum _timer_mode_t {
  TIMER_MODE_INC = 0,
  TIMER_MODE_DEC = 1,  
}timer_mode_t;

typedef struct _timer_struct_t {
  uint8_t  hour;
  uint8_t  min;
  uint8_t  sec;
  uint32_t ms19;
} timer_struct_t;

void timer_display_enable(bool enable);
void timer_refresh_display(uint8_t slot);

uint8_t timer_next_history(void);
void timer_rwind_history(void);

void timer_set_mode(timer_mode_t mode);
void timer_start(void);
uint8_t timer_save(void);
uint8_t timer_get_slot_cnt(void);

void timer_set(uint8_t hour, uint8_t min, uint8_t sec);
uint8_t timer_get_hour(uint8_t slot);
uint8_t timer_inc_hour(bool fast);
uint8_t timer_dec_hour(bool fast);
uint8_t timer_get_min(uint8_t slot);
uint8_t timer_inc_min(bool fast);
uint8_t timer_dec_min(bool fast);
uint8_t timer_get_sec(uint8_t slot);
uint8_t timer_inc_sec(bool fast);
uint8_t timer_dec_sec(bool fast);

void timer_stop(void);
void timer_resume(void);
void timer_clr(void);


void timer_init(void);
uint8_t timer_get_snd(void);
uint8_t timer_inc_snd(void);
uint8_t timer_dec_snd(void);
void timer_save_config(void);

#endif  // NEO_IV_CLOCK_TIMER_H
