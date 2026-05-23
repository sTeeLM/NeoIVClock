#include "sm_set_time.h"
#include "task.h"
#include "sm.h"
#include "logger.h"

const char * sm_states_names_set_time[] = {
  "SM_SET_TIME_INIT",
};

void do_set_time_init(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{

}

static const sm_trans_t sm_trans_set_time_init[] = {
      {0, 0, 0, NULL}
};

const sm_trans_t * sm_trans_set_time[] = {
  sm_trans_set_time_init
};
