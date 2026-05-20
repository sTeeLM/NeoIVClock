#ifndef NEO_IV_CLOCK_SM_CLOCK_H
#define NEO_IV_CLOCK_SM_CLOCK_H

#include "task.h"
#include "sm.h"

extern const char * sm_states_names_clock[];
extern sm_trans_t * sm_trans_clock[];

enum sm_states_clock
{
  SM_CLOCK_INIT,  
  SM_CLOCK_TIME_A, // IV显示时间，OLED显示气温、气压、湿度
  SM_CLOCK_TIME_B, // IV显示时间，OLED显示TVOC、甲醛、PM2.5  
  SM_CLOCK_DATE_A, // IV显示日期，OLED显示气温、气压、湿度
  SM_CLOCK_DATE_B  // IV显示日期，OLED显示TVOC、甲醛、PM2.5 
};

void do_clock_init(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev);

#endif // NEO_IV_CLOCK_SM_CLOCK_H