#ifndef NEO_IV_CLOCK_BEEPER_H
#define NEO_IV_CLOCK_BEEPER_H

#include <stdbool.h>

void beeper_init(void);
void beeper_beep(void);
void beeper_beep_beep(void);
bool beep_test_enable(void);

#endif // NEO_IV_CLOCK_BEEPER_H
