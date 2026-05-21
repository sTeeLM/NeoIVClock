#ifndef NEO_IV_CLOCK_SENSOR_DATA_H
#define NEO_IV_CLOCK_SENSOR_DATA_H

#include <stdint.h>
#include <stdbool.h>

#include "pms5003st.h"
#include "tpm300.h"
#include "bmp280.h"

typedef struct _sensor_data_t
{
  pms5003st_data_t pms5003st_data;
  float tpm300_tvoc;
  float bmp280_temp;
  float bmp280_press;  
} __attribute__((packed)) sensor_data_t;

typedef enum _sensor_data_update_type_t
{
  SENSOR_DATA_UPDATE_ALL = 0,
  SENSOR_DATA_UPDATE_TPM300,
  SENSOR_DATA_UPDATE_BMP280,
  SENSOR_DATA_UPDATE_PMS5003ST
} sensor_data_update_type_t;

void sensor_data_init(void);

bool sensor_data_update(sensor_data_update_type_t type, bool init);

// 读取温度, SENSOR_DATA_BUFFER_SIZE个数据，[0] 是最新的， 下同
bool sensor_data_get_temp(float * data);

// 读取气压
bool sensor_data_get_press(float * data);

// 读取tvoc
bool sensor_data_get_tvoc(float * data);

// 读取PM2.5
bool sensor_data_get_pm25(uint16_t * data);

// 读取甲醛
bool sensor_data_get_form(float * data);

// 读取相对湿度
bool sensor_data_get_mol(float * data);

// 一次上报，读取整个数据（数据上报时用）,并更新data_buffer
bool sensor_data_get_all(sensor_data_t *data);

#endif // NEO_IV_CLOCK_ALARM_H
