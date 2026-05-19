#include "sm_sensor.h"
#include "task.h"
#include "sm.h"
#include "logger.h"

static const char * TAG = "SM_SSENSOR";

const char * sm_states_names_sensor[] = {
  "SM_SENSOR_INIT",
  "SM_SENSOR_POLL_PMS_ON",
  "SM_SENSOR_POLL_PMS_OFF"
};

// 固定一秒采样一次
// PMS每1小时激活10分钟
// 数据上报间隔，根据配置

void do_sensor_init(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  // 初始化
}

static void do_sensor_poll_pms_on(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  // 定期轮询传感器，并更新传感器数据
}

static void do_sensor_poll_pms_off(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  // 定期轮询传感器，并更新传感器数据
}

static sm_trans_t sm_trans_sensor_init[] = {
  {EV_EC11_UP, SM_SENSOR, SM_SENSOR_POLL_PMS_ON, do_sensor_init},
  {0, 0, 0, NULL}
};

static sm_trans_t sm_trans_sensor_poll_pms_on[] = {
  {EV_V1, SM_SENSOR, SM_SENSOR_POLL_PMS_OFF, do_sensor_poll_pms_off},
  {EV_1S, SM_SENSOR, SM_SENSOR_POLL_PMS_ON, do_sensor_poll_pms_on},
  {0, 0, 0, NULL}
};

static sm_trans_t sm_trans_sensor_poll_pms_off[] = {
  {EV_V1, SM_SENSOR, SM_SENSOR_POLL_PMS_ON, do_sensor_poll_pms_on},
  {EV_1S, SM_SENSOR, SM_SENSOR_POLL_PMS_OFF, do_sensor_poll_pms_off},
  {0, 0, 0, NULL}
};

sm_trans_t * sm_trans_sensor[] = {
  sm_trans_sensor_init,
  sm_trans_sensor_poll_pms_on,
  sm_trans_sensor_poll_pms_off
};
