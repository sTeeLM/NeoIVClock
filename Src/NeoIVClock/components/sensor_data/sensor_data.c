#include "sensor_data.h"
#include "logger.h"
#include "cext.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h" // 互斥锁包含在信号量头文件中

static const char * TAG = "SENSOR_DATA";

static SemaphoreHandle_t sensor_data_mutex;

#define SENSOR_DATA_MUTEX_MAX_WAIT_MS 1000

// 2/4/8/16
#define SENSOR_DATA_COE  4

static sensor_data_t sensor_data;

static float sensor_data_temp[SENSOR_DATA_BUFFER_SIZE];
static float sensor_data_press[SENSOR_DATA_BUFFER_SIZE];
static float sensor_data_tvoc[SENSOR_DATA_BUFFER_SIZE];
static float sensor_data_form[SENSOR_DATA_BUFFER_SIZE];
static float sensor_data_mol[SENSOR_DATA_BUFFER_SIZE];
static uint16_t sensor_data_pm25[SENSOR_DATA_BUFFER_SIZE];

void sensor_data_init(void)
{
  NEO_LOGI(TAG, "init");

  sensor_data_mutex = xSemaphoreCreateMutex();

  memset(&sensor_data, 0, sizeof(sensor_data));
  memset(sensor_data_temp, 0, sizeof(sensor_data_temp));
  memset(sensor_data_mol, 0, sizeof(sensor_data_mol));    
  memset(sensor_data_press, 0, sizeof(sensor_data_press));  
  memset(&sensor_data_tvoc, 0, sizeof(sensor_data_tvoc));
  memset(sensor_data_form, 0, sizeof(sensor_data_form));
  memset(sensor_data_pm25, 0, sizeof(sensor_data_pm25));   
}

static bool sensor_data_update_bmp280(void)
{
  float temp, press;

  press =  bmp280_read_data(&temp); 

  if (xSemaphoreTake(sensor_data_mutex, pdMS_TO_TICKS(SENSOR_DATA_MUTEX_MAX_WAIT_MS)) == pdTRUE) {
    sensor_data.bmp280_temp = cext_iir_float(sensor_data.bmp280_temp, temp, SENSOR_DATA_COE);
    sensor_data.bmp280_press = cext_iir_float(sensor_data.bmp280_press, press, SENSOR_DATA_COE);
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
    sensor_data.tpm300_tvoc = cext_iir_float(sensor_data.tpm300_tvoc, tvoc, SENSOR_DATA_COE);
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
    sensor_data.pms5003st_data.pm_10 = cext_iir_uint16(sensor_data.pms5003st_data.pm_10, data.pm_10, SENSOR_DATA_COE);
    sensor_data.pms5003st_data.pm_25 = cext_iir_uint16(sensor_data.pms5003st_data.pm_25, data.pm_25, SENSOR_DATA_COE);
    sensor_data.pms5003st_data.pm_100 = cext_iir_uint16(sensor_data.pms5003st_data.pm_10, data.pm_100, SENSOR_DATA_COE);
    sensor_data.pms5003st_data.pm_10a = cext_iir_uint16(sensor_data.pms5003st_data.pm_10a, data.pm_10a, SENSOR_DATA_COE);
    sensor_data.pms5003st_data.pm_25a = cext_iir_uint16(sensor_data.pms5003st_data.pm_25a, data.pm_25a, SENSOR_DATA_COE);
    sensor_data.pms5003st_data.pm_100a = cext_iir_uint16(sensor_data.pms5003st_data.pm_10, data.pm_100a, SENSOR_DATA_COE);
    sensor_data.pms5003st_data.pm_03cnt = cext_iir_uint16(sensor_data.pms5003st_data.pm_03cnt, data.pm_03cnt, SENSOR_DATA_COE);   
    sensor_data.pms5003st_data.pm_05cnt = cext_iir_uint16(sensor_data.pms5003st_data.pm_05cnt, data.pm_05cnt, SENSOR_DATA_COE);   
    sensor_data.pms5003st_data.pm_10cnt = cext_iir_uint16(sensor_data.pms5003st_data.pm_10cnt, data.pm_10cnt, SENSOR_DATA_COE);   
    sensor_data.pms5003st_data.pm_25cnt = cext_iir_uint16(sensor_data.pms5003st_data.pm_25cnt, data.pm_25cnt, SENSOR_DATA_COE);   
    sensor_data.pms5003st_data.pm_50cnt = cext_iir_uint16(sensor_data.pms5003st_data.pm_50cnt, data.pm_50cnt, SENSOR_DATA_COE);   
    sensor_data.pms5003st_data.pm_100cnt = cext_iir_uint16(sensor_data.pms5003st_data.pm_100cnt, data.pm_100cnt, SENSOR_DATA_COE);   
    sensor_data.pms5003st_data.form = cext_iir_float(sensor_data.pms5003st_data.form, data.form, SENSOR_DATA_COE); 
    sensor_data.pms5003st_data.temp = cext_iir_float(sensor_data.pms5003st_data.temp, data.temp, SENSOR_DATA_COE); 
    sensor_data.pms5003st_data.mol = cext_iir_float(sensor_data.pms5003st_data.mol, data.mol, SENSOR_DATA_COE); 
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
    default: //SENSOR_DATA_UPDATE_ALL
      ret = sensor_data_update_tpm300();
      ret = ret && sensor_data_update_bmp280();
      ret = ret && sensor_data_update_pms5003st();
  }
  return ret;
}


 bool sensor_data_get_temp(float * ret)
{
  if (xSemaphoreTake(sensor_data_mutex, pdMS_TO_TICKS(SENSOR_DATA_MUTEX_MAX_WAIT_MS)) == pdTRUE) {
    memcpy(ret, sensor_data_temp, sizeof(sensor_data_temp));
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
    memcpy(ret, sensor_data_press, sizeof(sensor_data_press));
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
    memcpy(ret, sensor_data_tvoc, sizeof(sensor_data_tvoc));
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
    memcpy(ret, sensor_data_pm25, sizeof(sensor_data_pm25));
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
    memcpy(ret, sensor_data_form, sizeof(sensor_data_form));
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
    memcpy(ret, sensor_data_mol, sizeof(sensor_data_mol));
    xSemaphoreGive(sensor_data_mutex);
  } else {
    NEO_LOGW(TAG, "xSemaphoreTake failed");
    return false;
  }
  return true;
}

// 将buffer向高位移动一位，并将[0填充为value]
static void sensor_fill_buffer_float(float * buffer, float val)
{
  uint8_t i;
  for(i = SENSOR_DATA_BUFFER_SIZE - 1 ; i > 0 ; i --) {
    buffer[i] = buffer[i-1];
  }
  buffer[i] = val;
}

static void sensor_fill_buffer_uint16(uint16_t * buffer, uint16_t val)
{
  uint8_t i;
  for(i = SENSOR_DATA_BUFFER_SIZE - 1 ; i > 0 ; i --) {
    buffer[i] = buffer[i-1];
  }
  buffer[i] = val;
}

/*
  memset(&sensor_data, 0, sizeof(sensor_data));
  memset(sensor_data_temp, 0, sizeof(sensor_data_temp));
  memset(sensor_data_mol, 0, sizeof(sensor_data_mol));    
  memset(sensor_data_press, 0, sizeof(sensor_data_press));  
  memset(&sensor_data_tvoc, 0, sizeof(sensor_data_tvoc));
  memset(sensor_data_form, 0, sizeof(sensor_data_form));
  memset(sensor_data_pm25, 0, sizeof(sensor_data_pm25)); 
*/
bool sensor_data_get_all(sensor_data_t *data)
{
  if (xSemaphoreTake(sensor_data_mutex, pdMS_TO_TICKS(SENSOR_DATA_MUTEX_MAX_WAIT_MS)) == pdTRUE) {
    memcpy(data, &sensor_data.pms5003st_data, sizeof(sensor_data_t));
    // 注意我们有太多地方可以测量温度了。。。
    sensor_fill_buffer_float(sensor_data_temp, sensor_data.pms5003st_data.temp);
    sensor_fill_buffer_float(sensor_data_mol, sensor_data.pms5003st_data.mol);
    sensor_fill_buffer_float(sensor_data_press, sensor_data.bmp280_press);
    sensor_fill_buffer_float(sensor_data_tvoc, sensor_data.tpm300_tvoc);
    sensor_fill_buffer_float(sensor_data_form, sensor_data.pms5003st_data.form);
    sensor_fill_buffer_uint16(sensor_data_pm25, sensor_data.pms5003st_data.pm_25a);
    xSemaphoreGive(sensor_data_mutex);
  } else {
    NEO_LOGW(TAG, "xSemaphoreTake failed");
    memset(data, 0, sizeof(sensor_data_t));
    return false;
  }
  return true;
}