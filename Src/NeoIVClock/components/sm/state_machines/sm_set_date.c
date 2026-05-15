#include "sm_set_date.h"
#include "task.h"
#include "sm.h"
#include "logger.h"

const char * sm_states_names_set_date[] = {
  "SM_SET_DATE_INIT",
};

static sm_trans_t sm_trans_set_date_init[] = {
      {0, 0, 0, NULL}
};

sm_trans_t * sm_trans_set_date[] = {
  sm_trans_set_date_init
};
