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

bool sensor_data_update(sensor_data_update_type_t type);


// 读取温度
float sensor_data_get_temp(void);

// 读取气压
float sensor_data_get_press(void);

// 读取tvoc
float sensor_data_get_tvoc(void);

// 读取PM2.5
uint16_t sensor_data_get_pm25(void);

// 读取甲醛
float sensor_data_get_form(void);

// 读取相对湿度
float sensor_data_get_mol(void);

#endif // NEO_IV_CLOCK_ALARM_H
