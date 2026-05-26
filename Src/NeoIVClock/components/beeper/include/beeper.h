#ifndef NEO_IV_CLOCK_BEEPER_H
#define NEO_IV_CLOCK_BEEPER_H

#include <stdbool.h>

void beeper_init(void);
void beeper_beep(void);
void beeper_beep_beep(void);
bool beeper_test_enable(void);
bool beeper_enable(bool enable);
void beeper_save_config();

#endif // NEO_IV_CLOCK_BEEPER_H
