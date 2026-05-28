#include "sm_sensor.h"
#include "task.h"
#include "sm.h"
#include "logger.h"

#include "pms5003st.h"
#include "sensor_data.h"
#include "reporter.h"

static const char * TAG = "SM_SENSOR";

const char * sm_states_names_sensor[] = {
  "SM_SENSOR_INIT",
  "SM_SENSOR_STAGE0",
  "SM_SENSOR_STAGE1",
  "SM_SENSOR_STAGE2"  
};


void do_sensor_init(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  NEO_LOGD(TAG, "do_sensor_init");
}

static void do_sensor_stage0(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  sensor_data_t data = {};

  if(ev == EV_1S) {
    sensor_data_update(false);
    task_set_ipc(EV_SENSOR_UPDATE);
  } else if(ev == EV_SENSOR_REPORT) {
    // report data
    if(sensor_data_get_all(&data)) {
      if(!reporter_report_data(&data)) {
        NEO_LOGW(TAG, "reporter_report_data error");
      }
    } else {
      NEO_LOGW(TAG, "sensor_data_get_all error");
    }
    // switch stage
    sensor_data_enter_stage(SENSOR_DATA_STAGE0);
  }
}

static void do_sensor_stage1(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  if(ev == EV_1S) {
    sensor_data_update(false);
    pms5003st_collect_garbage_data();
    task_set_ipc(EV_SENSOR_UPDATE);
  } else if(ev == EV_SENSOR_STAGE1) {
    // switch stage
    sensor_data_enter_stage(SENSOR_DATA_STAGE1);
  }
}

static void do_sensor_stage2(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  if(ev == EV_1S) {
    sensor_data_update(false);
    task_set_ipc(EV_SENSOR_UPDATE);
  } else if(ev == EV_SENSOR_STAGE2) {
    // switch stage
    sensor_data_enter_stage(SENSOR_DATA_STAGE2);
  }
}

static const sm_trans_t sm_trans_sensor_init[] = {
  {EV_EC11_UP, SM_SENSOR, SM_SENSOR_STAGE0, do_sensor_init},
  {0, 0, 0, NULL}
};

static const sm_trans_t sm_trans_sensor_stage0[] = {
  {EV_1S, SM_SENSOR, SM_SENSOR_STAGE0, do_sensor_stage0},
  {EV_SENSOR_STAGE1, SM_SENSOR, SM_SENSOR_STAGE1, do_sensor_stage1},
  {0, 0, 0, NULL}
};

static const sm_trans_t sm_trans_sensor_stage1[] = {
  {EV_1S, SM_SENSOR, SM_SENSOR_STAGE1, do_sensor_stage1},
  {EV_SENSOR_STAGE2, SM_SENSOR, SM_SENSOR_STAGE2, do_sensor_stage2},
  {0, 0, 0, NULL}
};

static const sm_trans_t sm_trans_sensor_stage2[] = {
  {EV_1S, SM_SENSOR, SM_SENSOR_STAGE2, do_sensor_stage2},
  {EV_SENSOR_REPORT, SM_SENSOR, SM_SENSOR_STAGE0, do_sensor_stage0},  
  {0, 0, 0, NULL}
};

const sm_trans_t * sm_trans_sensor[] = {
  sm_trans_sensor_init,
  sm_trans_sensor_stage0,
  sm_trans_sensor_stage1,
  sm_trans_sensor_stage2
};
