#include "sm_clock.h"
#include "task.h"
#include "sm.h"
#include "logger.h"

static const char * TAG = "SM_CLOCK";

const char * sm_states_names_clock[] = {
  "SM_CLOCK_INIT",
};

void do_clock_init(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  task_set_ipc(EV_ALARM0);
}

static sm_trans_t sm_trans_clock_init[] = {
  {EV_1S, SM_CLOCK , SM_CLOCK_INIT, do_clock_init},
  {0, 0, 0, NULL}
};

sm_trans_t * sm_trans_clock[] = {
  sm_trans_clock_init
};
