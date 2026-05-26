#ifndef NEO_IV_CLOCK_SM_SET_PARAM_H
#define NEO_IV_CLOCK_SM_SET_PARAM_H

#include "task.h"
#include "sm.h"

extern const char * sm_states_names_set_param[];
extern const sm_trans_t * sm_trans_set_param[];

enum sm_states_set_param
{
  SM_SET_PARAM_INIT, 
  SM_SET_PARAM_SET_SWITCH,  // 所有简单离散值的设置
  SM_SET_PARAM_IV18_BRIGHT, // IV18亮度
  SM_SET_PARAM_PLY_VOL,     // 播放器音量
  SM_SET_PARAM_TIMER_SND,   // timer音效
  SM_SET_PARAM_OLED_CONTRAST // OLED对比度 
};

void do_set_param_init(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev);

#endif // NEO_IV_CLOCK_SM_SET_PARAM_H