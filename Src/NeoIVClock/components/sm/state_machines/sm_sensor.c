#include "sm_sensor.h"
#include "task.h"
#include "sm.h"
#include "logger.h"

static const char * TAG = "SM_SSENSOR";

const char * sm_states_names_sensor[] = {
  "SM_SENSOR_INIT",
};

void do_sensor_init(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{

}

static sm_trans_t sm_trans_sensor_init[] = {
  {EV_1S, SM_SENSOR, SM_SENSOR_INIT, do_sensor_init},
  {0, 0, 0, NULL}
};

sm_trans_t * sm_trans_sensor[] = {
  sm_trans_sensor_init
};
