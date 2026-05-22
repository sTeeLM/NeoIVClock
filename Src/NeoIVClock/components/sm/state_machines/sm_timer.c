#include "sm_timer.h"
#include "task.h"
#include "sm.h"
#include "logger.h"

const char * sm_states_names_timer[] = {
  "SM_TIMER_INIT",
};

void do_timer_init(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  
}

static sm_trans_t sm_trans_timer_init[] = {
      {0, 0, 0, NULL}
};

sm_trans_t * sm_trans_timer[] = {
  sm_trans_timer_init
};
