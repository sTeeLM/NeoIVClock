#ifndef NEO_IV_CLOCK_SM_SENSOR_H
#define NEO_IV_CLOCK_SM_SENSOR_H

#include "task.h"
#include "sm.h"

extern const char * sm_states_names_sensor[];
extern const sm_trans_t * sm_trans_sensor[];

enum sm_states_sensor
{
  SM_SENSOR_INIT, 
  SM_SENSOR_STAGE0, // 更新所有传感器数据，但是排除PMS（休眠状态）
  SM_SENSOR_STAGE1, // 更新所有传感器数据，预热PMS
  SM_SENSOR_STAGE2  // 更新所有传感器数据，准备上报
};

void do_sensor_init(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev);

#endif // NEO_IV_CLOCK_SM_SENSOR_H