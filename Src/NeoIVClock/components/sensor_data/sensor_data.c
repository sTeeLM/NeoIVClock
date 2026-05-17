#include "sensor_data.h"
#include "logger.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h" // 互斥锁包含在信号量头文件中

static const char * TAG = "SENSOR_DATA";

static SemaphoreHandle_t sensor_data_mutex;

#define SENSOR_DATA_MUTEX_MAX_WAIT_MS 1000

// 2/4/8/16
#define SENSOR_DATA_COE  4

static sensor_data_t sensor_data;

void sensor_data_init(void)
{
  NEO_LOGI(TAG, "init");

  sensor_data_mutex = xSemaphoreCreateMutex();

  memset(&sensor_data, 0, sizeof(sensor_data));
}

/*
  IIR:

  value = (value * (filter_coefficient - 1) + new_data) / filter_coefficient

*/
static uint16_t sensor_data_iir_uint16(uint16_t oldv, uint16_t newv)
{
  int32_t ret = oldv;
  ret += (int32_t)((newv - oldv) / SENSOR_DATA_COE);
  return  (uint16_t) ret;
}

static float sensor_data_iir_float(float oldv, float newv)
{
  float ret = oldv;
  ret += (float)((newv - oldv) / SENSOR_DATA_COE);
  return ret;
}

static bool sensor_data_update_bmp280(void)
{
  float temp, press;

  press =  bmp280_read_data(&temp); 

  if (xSemaphoreTake(sensor_data_mutex, pdMS_TO_TICKS(SENSOR_DATA_MUTEX_MAX_WAIT_MS)) == pdTRUE) {
    sensor_data.bmp280_temp = sensor_data_iir_float(sensor_data.bmp280_temp, temp);
    sensor_data.bmp280_press = sensor_data_iir_float(sensor_data.bmp280_press, press);
    xSemaphoreGive(sensor_data_mutex);
  } else {
    NEO_LOGW(TAG, "xSemaphoreTake failed");
    return false;
  }
  return true;
}

static bool sensor_data_update_tpm300()
{
  float tvoc;

  if((tvoc = tpm300_read_data()) < 0) {
    return false;
  }

  if (xSemaphoreTake(sensor_data_mutex, pdMS_TO_TICKS(SENSOR_DATA_MUTEX_MAX_WAIT_MS)) == pdTRUE) {
    sensor_data.tpm300_tvoc = sensor_data_iir_float(sensor_data.tpm300_tvoc, tvoc);
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
static bool sensor_data_update_pms5003st()
{
  pms5003st_data_t data;
  if(!pms5003st_read_data(&data)) {
    return false;
  }

  if (xSemaphoreTake(sensor_data_mutex, pdMS_TO_TICKS(SENSOR_DATA_MUTEX_MAX_WAIT_MS)) == pdTRUE) {
    sensor_data.pms5003st_data.pm_10 = sensor_data_iir_uint16(sensor_data.pms5003st_data.pm_10, data.pm_10);
    sensor_data.pms5003st_data.pm_25 = sensor_data_iir_uint16(sensor_data.pms5003st_data.pm_25, data.pm_25);
    sensor_data.pms5003st_data.pm_100 = sensor_data_iir_uint16(sensor_data.pms5003st_data.pm_10, data.pm_100);
    sensor_data.pms5003st_data.pm_10a = sensor_data_iir_uint16(sensor_data.pms5003st_data.pm_10a, data.pm_10a);
    sensor_data.pms5003st_data.pm_25a = sensor_data_iir_uint16(sensor_data.pms5003st_data.pm_25a, data.pm_25a);
    sensor_data.pms5003st_data.pm_100a = sensor_data_iir_uint16(sensor_data.pms5003st_data.pm_10, data.pm_100a);
    sensor_data.pms5003st_data.pm_03cnt = sensor_data_iir_uint16(sensor_data.pms5003st_data.pm_03cnt, data.pm_03cnt);   
    sensor_data.pms5003st_data.pm_05cnt = sensor_data_iir_uint16(sensor_data.pms5003st_data.pm_05cnt, data.pm_05cnt);   
    sensor_data.pms5003st_data.pm_10cnt = sensor_data_iir_uint16(sensor_data.pms5003st_data.pm_10cnt, data.pm_10cnt);   
    sensor_data.pms5003st_data.pm_25cnt = sensor_data_iir_uint16(sensor_data.pms5003st_data.pm_25cnt, data.pm_25cnt);   
    sensor_data.pms5003st_data.pm_50cnt = sensor_data_iir_uint16(sensor_data.pms5003st_data.pm_50cnt, data.pm_50cnt);   
    sensor_data.pms5003st_data.pm_100cnt = sensor_data_iir_uint16(sensor_data.pms5003st_data.pm_100cnt, data.pm_100cnt);   
    sensor_data.pms5003st_data.form = sensor_data_iir_float(sensor_data.pms5003st_data.form, data.form); 
    sensor_data.pms5003st_data.temp = sensor_data_iir_float(sensor_data.pms5003st_data.temp, data.temp); 
    sensor_data.pms5003st_data.mol = sensor_data_iir_float(sensor_data.pms5003st_data.mol, data.mol); 
    xSemaphoreGive(sensor_data_mutex);
  } else {
    NEO_LOGW(TAG, "xSemaphoreTake failed");
    return false;
  }

  return true;
}

bool sensor_data_update(sensor_data_update_type_t type)
{
  bool ret = false;
  switch (type) {
    case SENSOR_DATA_UPDATE_TPM300:
      ret = sensor_data_update_tpm300();
      break;
    case SENSOR_DATA_UPDATE_BMP280:
      ret = sensor_data_update_bmp280();
      break;
    case SENSOR_DATA_UPDATE_PMS5003ST:
      ret = sensor_data_update_pms5003st();
      break;
    default:
      ret = sensor_data_update_tpm300();
      ret = ret && sensor_data_update_bmp280();
      ret = ret && sensor_data_update_pms5003st();
  }
  return ret;
}


 bool sensor_data_get_temp(float * ret)
{
  if (xSemaphoreTake(sensor_data_mutex, pdMS_TO_TICKS(SENSOR_DATA_MUTEX_MAX_WAIT_MS)) == pdTRUE) {
    *ret = sensor_data.bmp280_temp;
    xSemaphoreGive(sensor_data_mutex);
  } else {
    NEO_LOGW(TAG, "xSemaphoreTake failed");
    *ret = 0.0f;
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
    *ret = 0.0f;
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
    *ret = 0.0f;
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
    *ret = 0;
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
    *ret = 0.0f;
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
    *ret = 0.0f;
    return false;
  }
  return true;
}

bool sensor_data_get_all(sensor_data_t *data)
{
  if (xSemaphoreTake(sensor_data_mutex, pdMS_TO_TICKS(SENSOR_DATA_MUTEX_MAX_WAIT_MS)) == pdTRUE) {
    memcpy(data, &sensor_data.pms5003st_data, sizeof(sensor_data_t));
    xSemaphoreGive(sensor_data_mutex);
  } else {
    NEO_LOGW(TAG, "xSemaphoreTake failed");
    memset(data, 0, sizeof(sensor_data_t));
    return false;
  }
  return true;
}