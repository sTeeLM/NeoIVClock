#ifndef NEO_IV_CLOCK_SENSOR_DATA_H
#define NEO_IV_CLOCK_SENSOR_DATA_H

#include <stdint.h>
#include <stdbool.h>

#include "pms5003st.h"
#include "task.h"

typedef struct _sensor_data_t
{
  pms5003st_data_t pms5003st_data;
  float tpm300_tvoc;
  float bmp280_temp;
  float bmp280_press;  
} __attribute__((packed)) sensor_data_t;


typedef enum _sensor_data_stage_t
{
  SENSOR_DATA_STAGE0 = 0,     // 除pms5003st外其他传感器工作，数据正常更新
  SENSOR_DATA_STAGE1,         // pms5003st唤醒，预热，不更新数据，其他传感器正常更新
  SENSOR_DATA_STAGE2,         // 所有传感器正常工作，正常更新数据
} sensor_data_stage_t;

void sensor_data_init(void);

sensor_data_stage_t sensor_data_enter_stage(sensor_data_stage_t stage);

bool sensor_data_update(bool init);

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

typedef enum _sensor_data_temp_unit_t
{
  SENSOR_DATA_TEMP_UNIT_SHESHI = 0,
  SENSOR_DATA_TEMP_UNIT_HUASHI,
  SENSOR_DATA_TEMP_UNIT_CNT
} sensor_data_temp_unit_t;
// 
sensor_data_temp_unit_t sensor_data_get_temp_unit(void);
sensor_data_temp_unit_t sensor_data_set_temp_unit(sensor_data_temp_unit_t temp_unit);
sensor_data_temp_unit_t sensor_data_next_temp_unit(void);

typedef enum _sensor_data_press_unit_t
{
  SENSOR_DATA_PRESS_UNIT_HPA = 0,
  SENSOR_DATA_PRESS_UNIT_HGMM,
  SENSOR_DATA_PRESS_UNIT_ATM,
  SENSOR_DATA_PRESS_UNIT_CNT
} sensor_data_press_unit_t;
// 
sensor_data_press_unit_t sensor_data_get_press_unit(void);
sensor_data_press_unit_t sensor_data_set_press_unit(sensor_data_press_unit_t press_unit);
sensor_data_press_unit_t sensor_data_next_press_unit(void);

void sensor_data_save_config();

void sensor_data_test(uint8_t day, uint8_t hour, uint8_t min);

void sensor_data_proc(task_event_t ev);

#endif // NEO_IV_CLOCK_ALARM_H
