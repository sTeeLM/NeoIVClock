#include "sm_set_time.h"
#include "task.h"
#include "sm.h"
#include "logger.h"

const char * sm_states_names_set_time[] = {
  "SM_SET_TIME_INIT",
};

static sm_trans_t sm_trans_set_time_init[] = {
      {0, 0, 0, NULL}
};

sm_trans_t * sm_trans_set_time[] = {
  sm_trans_set_time_init
};
