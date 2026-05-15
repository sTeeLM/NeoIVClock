#include "sm_set_alarm.h"
#include "task.h"
#include "sm.h"
#include "logger.h"

const char * sm_states_names_set_alarm[] = {
  "SM_SET_ALARM_INIT",
};

static sm_trans_t sm_trans_set_alarm_init[] = {
      {0, 0, 0, NULL}
};

sm_trans_t * sm_trans_set_alarm[] = {
  sm_trans_set_alarm_init
};
