#include "oled_ext.h"
#include "cext.h"
#include "logger.h"
#include "oled.h"

static const char * TAG = "OLED_EXT";

void oled_ext_init(void)
{
  NEO_LOGI(TAG, "init");
}

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
  oled_draw_type_t type)
{
  const wchar_t * p = str;
  const uint8_t * font = NULL;
  uint8_t w, h, offset = 0;

  while(p && *p != 0) {
    if((font = mini_font_lookup(*p, (*p & 0xFF00) ? wide_char_font : ascii_font, &w, &h)) != NULL) {
      oled_draw_bitmap(x + offset, y, w, h, font, type);
    } else {
      oled_fill_rect(x + offset, y, w, h, true);
    }
    p ++;
    offset += w;
  }
}

// 在 x, y位置写字符串str, str必须是ascii字符
// ascii_font: ascii字符使用的font
// 如果字库中没有对应字符，打印一个方块
void oled_ext_draw_string(
  uint8_t x, 
  uint8_t y, 
  const char * str, 
  mini_font_type_t ascii_font,
  oled_draw_type_t type)
{
  const char * p = str;
  wchar_t c;
  const uint8_t * font = NULL;
  uint8_t w, h, offset = 0;
  while(p && *p != 0) {
    c = *p;
    if((font = mini_font_lookup(c, ascii_font, &w, &h)) != NULL) {
      oled_draw_bitmap(x + offset, y, w, h, font, type);
    } else {
      oled_fill_rect(x + offset, y, w, h, true);
    }
    p ++;
    offset += w;
  }
}

void oled_ext_draw_progress_bar(
  uint8_t x, 
  uint8_t y,
  uint8_t w, 
  uint8_t h,
  uint8_t progress)
{
  uint8_t pro;

  progress %= 101; // 0 ~ 100

  if(w < 4 || h < 4) return;

  pro = cext_linear_interpolate(0, 0, 100 , w - 4, progress);

  oled_fill_rect(x, y, w, h, true);
  oled_fill_rect(x + 1, y + 1, w - 2, h - 2, false);
  oled_fill_rect(x + 2, y + 2, pro, h - 4, true);
}