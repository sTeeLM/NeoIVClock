#ifndef NEO_IV_CLOCK_OLED_EXT_H
#define NEO_IV_CLOCK_OLED_EXT_H

#include <stdint.h>

#include "oled.h"
#include "mini_font.h"

// 在OLED基础上，提供了汉字，进度条，mini chart等功能

void oled_ext_init(void);

// 在 x, y位置写字符串str, str是宽字符和ascii的混合(中英文混合）
// 英文字符必须占两个字节，
// 高位全零, 低位为ascii码
// 两者可以使用不一样大小的font, 所有字符会顶对齐
// ascii_font: ascii字符使用的font
// wide_char_font: 宽字符使用的font
// 如果字库中没有对应字符，打印一个方块
void oled_ext_draw_wstring(
  uint8_t x, 
  uint8_t y, 
  const wchar_t * str, 
  mini_font_type_t ascii_font,
  mini_font_type_t wide_char_font,
  oled_draw_type_t type);

// 在 x, y位置写字符串str, str必须是ascii字符
// ascii_font: ascii字符使用的font
// 如果字库中没有对应字符，打印一个方块
void oled_ext_draw_string(
  uint8_t x, 
  uint8_t y, 
  const char * str, 
  mini_font_type_t ascii_font,
  oled_draw_type_t type);

#endif // NEO_IV_CLOCK_OLED_EXT_H
