#include "sm_set_net.h"
#include "task.h"
#include "sm.h"
#include "logger.h"

const char * sm_states_names_set_net[] = {
  "SM_SET_NET_INIT",
};

void do_set_net_init(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{

}

static sm_trans_t sm_trans_set_net_init[] = {
      {0, 0, 0, NULL}
};

sm_trans_t * sm_trans_set_net[] = {
  sm_trans_set_net_init
};
