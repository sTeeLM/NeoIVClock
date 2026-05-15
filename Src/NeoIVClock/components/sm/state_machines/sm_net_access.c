#include "sm_net_access.h"
#include "task.h"
#include "sm.h"
#include "logger.h"

const char * sm_states_names_net_access[] = {
  "SM_NET_ACCESS_INIT",
};

static sm_trans_t sm_trans_net_access_init[] = {
      {0, 0, 0, NULL}
};

sm_trans_t * sm_trans_net_access[] = {
  sm_trans_net_access_init
};
