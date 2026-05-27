#ifndef NEO_IV_CLOCK_SM_STOP_WATCH_H
#define NEO_IV_CLOCK_SM_STOP_WATCH_H

#include "task.h"
#include "sm.h"

extern const char * sm_states_names_stop_watch[];
extern const sm_trans_t * sm_trans_stop_watch[];

enum sm_states_stop_watch
{
  SM_STOP_WATCH_INIT,
  SM_STOP_WATCH_STOP,  // 停止状态（清零）
  SM_STOP_WATCH_RUN,   // 运行状态
  SM_STOP_WATCH_PAUSE  // 暂停状态
};

void do_stop_watch_init(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev);

#endif // NEO_IV_CLOCK_SM_STOP_WATCH_H