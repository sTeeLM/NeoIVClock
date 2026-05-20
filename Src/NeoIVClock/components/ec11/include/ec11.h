#ifndef NEO_IV_CLOCK_EC11_H
#define NEO_IV_CLOCK_EC11_H

#include "task.h"

#include <stdint.h>
#include <stdbool.h>

void ec11_init(void);
bool ec11_is_factory_reset(void);
void ec11_scan_proc(task_event_t ev);
void ec11_key_proc(task_event_t ev);

#endif // NEO_IV_CLOCK_EC11_H
