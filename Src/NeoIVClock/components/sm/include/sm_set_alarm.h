#ifndef NEO_IV_CLOCK_SM_SET_ALARM_H
#define NEO_IV_CLOCK_SM_SET_ALARM_H

#include "task.h"
#include "sm.h"

extern const char * sm_states_names_set_alarm[];
extern const sm_trans_t * sm_trans_set_alarm[];

enum sm_states_set_alarm
{
  SM_SET_ALARM_INIT,
  SM_SET_ALARM_SEL_TYPE,        // 普通闹钟还是整点报时
  SM_SET_ALARM_SEL_ALARM1,      // 选择普通闹钟的第几个？
  SM_SET_ALARM_SEL_ALARM1_TYPE, // 设置普通闹钟的啥？
  SM_SET_ALARM_SET_ALARM1_EN,   // 普通闹钟设置on/off
  SM_SET_ALARM_SET_ALARM1_HOUR, // 普通闹钟设置小时
  SM_SET_ALARM_SET_ALARM1_MIN,  // 普通闹钟设置分
  SM_SET_ALARM_SET_ALARM1_DAY,  // 普通闹钟设置重复周期
  SM_SET_ALARM_SET_ALARM1_SND,  // 闹钟音乐
  SM_SET_ALARM_SEL_ALARM0_TYPE,       // 整点报时调整begin还是end
  SM_SET_ALARM_SET_ALARM0_BEGIN,
  SM_SET_ALARM_SET_ALARM0_END
};

void do_set_alarm_init(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev);

#endif // NEO_IV_CLOCK_SM_SET_ALARM_H