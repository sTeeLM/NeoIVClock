#ifndef NEO_IV_CLOCK_OLED_EXT_H
#define NEO_IV_CLOCK_OLED_EXT_H

#include <stdint.h>

#include "oled.h"

// 在OLED基础上，提供了汉字，进度条，mini chart等功能

void oled_ext_init(void);

// 在 x，y位置写字符串
void oled_ext_draw_string(uint8_t x, uint8_t y, const char * str, oled_draw_type_t type);

#endif // NEO_IV_CLOCK_OLED_EXT_H
