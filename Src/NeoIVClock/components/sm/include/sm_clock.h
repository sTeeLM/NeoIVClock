#ifndef NEO_IV_CLOCK_SM_CLOCK_H
#define NEO_IV_CLOCK_SM_CLOCK_H

#include "task.h"
#include "sm.h"

extern const char * sm_states_names_clock[];
extern sm_trans_t * sm_trans_clock[];

enum sm_states_clock
{
  SM_CLOCK_INIT,  // 进入
  SM_CLOCK_TIME,  // 时间显示：iv显示时间，oled显示气温、湿度、气压、pm2.5、tvol
  SM_CLOCK_DATE   // 日期显示：iv显示日期，oled显示气温、湿度、气压、pm2.5、tvol
};

void do_clock_init(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev);

#endif // NEO_IV_CLOCK_SM_CLOCK_H