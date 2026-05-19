#ifndef NEO_IV_CLOCK_REPORTER_H
#define NEO_IV_CLOCK_REPORTER_H

#include <stdint.h>

// 数据上报的协议封装

void reporter_init(void);

uint16_t reporter_get_interval(void);

void reporter_set_interval(uint16_t val);

#endif // NEO_IV_CLOCK_REPORTER_H
