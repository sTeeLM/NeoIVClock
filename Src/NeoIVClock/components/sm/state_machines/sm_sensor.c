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
  // 初始化
}

static void do_sensor_poll(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  // 定期轮询传感器，并更新传感器数据
}

static sm_trans_t sm_trans_sensor_init[] = {
  {EV_EC11_UP, SM_SENSOR, SM_SENSOR_POLL, do_sensor_init},
  {EV_1S, SM_SENSOR, SM_SENSOR_POLL, do_sensor_poll},
  {0, 0, 0, NULL}
};

static sm_trans_t sm_trans_sensor_poll[] = {
  {EV_1S, SM_SENSOR, SM_SENSOR_POLL, do_sensor_poll},
  {0, 0, 0, NULL}
};

sm_trans_t * sm_trans_sensor[] = {
  sm_trans_sensor_init,
  sm_trans_sensor_poll
};
