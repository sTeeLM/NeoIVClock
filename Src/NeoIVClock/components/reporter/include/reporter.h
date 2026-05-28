#ifndef NEO_IV_CLOCK_REPORTER_H
#define NEO_IV_CLOCK_REPORTER_H

#include <stdint.h>

#include "sensor_data.h"

// 数据上报的协议封装

void reporter_init(void);

uint8_t reporter_get_interval(void);

uint8_t reporter_inc_interval(void);

bool reporter_report_data(const sensor_data_t * data);

void reporter_save_config(void);

#endif // NEO_IV_CLOCK_REPORTER_H
