#ifndef NEO_IV_CLOCK_SM_STOP_WATCH_H
#define NEO_IV_CLOCK_SM_STOP_WATCH_H

#include "task.h"
#include "sm.h"

extern const char * sm_states_names_stop_watch[];
extern sm_trans_t * sm_trans_stop_watch[];

enum sm_states_stop_watch
{
  SM_STOP_WATCH_INIT, 
};

void do_stop_watch_init(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev);

#endif // NEO_IV_CLOCK_SM_STOP_WATCH_H