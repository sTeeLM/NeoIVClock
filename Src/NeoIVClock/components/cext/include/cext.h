#ifndef NEO_IV_CLOCK_CEXT_H
#define NEO_IV_CLOCK_CEXT_H

#include <stdint.h>

#define cext_is_leap_year(y) \
(( ((y % 100) !=0) && ((y % 4)==0)) || ( (y % 400) == 0))

// 温度单位换算
#define cext_celsius_to_fahrenheit(c) (((float)(c) * 9) / 5 + 32)
#define cext_fahrenheit_to_celsius(f) (((float)(f) - 32) * 5 / 9)

// pa和mmHg的换算，1 mmHg = 133.322 Pa
#define cext_pa_to_mmhg(pa) ((float)(pa) / 133.322f)
#define cext_mmhg_to_pa(mmhg) ((float)(mmhg) * 133.322f)

// pa和atm的换算，1 atm = 101325 Pa
#define cext_pa_to_atm(pa) ((float)(pa) / 101325.0f)
#define cext_atm_to_pa(atm) ((float)(atm) * 101325.0f)


#endif