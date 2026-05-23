#include "sensor_data.h"
#include "delay.h"
#include "logger.h"
#include "cext.h"

#include "tpm300.h"
#include "bmp280.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h" // 互斥锁包含在信号量头文件中

static const char * TAG = "SENSOR_DATA";

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

  while (cnt++ < SENSOR_DATA_INIT_UPDATE_CNT && !sensor_data_update(SENSOR_DATA_UPDATE_ALL, true)) {
    NEO_LOGI(TAG, "fill init data try cnt %d", cnt);
    delay_ms(1000);
  }
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

bool sensor_data_update(sensor_data_update_type_t type, bool init)
{
  bool ret = false;
  switch (type) {
    case SENSOR_DATA_UPDATE_TPM300:
      ret = sensor_data_update_tpm300(init);
      break;
    case SENSOR_DATA_UPDATE_BMP280:
      ret = sensor_data_update_bmp280(init);
      break;
    case SENSOR_DATA_UPDATE_PMS5003ST:
      ret = sensor_data_update_pms5003st(init);
      break;
    default: //SENSOR_DATA_UPDATE_ALL
      ret = sensor_data_update_tpm300(init) && sensor_data_update_bmp280(init) && sensor_data_update_pms5003st(init);
  }
  return ret;
}


 bool sensor_data_get_temp(float * ret)
{
  if (xSemaphoreTake(sensor_data_mutex, pdMS_TO_TICKS(SENSOR_DATA_MUTEX_MAX_WAIT_MS)) == pdTRUE) {
    *ret = sensor_data.pms5003st_data.temp;
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
    *ret = sensor_data.bmp280_press;
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