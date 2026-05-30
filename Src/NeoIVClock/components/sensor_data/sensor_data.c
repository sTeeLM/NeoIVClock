#include "sensor_data.h"
#include "delay.h"
#include "logger.h"
#include "cext.h"
#include "config.h"

#include "pms5003st.h"
#include "task.h"
#include "sm.h"
#include "tpm300.h"
#include "bmp280.h"

#include "reporter.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h" // 互斥锁包含在信号量头文件中

static const char * TAG = "SENSOR_DATA";

static sensor_data_temp_unit_t sensor_data_temp_unit;
static sensor_data_press_unit_t  sensor_data_press_unit;
static sensor_data_stage_t sensor_data_stage;
static SemaphoreHandle_t sensor_data_mutex;

#define SENSOR_DATA_MUTEX_MAX_WAIT_MS 1000

// 2/4/8/16
#define SENSOR_DATA_COE  4

#define SENSOR_DATA_INIT_UPDATE_CNT 5

static sensor_data_t sensor_data;

void sensor_data_init(void)
{
  uint8_t cnt = 0;
  NEO_LOGI(TAG, "init");

  sensor_data_mutex = xSemaphoreCreateMutex();

  memset(&sensor_data, 0, sizeof(sensor_data));

  pms5003st_enable(true);

  sensor_data_stage = SENSOR_DATA_STAGE2;

  while (cnt++ < SENSOR_DATA_INIT_UPDATE_CNT && !sensor_data_update(true)) {
    NEO_LOGI(TAG, "fill init data try cnt %d", cnt);
    delay_ms(1000);
  }

  pms5003st_enable(false);

  sensor_data_temp_unit = config_read_int("temp_unit") % SENSOR_DATA_TEMP_UNIT_CNT;
  sensor_data_press_unit = config_read_int("press_unit") % SENSOR_DATA_PRESS_UNIT_CNT;

  sensor_data_stage = SENSOR_DATA_STAGE0;
}

static bool sensor_data_update_bmp280(bool init)
{
  float temp, press;

  press =  bmp280_read_data(&temp); 

  if (xSemaphoreTake(sensor_data_mutex, pdMS_TO_TICKS(SENSOR_DATA_MUTEX_MAX_WAIT_MS)) == pdTRUE) {
    sensor_data.bmp280_temp = cext_iir_float(sensor_data.bmp280_temp, temp, init ? 1 : SENSOR_DATA_COE);
    sensor_data.bmp280_press = cext_iir_float(sensor_data.bmp280_press, press, init ? 1 : SENSOR_DATA_COE);
    xSemaphoreGive(sensor_data_mutex);
  } else {
    NEO_LOGW(TAG, "xSemaphoreTake failed");
    return false;
  }
  return true;
}

static bool sensor_data_update_tpm300(bool init)
{
  float tvoc;

  if((tvoc = tpm300_read_data()) < 0) {
    return false;
  }

  if (xSemaphoreTake(sensor_data_mutex, pdMS_TO_TICKS(SENSOR_DATA_MUTEX_MAX_WAIT_MS)) == pdTRUE) {
    sensor_data.tpm300_tvoc = cext_iir_float(sensor_data.tpm300_tvoc, tvoc, init ? 1 : SENSOR_DATA_COE);
    xSemaphoreGive(sensor_data_mutex);
  } else {
    NEO_LOGW(TAG, "xSemaphoreTake failed");
    return false;
  }

  return true;
}
/*
    uint16_t  pm_10;
    uint16_t  pm_25;   
    uint16_t  pm_100;        
    uint16_t  pm_10a; 
    uint16_t  pm_25a;  
    uint16_t  pm_100a; 
    uint16_t  pm_03cnt;
    uint16_t  pm_05cnt;
    uint16_t  pm_10cnt;   
    uint16_t  pm_25cnt; 
    uint16_t  pm_50cnt;   
    uint16_t  pm_100cnt;
    float  form;  
    float  temp;       
    float  mol;     
*/
static bool sensor_data_update_pms5003st(bool init)
{
  pms5003st_data_t data;
  if(!pms5003st_read_data(&data)) {
    return false;
  }

  if (xSemaphoreTake(sensor_data_mutex, pdMS_TO_TICKS(SENSOR_DATA_MUTEX_MAX_WAIT_MS)) == pdTRUE) {
    sensor_data.pms5003st_data.pm_10 = cext_iir_uint16(sensor_data.pms5003st_data.pm_10, data.pm_10, init ? 1 : SENSOR_DATA_COE);
    sensor_data.pms5003st_data.pm_25 = cext_iir_uint16(sensor_data.pms5003st_data.pm_25, data.pm_25, init ? 1 : SENSOR_DATA_COE);
    sensor_data.pms5003st_data.pm_100 = cext_iir_uint16(sensor_data.pms5003st_data.pm_10, data.pm_100, init ? 1 : SENSOR_DATA_COE);
    sensor_data.pms5003st_data.pm_10a = cext_iir_uint16(sensor_data.pms5003st_data.pm_10a, data.pm_10a, init ? 1 : SENSOR_DATA_COE);
    sensor_data.pms5003st_data.pm_25a = cext_iir_uint16(sensor_data.pms5003st_data.pm_25a, data.pm_25a, init ? 1 : SENSOR_DATA_COE);
    sensor_data.pms5003st_data.pm_100a = cext_iir_uint16(sensor_data.pms5003st_data.pm_10, data.pm_100a, init ? 1 : SENSOR_DATA_COE);
    sensor_data.pms5003st_data.pm_03cnt = cext_iir_uint16(sensor_data.pms5003st_data.pm_03cnt, data.pm_03cnt, init ? 1 : SENSOR_DATA_COE);   
    sensor_data.pms5003st_data.pm_05cnt = cext_iir_uint16(sensor_data.pms5003st_data.pm_05cnt, data.pm_05cnt, init ? 1 : SENSOR_DATA_COE);   
    sensor_data.pms5003st_data.pm_10cnt = cext_iir_uint16(sensor_data.pms5003st_data.pm_10cnt, data.pm_10cnt, init ? 1 : SENSOR_DATA_COE);   
    sensor_data.pms5003st_data.pm_25cnt = cext_iir_uint16(sensor_data.pms5003st_data.pm_25cnt, data.pm_25cnt, init ? 1 : SENSOR_DATA_COE);   
    sensor_data.pms5003st_data.pm_50cnt = cext_iir_uint16(sensor_data.pms5003st_data.pm_50cnt, data.pm_50cnt, init ? 1 : SENSOR_DATA_COE);   
    sensor_data.pms5003st_data.pm_100cnt = cext_iir_uint16(sensor_data.pms5003st_data.pm_100cnt, data.pm_100cnt, init ? 1 : SENSOR_DATA_COE);   
    sensor_data.pms5003st_data.form = cext_iir_float(sensor_data.pms5003st_data.form, data.form, init ? 1 : SENSOR_DATA_COE); 
    sensor_data.pms5003st_data.temp = cext_iir_float(sensor_data.pms5003st_data.temp, data.temp, init ? 1 : SENSOR_DATA_COE); 
    sensor_data.pms5003st_data.mol = cext_iir_float(sensor_data.pms5003st_data.mol, data.mol, init ? 1 : SENSOR_DATA_COE); 
    xSemaphoreGive(sensor_data_mutex);
  } else {
    NEO_LOGW(TAG, "xSemaphoreTake failed");
    return false;
  }

  return true;
}

bool sensor_data_update(bool init)
{
  bool ret = false;
  switch (sensor_data_stage) {
    case SENSOR_DATA_STAGE0:
    case SENSOR_DATA_STAGE1:
      ret = sensor_data_update_tpm300(init) && sensor_data_update_bmp280(init);
      break;
    case SENSOR_DATA_STAGE2:
      ret = sensor_data_update_tpm300(init) && sensor_data_update_bmp280(init) && sensor_data_update_pms5003st(init);
      break;
    default:;
  }
  return ret;
}

sensor_data_stage_t sensor_data_enter_stage(sensor_data_stage_t stage)
{
  switch(stage) {
    case SENSOR_DATA_STAGE0:
      NEO_LOGI(TAG, "enter stage0");
      pms5003st_enable(false);
      break;
    case SENSOR_DATA_STAGE1:
      NEO_LOGI(TAG, "enter stage1");
      pms5003st_enable(true);
      break;
    case SENSOR_DATA_STAGE2:
      NEO_LOGI(TAG, "enter stage2");
      pms5003st_enable(true);
      break;
    default:
      NEO_LOGW(TAG, "unknown stage %d", stage);
    break;
  }

  sensor_data_stage = stage;
  return sensor_data_stage;
}


 bool sensor_data_get_temp(float * ret)
{
  if (xSemaphoreTake(sensor_data_mutex, pdMS_TO_TICKS(SENSOR_DATA_MUTEX_MAX_WAIT_MS)) == pdTRUE) {
    *ret = (sensor_data_temp_unit == SENSOR_DATA_TEMP_UNIT_SHESHI ? 
      sensor_data.bmp280_temp : cext_celsius_to_fahrenheit(sensor_data.bmp280_temp));
    xSemaphoreGive(sensor_data_mutex);
  } else {
    NEO_LOGW(TAG, "xSemaphoreTake failed");
    return false;
  }
  return true;
}

bool sensor_data_get_press(float * ret)
{
  if (xSemaphoreTake(sensor_data_mutex, pdMS_TO_TICKS(SENSOR_DATA_MUTEX_MAX_WAIT_MS)) == pdTRUE) {
    *ret = (sensor_data_press_unit == SENSOR_DATA_PRESS_UNIT_HPA ? sensor_data.bmp280_press / 100.0f : 
    (sensor_data_press_unit == SENSOR_DATA_PRESS_UNIT_HGMM ? 
      cext_pa_to_mmhg(sensor_data.bmp280_press) : cext_pa_to_atm(sensor_data.bmp280_press)));
    xSemaphoreGive(sensor_data_mutex);
  } else {
    NEO_LOGW(TAG, "xSemaphoreTake failed");
    return false;
  }
  return true;
}

bool sensor_data_get_tvoc(float * ret)
{
  if (xSemaphoreTake(sensor_data_mutex, pdMS_TO_TICKS(SENSOR_DATA_MUTEX_MAX_WAIT_MS)) == pdTRUE) {
    *ret = sensor_data.tpm300_tvoc;
    xSemaphoreGive(sensor_data_mutex);
  } else {
    NEO_LOGW(TAG, "xSemaphoreTake failed");
    return false;
  }
  return true;
}

bool sensor_data_get_pm25(uint16_t * ret)
{
  if (xSemaphoreTake(sensor_data_mutex, pdMS_TO_TICKS(SENSOR_DATA_MUTEX_MAX_WAIT_MS)) == pdTRUE) {
    *ret = sensor_data.pms5003st_data.pm_25a;
    xSemaphoreGive(sensor_data_mutex);
  } else {
    NEO_LOGW(TAG, "xSemaphoreTake failed");
    return false;
  }
  return true;
}

bool sensor_data_get_form(float * ret)
{
  if (xSemaphoreTake(sensor_data_mutex, pdMS_TO_TICKS(SENSOR_DATA_MUTEX_MAX_WAIT_MS)) == pdTRUE) {
    *ret = sensor_data.pms5003st_data.form;
    xSemaphoreGive(sensor_data_mutex);
  } else {
    NEO_LOGW(TAG, "xSemaphoreTake failed");
    return false;
  }
  return true;
}


bool sensor_data_get_mol(float * ret)
{
  if (xSemaphoreTake(sensor_data_mutex, pdMS_TO_TICKS(SENSOR_DATA_MUTEX_MAX_WAIT_MS)) == pdTRUE) {
    *ret = sensor_data.pms5003st_data.mol;
    xSemaphoreGive(sensor_data_mutex);
  } else {
    NEO_LOGW(TAG, "xSemaphoreTake failed");
    return false;
  }
  return true;
}


bool sensor_data_get_all(sensor_data_t *data)
{
  if (xSemaphoreTake(sensor_data_mutex, pdMS_TO_TICKS(SENSOR_DATA_MUTEX_MAX_WAIT_MS)) == pdTRUE) {
    if(data)
      memcpy(data, &sensor_data, sizeof(sensor_data_t));
    xSemaphoreGive(sensor_data_mutex);
  } else {
    NEO_LOGW(TAG, "xSemaphoreTake failed");
    memset(data, 0, sizeof(sensor_data_t));
    return false;
  }
  return true;
}

sensor_data_temp_unit_t sensor_data_get_temp_unit(void)
{
  return sensor_data_temp_unit;
}
sensor_data_temp_unit_t sensor_data_set_temp_unit(sensor_data_temp_unit_t temp_unit)
{
  sensor_data_temp_unit = temp_unit % SENSOR_DATA_TEMP_UNIT_CNT;
  return sensor_data_temp_unit;
}

sensor_data_temp_unit_t sensor_data_next_temp_unit(void)
{
  sensor_data_temp_unit = (sensor_data_temp_unit + 1) % SENSOR_DATA_TEMP_UNIT_CNT;
  return sensor_data_temp_unit;
}

sensor_data_press_unit_t sensor_data_get_press_unit(void)
{
  return sensor_data_press_unit;
}

sensor_data_press_unit_t sensor_data_set_press_unit(sensor_data_press_unit_t press_unit)
{
  sensor_data_temp_unit = press_unit % SENSOR_DATA_PRESS_UNIT_CNT;
  return sensor_data_press_unit;
}

sensor_data_press_unit_t sensor_data_next_press_unit(void)
{
  sensor_data_press_unit = (sensor_data_press_unit + 1) % SENSOR_DATA_PRESS_UNIT_CNT;
  return sensor_data_temp_unit;
}

void sensor_data_save_config()
{
  config_write_int("temp_unit", sensor_data_temp_unit);
  config_write_int("press_unit", sensor_data_press_unit);  
}

// 如果间隔30分钟上报
// stage0: xx:00 ~ xx:20 / xx:30 ~ xx:50
// stage1  xx:20 ~ xx:25 / xx:50 ~ xx:55
// stage2  xx:25 ~ xx:30 / xx:55 ~ xx 00

// 如果间隔1小时上报
// stage0: xx:00 ~ xx:50
// stage1  xx:50 ~ xx:55
// stage2  xx:55 ~ xx:00
void sensor_data_test(uint8_t day, uint8_t hour, uint8_t min)
{
  NEO_EARLY_LOGI(TAG, "sensor_data_test %02u %02u:%02u", day, hour, min);
  if(reporter_get_interval() == 0) { // 30分钟上报一次
    if(min == 20 || min == 50) {
      task_set(EV_SENSOR_STAGE1);
    } else if (min == 25 || min == 55) {
      task_set(EV_SENSOR_STAGE2);
    } else if(min == 0) {
      task_set(EV_SENSOR_REPORT);
    }
  } else { // 1 小时上报一次
    if(min == 50) {
      task_set(EV_SENSOR_STAGE1);
    } else if (min == 55) {
      task_set(EV_SENSOR_STAGE2);
    } else if(min == 0) {
      task_set(EV_SENSOR_REPORT);
    }
  }
}

// 转发一下消息，变为ipc，因为sensor_data_test在中断上下文调用，无法调用task_set_ipc
void sensor_data_proc(task_event_t ev) 
{
  // 如果不判断CPU id，消息会在两个CPU之间来回跳
  if(esp_cpu_get_core_id() == SM_APP_CORE_ID)
    task_set_ipc(ev);
  else 
    sm_run(ev);
}