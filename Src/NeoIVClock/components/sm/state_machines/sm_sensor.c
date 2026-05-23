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
  "SM_SENSOR_POLL_PMS_ON",
  "SM_SENSOR_POLL_PMS_OFF"
};

#define SM_SENSOR_PMS_ON_SEC   (10 * 60)
#define SM_SENSOR_PMS_OFF_SEC  (60 * 60)
// 固定一秒采样一次
// PMS每1小时激活10分钟
// 数据上报间隔，根据配置

static uint32_t interal_sec_pms;
static uint32_t interal_sec_report;

void do_sensor_init(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  // 初始化
  interal_sec_pms     = 0;
  interal_sec_report  = 0;

  pms5003st_enable(true);
}

static void do_sensor_poll_pms_on(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  sensor_data_t data;

  NEO_LOGD(TAG, "switch to pms on");

  if(ev == EV_V1)
    pms5003st_enable(true);

  // 定期轮询传感器，并更新传感器数据
  interal_sec_pms ++;
  if(interal_sec_pms >= SM_SENSOR_PMS_ON_SEC) {
    task_set(EV_V1);
    interal_sec_pms = 0;
  }

  sensor_data_update(SENSOR_DATA_UPDATE_ALL, false);

  interal_sec_report ++;
  if(interal_sec_report >= reporter_get_interval()) {
    if(sensor_data_get_all(&data)) {
      task_set_ipc(EV_UPDATE_SENSOR);
      if(!reporter_report_data(&data)) {
        NEO_LOGW(TAG, "reporter report data failed");
      }
    } else {
      NEO_LOGW(TAG, "sensor get data failed");
    }
    interal_sec_report = 0;
  }
}

static void do_sensor_poll_pms_off(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  // 定期轮询传感器，并更新传感器数据
  sensor_data_t data;

  NEO_LOGD(TAG, "switch to pms on");

  if(ev == EV_V1)
    pms5003st_enable(false);

  // 定期轮询传感器，并更新传感器数据
  interal_sec_pms ++;
  if(interal_sec_pms >= SM_SENSOR_PMS_OFF_SEC) {
    task_set(EV_V1);
    interal_sec_pms = 0;
  }

  sensor_data_update(SENSOR_DATA_UPDATE_BMP280, false);
  sensor_data_update(SENSOR_DATA_UPDATE_TPM300, false);  

  interal_sec_report ++;
  if(interal_sec_report >= reporter_get_interval()) {
    if(sensor_data_get_all(&data)) {
      task_set_ipc(EV_UPDATE_SENSOR);
      if(!reporter_report_data(&data)) {
        NEO_LOGW(TAG, "reporter report data failed");
      }
    } else {
      NEO_LOGW(TAG, "sensor get data failed");
    }
    interal_sec_report = 0;
  }
}

static const sm_trans_t sm_trans_sensor_init[] = {
  {EV_EC11_UP, SM_SENSOR, SM_SENSOR_POLL_PMS_ON, do_sensor_init},
  {0, 0, 0, NULL}
};

static const sm_trans_t sm_trans_sensor_poll_pms_on[] = {
  {EV_V1, SM_SENSOR, SM_SENSOR_POLL_PMS_OFF, do_sensor_poll_pms_off},
  {EV_1S, SM_SENSOR, SM_SENSOR_POLL_PMS_ON, do_sensor_poll_pms_on},
  {0, 0, 0, NULL}
};

static const sm_trans_t sm_trans_sensor_poll_pms_off[] = {
  {EV_V1, SM_SENSOR, SM_SENSOR_POLL_PMS_ON, do_sensor_poll_pms_on},
  {EV_1S, SM_SENSOR, SM_SENSOR_POLL_PMS_OFF, do_sensor_poll_pms_off},
  {0, 0, 0, NULL}
};

const sm_trans_t * sm_trans_sensor[] = {
  sm_trans_sensor_init,
  sm_trans_sensor_poll_pms_on,
  sm_trans_sensor_poll_pms_off
};
