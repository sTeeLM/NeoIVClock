#include "oled_ext.h"
#include "cext.h"
#include "logger.h"
#include "mini_font.h"
#include "oled.h"
#include "config.h"

static const char * TAG = "OLED_EXT";

#define OLED_EXT_MIN_CONTRAST 0
#define OLED_EXT_MAX_CONTRAST 255
#define OLED_EXT_CONTRAST_FAST_STEP 5
static bool oled_ext_invert;
static uint8_t oled_ext_contrast;

void oled_ext_init(void)
{
  NEO_LOGI(TAG, "init");

  oled_ext_invert = config_read_int("oled_invert");

  oled_ext_contrast = config_read_int("oled_contrast");

  oled_set_inverse(oled_ext_invert);
  oled_set_contrast(oled_ext_contrast);
}

bool oled_ext_test_inverse(void)
{
  return oled_ext_invert;
}
bool oled_ext_set_inverse(bool inverse)
{
  oled_ext_invert = inverse;
  oled_set_inverse(oled_ext_invert);
  return oled_ext_invert;
}

uint8_t oled_ext_get_contrast(void)
{
  return oled_ext_contrast;
}
uint8_t oled_ext_inc_contrast(bool fast)
{
  int16_t val = oled_ext_contrast;
  val = val + (fast ? OLED_EXT_CONTRAST_FAST_STEP : 1);
  if(val > OLED_EXT_MAX_CONTRAST)
    val = OLED_EXT_MAX_CONTRAST;
  oled_ext_contrast = val;
  oled_set_contrast(oled_ext_contrast);
  return oled_ext_contrast;
}

uint8_t oled_ext_dec_contrast(bool fast)
{
  int16_t val = oled_ext_contrast;
  val = val - (fast ? OLED_EXT_CONTRAST_FAST_STEP : 1);
  if(val < OLED_EXT_MIN_CONTRAST)
    val = OLED_EXT_MIN_CONTRAST;
  oled_ext_contrast = val;
  oled_set_contrast(oled_ext_contrast);
  return oled_ext_contrast;
}

void oled_ext_save_config(void)
{
  config_write_int("oled_invert", oled_ext_invert ? 1 : 0);
  config_write_int("oled_contrast", oled_ext_contrast);  
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

void oled_ext_print_progress(uint8_t progress, const wchar_t * info)
{
  oled_clear();
  oled_ext_draw_wstring(16, 2, L"IVClock 2.0", MINI_FONT_TYPE_ASCII_8X16, MINI_FONT_TYPE_CHINESE_16X16, OLED_DRAW_OVERWRITE);
  oled_ext_draw_wstring(0, 18, info, MINI_FONT_TYPE_ASCII_8X16, MINI_FONT_TYPE_CHINESE_16X16, OLED_DRAW_OVERWRITE);
  oled_ext_draw_progress_bar(0, 40, 128, 16, progress);
  oled_redraw_buffer();
}