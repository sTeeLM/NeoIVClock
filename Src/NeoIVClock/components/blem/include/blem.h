#ifndef NEO_IV_CLOCK_BLEM_H
#define NEO_IV_CLOCK_BLEM_H

#include <stdint.h>
#include <stdbool.h>

void blem_init(void);
bool blem_report_data(float temp, float hum, float press, uint32_t pm25, uint32_t pm10, float tvoc, float eco2);
bool blem_test_enabled(void);
bool blem_enable(bool enable);
void blem_save_config(void);

#endif //NEO_IV_CLOCK_BLEM_H