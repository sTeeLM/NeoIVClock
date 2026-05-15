#ifndef NEO_IV_CLOCK_SM_H
#define NEO_IV_CLOCK_SM_H

#include <stdint.h>
#include <stdbool.h>

#include "task.h"

typedef enum _sm_functions_t 
{
  SM_CLOCK = 0,             // 主功能：时间日期展示，传感器数据展示
  SM_NET_ACCESS,            // 联网
  SM_SET_TIME,              // 时间设置 
  SM_SET_DATE,              // 日期设置
  SM_SET_ALARM,             // 闹钟设置
  SM_SET_PARAM,             // 参数设置
  SM_TIMER,                 // 倒计时
  SM_STOP_WATCH,            // 跑表
  SM_SENSOR,                // 轮询传感器并上报数据
  SM_FUNCTION_CNT
} sm_functions_t;


typedef void (*SM_PROC)(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev);

typedef struct _sm_trans_t
{
  uint8_t event;
  uint8_t to_function;
  uint8_t to_state;
  SM_PROC sm_proc;
} sm_trans_t;

void sm_init(void);

void sm_run(task_event_t ev);

#endif  // NEO_IV_CLOCK_SM_H
