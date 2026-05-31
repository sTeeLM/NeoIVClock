#include "sm_about.h"
#include "logger.h"
#include "mini_font.h"
#include "oled.h"
#include "oled_ext.h"
#include "oled_ext_icon.h"

#include "sm_func_select.h"

static const char * TAG = "SM_ABOUT";

const char * sm_states_names_about[] = {
  "SM_ABOUT_INIT",
  "SM_ABOUT_SHOW"
};


void do_about_init(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  NEO_LOGD(TAG, "do_about_init");
}

static void do_about_show(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  if(ev == EV_EC11_UP) {
    oled_clear();
    oled_fill_rect(0, 0, 64, 64, true);
    oled_draw_bitmap(0, 0, 64, 64, oled_ext_icon_HEAD, OLED_DRAW_XOR);
    oled_ext_draw_string(72, 0,  "MadCat", MINI_FONT_TYPE_ASCII_8X16, OLED_DRAW_OVERWRITE);
    oled_ext_draw_string(80, 16,  "2026", MINI_FONT_TYPE_ASCII_8X16, OLED_DRAW_OVERWRITE);
    oled_ext_draw_string(72, 32, "steelm", MINI_FONT_TYPE_ASCII_8X16, OLED_DRAW_OVERWRITE);
    oled_redraw_buffer();
  }

}

static const sm_trans_t sm_trans_about_init[] = {
  {EV_EC11_UP, SM_ABOUT, SM_ABOUT_SHOW, do_about_show},
  {0, 0, 0, NULL}
};

static const sm_trans_t sm_trans_about_show[] = {
  {EV_EC11_C, SM_ABOUT , SM_ABOUT_SHOW, do_about_show},
  {EV_EC11_FAST_C, SM_ABOUT , SM_ABOUT_SHOW, do_about_show},
  {EV_EC11_CC, SM_ABOUT , SM_ABOUT_SHOW, do_about_show},
  {EV_EC11_FAST_CC, SM_ABOUT , SM_ABOUT_SHOW, do_about_show},
  {EV_EC11_PRESS, SM_FUNC_SELECT , SM_FUNC_SELECT_INIT, do_func_select_init},
  {0, 0, 0, NULL}
};

const sm_trans_t * sm_trans_about[] = {
  sm_trans_about_init, 
  sm_trans_about_show
};