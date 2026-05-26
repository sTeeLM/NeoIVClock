#ifndef NEO_IV_CLOCK_MONTION_SENSOR_H
#define NEO_IV_CLOCK_MONTION_SENSOR_H

#include <stdint.h>
#include <stdbool.h>

void motion_sensor_init(void);
bool motion_sensor_test_enable(void);
bool motion_sensor_enable(bool enable);
void motion_sensor_save_config(void);

#endif // NEO_IV_CLOCK_MONTION_SENSOR_H
