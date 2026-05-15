#include "sm_clock.h"
#include "task.h"
#include "sm.h"
#include "logger.h"

const char * sm_states_names_clock[] = {
  "SM_CLOCK_INIT",
};

static sm_trans_t sm_trans_clock_init[] = {
      {0, 0, 0, NULL}
};

sm_trans_t * sm_trans_clock[] = {
  sm_trans_clock_init
};
