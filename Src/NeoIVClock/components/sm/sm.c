#include "sm.h"
#include "logger.h"
#include "task.h"

#include "sm_clock.h"
#include "sm_net_access.h"
#include "sm_sensor.h"
#include "sm_set_alarm.h"
#include "sm_set_date.h"
#include "sm_set_param.h"
#include "sm_set_time.h"
#include "sm_stop_watch.h"
#include "sm_timer.h"
#include "sm_func_select.h"


static const char * TAG = "SM";

static const char * sm_functions_names[] = {
  "SM_FUNC_SELECT",      // 功能选择
  "SM_CLOCK",            // 时钟功能
  "SM_NET_ACCESS",       // 联网
  "SM_SET_TIME",         // 时间设置
  "SM_SET_DATE",         // 日期设置
  "SM_SET_ALARM",        // 闹钟设置
  "SM_SET_PARAM",        // 参数设置
  "SM_TIMER",            // 计时器功能
  "SM_STOP_WATCH",       // 马表功能
  "SM_SENSOR"            // 轮询传感器并上报数据
};

static const char ** sm_states_names[] = {
  sm_states_names_func_select,
  sm_states_names_clock,
  sm_states_names_net_access,
  sm_states_names_set_time,
  sm_states_names_set_date,
  sm_states_names_set_alarm,
  sm_states_names_set_param,
  sm_states_names_timer,
  sm_states_names_stop_watch,
  sm_states_names_sensor
};

static sm_trans_t ** sm_trans_table[] = {
  sm_trans_func_select,
  sm_trans_clock,
  sm_trans_net_access,
  sm_trans_set_time,
  sm_trans_set_date,
  sm_trans_set_alarm,
  sm_trans_set_param,
  sm_trans_timer,
  sm_trans_stop_watch,
  sm_trans_sensor
};

static uint8_t sm_cur_fuction[2];
static uint8_t sm_cur_state[2];

void sm_init(void)
{
  NEO_LOGI(TAG, "init");

  sm_cur_fuction[0] = SM_SENSOR;
  sm_cur_state[0]   = SM_SENSOR_INIT;

  sm_cur_fuction[1] = SM_CLOCK;
  sm_cur_state[1]   = SM_CLOCK_INIT;

  task_set(EV_EC11_UP);
  task_set_cpu(0, EV_EC11_UP);
}

void sm_run(task_event_t ev)
{
  uint8_t sm_cur_f, sm_cur_s;
  uint32_t cpuid = esp_cpu_get_core_id();

  sm_cur_f = sm_cur_fuction[cpuid];
  sm_cur_s = sm_cur_state[cpuid];
  
  sm_trans_t *p = NULL;
  
  p = sm_trans_table[sm_cur_f][sm_cur_s];
  while(p != NULL && p->sm_proc) {
    if(p->event == ev) {
      NEO_LOGD(TAG, "[%s][%s][%s] -> [%s][%s]",
        task_names[ev],
        sm_functions_names[sm_cur_f],
        sm_states_names[sm_cur_f][sm_cur_s],
        sm_functions_names[p->to_function],
        sm_states_names[p->to_function][p->to_state]     
      );
      p->sm_proc(sm_cur_f, sm_cur_s, p->to_function, p->to_state, ev);
      sm_cur_fuction[cpuid] = p->to_function;
      sm_cur_state[cpuid]   = p->to_state;
      break;
    }
    p ++;
  }
}

