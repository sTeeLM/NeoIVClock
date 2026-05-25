#ifndef NEO_IV_CLOCK_SM_COMMON_H
#define NEO_IV_CLOCK_SM_COMMON_H

#include <stdint.h>
#include <stdbool.h>

// 一些状态机的公用代码
void sm_common_reset_timeo(void);
bool sm_common_test_timeo(uint32_t timeo);

#endif // NEO_IV_CLOCK_SM_COMMON_H 