#ifndef NEO_IV_CLOCK_SM_TIMER_H
#define NEO_IV_CLOCK_SM_TIMER_H

#include "task.h"
#include "sm.h"

extern const char * sm_states_names_timer[];
extern const sm_trans_t * sm_trans_timer[];

enum sm_states_timer
{
  SM_TIMER_INIT, 
};

void do_timer_init(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev);

#endif // NEO_IV_CLOCK_SM_TIMER_H