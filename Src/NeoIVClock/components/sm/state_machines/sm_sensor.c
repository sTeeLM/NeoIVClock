#include "sm_sensor.h"
#include "task.h"
#include "sm.h"
#include "logger.h"
#include "task.h"

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
  
  if(ev == EV_10S) {
    sensor_data_update(false);
    task_set_ipc(EV_SENSOR_UPDATE);
  } else if(ev == EV_SENSOR_STAGE0 ) {
    if(from_state != SM_SENSOR_STAGE0) {
      sensor_data_enter_stage(SENSOR_DATA_STAGE0);
    }
    sensor_data_update(false);
    if(sensor_data_get_all(&data)) {
      reporter_report_data(&data, task_get_arg(ev));
    }
  }
}

static void do_sensor_stage1(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  sensor_data_t data = {};
  
  if(ev == EV_10S) {
    sensor_data_update(false);
    task_set_ipc(EV_SENSOR_UPDATE);
  } else if(ev == EV_SENSOR_STAGE1) {
    if(from_state != SM_SENSOR_STAGE1) {
      sensor_data_enter_stage(SENSOR_DATA_STAGE1);
    }
    sensor_data_update(false);
    if(sensor_data_get_all(&data)) {
      reporter_report_data(&data, task_get_arg(ev));
    }    
  }

}

static void do_sensor_stage2(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  sensor_data_t data = {};

  if(ev == EV_10S) {
    sensor_data_update(false);
    task_set_ipc(EV_SENSOR_UPDATE);
  } else if(ev == EV_SENSOR_STAGE2) {
    if(from_state != SM_SENSOR_STAGE2) {
      sensor_data_enter_stage(SENSOR_DATA_STAGE2);
    }
    sensor_data_update(false);
    if(sensor_data_get_all(&data)) {
      reporter_report_data(&data, task_get_arg(ev));
    }    
  }
}

static const sm_trans_t sm_trans_sensor_init[] = {
  {EV_EC11_UP, SM_SENSOR, SM_SENSOR_STAGE0, do_sensor_init},
  {0, 0, 0, NULL}
};

static const sm_trans_t sm_trans_sensor_stage0[] = {
  {EV_10S, SM_SENSOR, SM_SENSOR_STAGE0, do_sensor_stage0},
  {EV_SENSOR_STAGE0, SM_SENSOR, SM_SENSOR_STAGE0, do_sensor_stage0},
  {EV_SENSOR_STAGE1, SM_SENSOR, SM_SENSOR_STAGE1, do_sensor_stage1},
  {EV_SENSOR_STAGE2, SM_SENSOR, SM_SENSOR_STAGE2, do_sensor_stage2},
  {0, 0, 0, NULL}
};

static const sm_trans_t sm_trans_sensor_stage1[] = {
  {EV_10S, SM_SENSOR, SM_SENSOR_STAGE1, do_sensor_stage1},
  {EV_SENSOR_STAGE0, SM_SENSOR, SM_SENSOR_STAGE0, do_sensor_stage0},
  {EV_SENSOR_STAGE1, SM_SENSOR, SM_SENSOR_STAGE1, do_sensor_stage1},
  {EV_SENSOR_STAGE2, SM_SENSOR, SM_SENSOR_STAGE2, do_sensor_stage2},
  {0, 0, 0, NULL}
};

static const sm_trans_t sm_trans_sensor_stage2[] = {
  {EV_10S, SM_SENSOR, SM_SENSOR_STAGE2, do_sensor_stage2},
  {EV_SENSOR_STAGE0, SM_SENSOR, SM_SENSOR_STAGE0, do_sensor_stage0},   
  {EV_SENSOR_STAGE1, SM_SENSOR, SM_SENSOR_STAGE1, do_sensor_stage1},
  {EV_SENSOR_STAGE2, SM_SENSOR, SM_SENSOR_STAGE2, do_sensor_stage2}, 
  {0, 0, 0, NULL}
};

const sm_trans_t * sm_trans_sensor[] = {
  sm_trans_sensor_init,
  sm_trans_sensor_stage0,
  sm_trans_sensor_stage1,
  sm_trans_sensor_stage2
};
