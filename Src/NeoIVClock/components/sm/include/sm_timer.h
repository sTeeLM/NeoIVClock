#ifndef NEO_IV_CLOCK_SM_TIMER_H
#define NEO_IV_CLOCK_SM_TIMER_H

#include "task.h"
#include "sm.h"

extern const char * sm_states_names_timer[];
extern const sm_trans_t * sm_trans_timer[];

enum sm_states_timer
{
  SM_TIMER_INIT, 
  SM_TIMER_SEL,      // 选择timer时间，或者自行设置
  SM_TIMER_SET_SEL,  // 选择设置哪个字段
  SM_TIMER_SET_HOUR, // 设置小时
  SM_TIMER_SET_MIN,  // 设置分钟
  SM_TIMER_SET_SEC,  // 设置秒
  SM_TIMER_RUN,      // 倒计时运行
  SM_TIMER_PAUSE,    // 倒计时暂停
  SM_TIMER_STOP,     // 倒计时结束
};

void do_timer_init(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev);

#endif // NEO_IV_CLOCK_SM_TIMER_H