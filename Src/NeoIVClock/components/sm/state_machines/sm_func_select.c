#include "sm_func_select.h"
#include "task.h"
#include "sm.h"
#include "logger.h"

const char * sm_states_names_func_select[] = {
  "SM_FUNC_SELECT_INIT",
};

static sm_trans_t sm_trans_func_select_init[] = {
      {0, 0, 0, NULL}
};

sm_trans_t * sm_trans_func_select[] = {
  sm_trans_func_select_init
};
