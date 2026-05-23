#include "sm_set_param.h"
#include "task.h"
#include "sm.h"
#include "logger.h"

const char * sm_states_names_set_param[] = {
  "SM_SET_PRARM_INIT",
};

void do_set_param_init(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{

}

static const sm_trans_t sm_trans_set_param_init[] = {
      {0, 0, 0, NULL}
};

const sm_trans_t * sm_trans_set_param[] = {
  sm_trans_set_param_init
};
