#ifndef NEO_IV_CLOCK_AHT20_H
#define NEO_IV_CLOCK_AHT20_H

#include <stdint.h>
#include <stdbool.h>

void aht20_init(void);

// 返回温度读数，如果mol不为NULL，返回湿度
float aht20_read_data(float * mol);

#endif //NEO_IV_CLOCK_AHT20_H