#ifndef NEO_IV_CLOCK_WS2812B_H
#define NEO_IV_CLOCK_WS2812B_H

#include <stdint.h>
#include <stdbool.h>

void ws2812b_init(void);
void ws2812b_set_pixel(uint8_t red, uint8_t green, uint8_t blue);
void ws2812b_refresh(void);
void ws2812b_clear(void);


// 高级API
// 设置按照某颜色常亮
void ws2812b_set_background(uint8_t red, uint8_t green, uint8_t blue);

// 以某颜色闪烁10ms，然后恢复到之前的颜色
void ws2812b_blink(uint8_t red, uint8_t green, uint8_t blue);
#endif //NEO_IV_CLOCK_WS2812B_H