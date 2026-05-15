#include "sm_set_param.h"
#include "task.h"
#include "sm.h"
#include "logger.h"

const char * sm_states_names_set_param[] = {
  "SM_SET_PRARM_INIT",
};

static sm_trans_t sm_trans_set_param_init[] = {
      {0, 0, 0, NULL}
};

sm_trans_t * sm_trans_set_param[] = {
  sm_trans_set_param_init
};
