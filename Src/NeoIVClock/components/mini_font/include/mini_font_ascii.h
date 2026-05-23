#ifndef NEO_IV_CLOCK_MINI_FONT_ASCII_H
#define NEO_IV_CLOCK_MINI_FONT_ASCII_H

#include <stdint.h>

#include <mini_font.h>

extern mini_font_node_t mini_font_ascii_font_6x8[];
extern mini_font_node_t mini_font_ascii_font_8x16[];
bool mini_font_ascii_font_6x8_init(mini_font_node_t * lib);
bool mini_font_ascii_font_8x16_init(mini_font_node_t * lib);

#endif // NEO_IV_CLOCK_MINI_FONT_ASCII_H