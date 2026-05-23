#include "oled_ext.h"
#include "logger.h"
#include "oled.h"
#include "oled_font.h"

static const char * TAG = "OLED_EXT";

void oled_ext_init(void)
{
  NEO_LOGI(TAG, "init");
}

void oled_ext_draw_string(uint8_t x, uint8_t y, const char * str, oled_draw_type_t type)
{
  const char * p = str;
  uint8_t offset = 0;
  while(*p) {
    oled_draw_bitmap(x + offset * 8, y, 8, 16, &oled_ascii_F8X16[(*p - ' ') * 16], type);
    offset ++;
    p++;
  }
}

