#include "sm_timer.h"
#include "task.h"
#include "sm.h"
#include "logger.h"

const char * sm_states_names_timer[] = {
  "SM_TIMER_INIT",
};

static sm_trans_t sm_trans_timer_init[] = {
      {0, 0, 0, NULL}
};

sm_trans_t * sm_trans_timer[] = {
  sm_trans_timer_init
};
