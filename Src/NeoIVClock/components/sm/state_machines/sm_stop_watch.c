#include "sm_stop_watch.h"
#include "task.h"
#include "sm.h"
#include "logger.h"

const char * sm_states_names_stop_watch[] = {
  "SM_STOP_WATCH_INIT",
};

void do_stop_watch_init(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{

}

static const sm_trans_t sm_trans_stop_watch_init[] = {
      {0, 0, 0, NULL}
};

const sm_trans_t * sm_trans_stop_watch[] = {
  sm_trans_stop_watch_init
};
