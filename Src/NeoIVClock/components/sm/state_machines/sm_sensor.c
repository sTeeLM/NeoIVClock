#include "sm_sensor.h"
#include "task.h"
#include "sm.h"
#include "logger.h"

const char * sm_states_names_sensor[] = {
  "SM_SENSOR_INIT",
};

static sm_trans_t sm_trans_sensor_init[] = {
      {0, 0, 0, NULL}
};

sm_trans_t * sm_trans_sensor[] = {
  sm_trans_sensor_init
};
