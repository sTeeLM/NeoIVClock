#ifndef NEO_IV_CLOCK_SM_SET_DATE_H
#define NEO_IV_CLOCK_SM_SET_DATE_H

#include "task.h"
#include "sm.h"

extern const char * sm_states_names_set_date[];
extern const sm_trans_t * sm_trans_set_date[];

enum sm_states_set_date
{
  SM_SET_DATE_INIT, 
};

void do_set_date_init(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev);

#endif // NEO_IV_CLOCK_SM_SET_DATE_H