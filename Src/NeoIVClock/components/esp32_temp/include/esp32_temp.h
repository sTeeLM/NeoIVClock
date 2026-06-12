#ifndef NEO_IV_CLOCK_ESP32_TEMP_H
#define NEO_IV_CLOCK_ESP32_TEMP_H

#include <stdint.h>
#include <stdbool.h>

void esp32_temp_init(void);
float esp32_temp_read_data(void);

#endif // NEO_IV_CLOCK_ESP32_TEMP_H
