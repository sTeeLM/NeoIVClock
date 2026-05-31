#ifndef NEO_IV_CLOCK_SM_ABOUT_H
#define NEO_IV_CLOCK_SM_ABOUT_H

#include "task.h"
#include "sm.h"

extern const char * sm_states_names_about[];
extern const sm_trans_t * sm_trans_about[];

enum sm_states_about
{
  SM_ABOUT_INIT,  
  SM_ABOUT_SHOW
};

void do_about_init(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev);

#endif // NEO_IV_CLOCK_SM_ABOUT_H