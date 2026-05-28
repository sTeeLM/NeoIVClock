#include "sm_clock.h"
#include "config.h"
#include "mini_font.h"
#include "task.h"
#include "sm.h"
#include "logger.h"
#include "cext.h"

#include "clock.h"
#include "iv18.h"
#include "oled.h"
#include "oled_ext.h"
#include "oled_ext_icon.h"
#include "sensor_data.h"

#include "sm_func_select.h"


static const char * TAG = "SM_CLOCK";

const char * sm_states_names_clock[] = {
  "SM_CLOCK_INIT",
  "SM_CLOCK_TIME_A",
  "SM_CLOCK_TIME_B",
  "SM_CLOCK_DATE_A",
  "SM_CLOCK_DATE_B"
};

static void sm_clock_update_oled(bool is_oled_a)
{
  char str_buf[16];
  float fbuf;
  uint16_t ibuf;
  sensor_data_temp_unit_t temp_unit = sensor_data_get_temp_unit();
  sensor_data_press_unit_t press_unit = sensor_data_get_press_unit();

  if(is_oled_a) {
    // 温度、湿度、气压
    sensor_data_get_temp(&fbuf);
    snprintf(str_buf, sizeof(str_buf), "%6.2f",  temp_unit == SENSOR_DATA_TEMP_UNIT_SHESHI ? fbuf : cext_celsius_to_fahrenheit(fbuf));
    str_buf[6] = 0;
    oled_ext_draw_string(43, 2, str_buf, MINI_FONT_TYPE_ASCII_8X16, OLED_DRAW_OVERWRITE);

    sensor_data_get_mol(&fbuf);
    snprintf(str_buf, sizeof(str_buf), "%6.2f",  fbuf); 
    str_buf[6] = 0;
    oled_ext_draw_string(43, 22, str_buf, MINI_FONT_TYPE_ASCII_8X16, OLED_DRAW_OVERWRITE);

    sensor_data_get_press(&fbuf);
    switch (press_unit) {
      case SENSOR_DATA_PRESS_UNIT_HPA:
        snprintf(str_buf, sizeof(str_buf), "%6.2f",  fbuf);
        break;
      case SENSOR_DATA_PRESS_UNIT_HGMM:
        snprintf(str_buf, sizeof(str_buf), "%6.2f",fbuf);
        break;
      case SENSOR_DATA_PRESS_UNIT_ATM:
        snprintf(str_buf, sizeof(str_buf), "%9.7f",fbuf);
        break;
      default:;
    }
    str_buf[6] = 0;
    oled_ext_draw_string(43, 42, str_buf, MINI_FONT_TYPE_ASCII_8X16, OLED_DRAW_OVERWRITE);
    NEO_LOGD(TAG, "press: %s", str_buf);

  } else {
    // PM2.5、TVOC、甲醛
    sensor_data_get_pm25(&ibuf);
    snprintf(str_buf, sizeof(str_buf), "%6d",  ibuf);
    str_buf[6] = 0;
    oled_ext_draw_string(43, 2, str_buf, MINI_FONT_TYPE_ASCII_8X16, OLED_DRAW_OVERWRITE);

    sensor_data_get_tvoc(&fbuf);
    snprintf(str_buf, sizeof(str_buf), "%6.4f",  fbuf); 
    str_buf[6] = 0;
    oled_ext_draw_string(43, 22, str_buf, MINI_FONT_TYPE_ASCII_8X16, OLED_DRAW_OVERWRITE);

    sensor_data_get_form(&fbuf);
    snprintf(str_buf, sizeof(str_buf), "%6.4f",  fbuf);
    str_buf[6] = 0;
    oled_ext_draw_string(43, 42, str_buf, MINI_FONT_TYPE_ASCII_8X16, OLED_DRAW_OVERWRITE);
  }

  oled_redraw_buffer();
}

static void sm_clock_draw_oled(bool is_oled_a)
{
  sensor_data_temp_unit_t temp_unit = sensor_data_get_temp_unit();
  sensor_data_press_unit_t press_unit = sensor_data_get_press_unit();

  oled_clear();
  if(is_oled_a) {
    // 温度、湿度、气压
    oled_ext_draw_wstring(0, 2, L"温度", MINI_FONT_TYPE_ASCII_8X16, MINI_FONT_TYPE_CHINESE_16X16, OLED_DRAW_OVERWRITE);
    oled_draw_bitmap(94, 2, 30, 20,  temp_unit == SENSOR_DATA_TEMP_UNIT_SHESHI ? 
      oled_ext_char_SHESHI : oled_ext_char_HUASHI, OLED_DRAW_OVERWRITE);

    oled_ext_draw_wstring(0, 22, L"湿度", MINI_FONT_TYPE_ASCII_8X16, MINI_FONT_TYPE_CHINESE_16X16, OLED_DRAW_OVERWRITE);
    oled_draw_bitmap(94, 22, 30, 20, oled_ext_char_PERCENT, OLED_DRAW_OVERWRITE);

    oled_ext_draw_wstring(0, 42, L"气压", MINI_FONT_TYPE_ASCII_8X16, MINI_FONT_TYPE_CHINESE_16X16, OLED_DRAW_OVERWRITE);
    oled_draw_bitmap(94, 42, 30, 20, press_unit == SENSOR_DATA_PRESS_UNIT_HPA ? oled_ext_char_HPA :
       (press_unit == SENSOR_DATA_PRESS_UNIT_HGMM ? oled_ext_char_HGM : oled_ext_char_ATM), OLED_DRAW_OVERWRITE);

  } else {
    // PM2.5、TVOC、甲醛
    oled_ext_draw_wstring(0, 2, L"PM2.5", MINI_FONT_TYPE_ASCII_8X16, MINI_FONT_TYPE_CHINESE_16X16, OLED_DRAW_OVERWRITE);
    // PM2.5 没有单位需要打印

    oled_ext_draw_wstring(0, 22, L"TVOC", MINI_FONT_TYPE_ASCII_8X16, MINI_FONT_TYPE_CHINESE_16X16, OLED_DRAW_OVERWRITE);
    oled_draw_bitmap(94, 22, 30, 20, oled_ext_char_MGM3, OLED_DRAW_OVERWRITE);

    oled_ext_draw_wstring(0, 42, L"甲醛", MINI_FONT_TYPE_ASCII_8X16, MINI_FONT_TYPE_CHINESE_16X16, OLED_DRAW_OVERWRITE);
    oled_draw_bitmap(94, 42, 30, 20, oled_ext_char_MGM3, OLED_DRAW_OVERWRITE);
  }
 
  sm_clock_update_oled(is_oled_a);
}

static void sm_do_clock(bool is_time, bool is_oled_a, task_event_t ev)
{
  if(ev == EV_SENSOR_UPDATE) {
    // update oled
    sm_clock_update_oled(is_oled_a);
  } else if (ev == EV_1S) {
    // test timeo
    iv18_test_ps_timeo();
  } else if (ev == EV_ACC) {
    iv18_reset_ps_timeo();
  } else {
    iv18_reset_ps_timeo();
    clock_set_display_mode(is_time ? CLOCK_DISPLAY_MODE_TIME : CLOCK_DISPLAY_MODE_DATE);
    sm_clock_draw_oled(is_oled_a);
  }
}

void do_clock_init(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  NEO_LOGD(TAG, "do_clock_init");

  iv18_clr();
  iv18_reset_ps_timeo();
  clock_set_display_mode(CLOCK_DISPLAY_MODE_TIME);
  // draw oled
  sm_clock_draw_oled(true);
}

static void do_clock_time_a(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  sm_do_clock(true, true, ev);
}

static void do_clock_time_b(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  sm_do_clock(true, false, ev);
}

static void do_clock_date_a(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  sm_do_clock(false, true, ev);
}

static void do_clock_date_b(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  sm_do_clock(false, false, ev);
}

static const sm_trans_t sm_trans_clock_init[] = {
  {EV_EC11_UP, SM_CLOCK , SM_CLOCK_TIME_A, do_clock_init},
  {0, 0, 0, NULL}
};

static const sm_trans_t sm_trans_clock_time_a[] = {
  {EV_SENSOR_UPDATE, SM_CLOCK , SM_CLOCK_TIME_A, do_clock_time_a},
  {EV_1S, SM_CLOCK , SM_CLOCK_TIME_A, do_clock_time_a}, 
  {EV_ACC, SM_CLOCK , SM_CLOCK_TIME_A, do_clock_time_a}, 
  {EV_EC11_C, SM_CLOCK , SM_CLOCK_TIME_B, do_clock_time_b},
  {EV_EC11_FAST_C, SM_CLOCK , SM_CLOCK_TIME_B, do_clock_time_b},
  {EV_EC11_CC, SM_CLOCK , SM_CLOCK_DATE_B, do_clock_date_b},
  {EV_EC11_FAST_CC, SM_CLOCK , SM_CLOCK_DATE_B, do_clock_date_b},
  {EV_EC11_PRESS, SM_FUNC_SELECT , SM_FUNC_SELECT_INIT, do_func_select_init},
  {0, 0, 0, NULL}
};

static const sm_trans_t sm_trans_clock_time_b[] = {
  {EV_SENSOR_UPDATE, SM_CLOCK , SM_CLOCK_TIME_B, do_clock_time_b},
  {EV_1S, SM_CLOCK , SM_CLOCK_TIME_B, do_clock_time_b}, 
  {EV_ACC, SM_CLOCK , SM_CLOCK_TIME_B, do_clock_time_b},
  {EV_EC11_C, SM_CLOCK , SM_CLOCK_DATE_A, do_clock_date_a},
  {EV_EC11_FAST_C, SM_CLOCK , SM_CLOCK_DATE_A, do_clock_date_a},
  {EV_EC11_CC, SM_CLOCK , SM_CLOCK_TIME_A, do_clock_time_a},
  {EV_EC11_FAST_CC, SM_CLOCK , SM_CLOCK_TIME_A, do_clock_time_a},
  {EV_EC11_PRESS, SM_FUNC_SELECT , SM_FUNC_SELECT_INIT, do_func_select_init},
  {0, 0, 0, NULL}
};

static const sm_trans_t sm_trans_clock_date_a[] = {
  {EV_SENSOR_UPDATE, SM_CLOCK , SM_CLOCK_DATE_A, do_clock_date_a},
  {EV_1S, SM_CLOCK , SM_CLOCK_DATE_A, do_clock_date_a},
  {EV_ACC, SM_CLOCK , SM_CLOCK_DATE_A, do_clock_date_a},  
  {EV_EC11_C, SM_CLOCK , SM_CLOCK_DATE_B, do_clock_date_b},
  {EV_EC11_FAST_C, SM_CLOCK , SM_CLOCK_DATE_B, do_clock_date_b},
  {EV_EC11_CC, SM_CLOCK , SM_CLOCK_TIME_B, do_clock_time_b},
  {EV_EC11_FAST_CC, SM_CLOCK , SM_CLOCK_TIME_B, do_clock_time_b},
  {EV_EC11_PRESS, SM_FUNC_SELECT , SM_FUNC_SELECT_INIT, do_func_select_init},
  {0, 0, 0, NULL}
};

static const sm_trans_t sm_trans_clock_date_b[] = {
  {EV_SENSOR_UPDATE, SM_CLOCK , SM_CLOCK_DATE_B, do_clock_date_b},
  {EV_1S, SM_CLOCK , SM_CLOCK_DATE_B, do_clock_date_b},
  {EV_ACC, SM_CLOCK , SM_CLOCK_DATE_B, do_clock_date_b},    
  {EV_EC11_C, SM_CLOCK , SM_CLOCK_TIME_A, do_clock_time_a},
  {EV_EC11_FAST_C, SM_CLOCK , SM_CLOCK_TIME_A, do_clock_time_a}, 
  {EV_EC11_CC, SM_CLOCK , SM_CLOCK_DATE_A, do_clock_date_a},
  {EV_EC11_FAST_CC, SM_CLOCK , SM_CLOCK_DATE_A, do_clock_date_a},
  {EV_EC11_PRESS, SM_FUNC_SELECT , SM_FUNC_SELECT_INIT, do_func_select_init},  
  {0, 0, 0, NULL}
};

const sm_trans_t * sm_trans_clock[] = {
  sm_trans_clock_init,
  sm_trans_clock_time_a,
  sm_trans_clock_time_b,
  sm_trans_clock_date_a,
  sm_trans_clock_date_b,    
};
