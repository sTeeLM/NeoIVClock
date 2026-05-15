#include "sm_stop_watch.h"
#include "task.h"
#include "sm.h"
#include "logger.h"

const char * sm_states_names_stop_watch[] = {
  "SM_STOP_WATCH_INIT",
};

static sm_trans_t sm_trans_stop_watch_init[] = {
      {0, 0, 0, NULL}
};

sm_trans_t * sm_trans_stop_watch[] = {
  sm_trans_stop_watch_init
};
