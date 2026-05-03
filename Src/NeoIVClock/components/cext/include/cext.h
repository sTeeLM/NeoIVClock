#ifndef NEO_IV_CLOCK_CEXT_H
#define NEO_IV_CLOCK_CEXT_H

#include <stdint.h>

#define cext_is_leap_year(y) \
(( ((y % 100) !=0) && ((y % 4)==0)) || ( (y % 400) == 0))


#endif