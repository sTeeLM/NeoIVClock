#ifndef NEO_IV_CLOCK_SM_FUNC_SELECT_H
#define NEO_IV_CLOCK_SM_FUNC_SELECT_H

#include "task.h"
#include "sm.h"

extern const char * sm_states_names_func_select[];
extern const sm_trans_t * sm_trans_func_select[];

enum sm_states_func_select
{
  SM_FUNC_SELECT_INIT, 
  SM_FUNC_SELECT_CLOCK,
  SM_FUNC_SELECT_ALARM,
  SM_FUNC_SELECT_SET_TIME,
  SM_FUNC_SELECT_SET_DATE,
  SM_FUNC_SELECT_SET_PARAM,
  SM_FUNC_SELECT_SET_NET,
  SM_FUNC_SELECT_TIMER,
  SM_FUNC_SELECT_STOP_WATCH,
  SM_FUNC_SELECT_ABOUT
};

void do_func_select_init(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev);

#endif // NEO_IV_CLOCK_SM_FUNC_SELECT_H