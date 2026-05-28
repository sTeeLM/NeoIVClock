#ifndef NEO_IV_CLOCK_CEXT_H
#define NEO_IV_CLOCK_CEXT_H

#include <stdint.h>
#include <stdbool.h>

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

// 24小时制的小时转换成12小时制的小时，返回值是是否是下午
bool cext_cal_hour12(uint8_t hour, uint8_t * hour12);

// IIR 滤波
uint16_t cext_iir_uint16(uint16_t oldv, uint16_t newv, uint8_t coe);
float cext_iir_float(float oldv, float newv, uint8_t coe);

// 线性拟合
int32_t cext_linear_interpolate(int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t x);
float cext_linear_interpolate_float(float x1, float y1, float x2, float y2, float x);

/* 无符号时钟代数循环加 N 宏 */
#define cext_ring_add(v, n, minv, maxv) (minv + ((v - minv + n) % (maxv - minv + 1))) 

/* 无符号时钟代数循环减 N 宏 */
#define cext_ring_sub(v, n, minv, maxv) (minv + ((v - minv + ((maxv - minv + 1) - (n % (maxv - minv + 1)))) % (maxv - minv + 1)))

/* 无符号整数上下线 */
#define cext_limit(v, minv, maxv) (v < minv ? minv : (v > maxv ? maxv : v))


#endif