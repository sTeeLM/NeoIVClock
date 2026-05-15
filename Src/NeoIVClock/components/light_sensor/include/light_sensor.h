#ifndef NEO_IV_CLOCK_LIGHT_SENSOR_H
#define NEO_IV_CLOCK_LIGHT_SENSOR_H

#include <stdint.h>

void light_sensor_init(void);
int32_t light_sensor_read_data(void);

#endif //NEO_IV_CLOCK_LIGHT_SENSOR_H