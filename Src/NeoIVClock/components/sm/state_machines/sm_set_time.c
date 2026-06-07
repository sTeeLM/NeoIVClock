#include "sm_set_time.h"
#include "task.h"
#include "sm.h"
#include "logger.h"
#include "clock.h"
#include "iv18.h"
#include "oled.h"
#include "oled_ext.h"

#include "sm_common.h"
#include "sm_func_select.h"

const char * sm_states_names_set_time[] = {
  "SM_SET_TIME_INIT",
  "SM_SET_TIME_SEL",
  "SM_SET_TIME_HOUR",
  "SM_SET_TIME_MIN", 
  "SM_SET_TIME_SEC"
};

static const char * TAG = "SM_SET_TIME";

#define SM_SET_TIME_BLINK_TIMEO 1

// 菜单中的选项
static uint8_t sm_set_time_sel_index;
// 0 hour
// 1 min
// 2 sec
// 3 exit
static wchar_t * sm_set_time_sel_name[] = 
{
  L"设置小时",
  L"设置分钟",
  L"设置秒",
  L"退出",    
};
static void sm_set_time_draw_select(uint8_t index)
{
  uint8_t first, i;


  iv18_clr_blink(1);
  iv18_clr_blink(2);
  iv18_clr_blink(4);
  iv18_clr_blink(5);  
  iv18_clr_blink(7);
  iv18_clr_blink(8); 

  switch(index) {
    case 0:
      iv18_set_blink(1);
      iv18_set_blink(2);
    break;
    case 1:
      iv18_set_blink(4);
      iv18_set_blink(5);
    break;
    case 2:
      iv18_set_blink(7);
      iv18_set_blink(8);
    break;    
    default:;
  }

  // 绘制滚动菜单
  oled_clear();

  first = (index + 3) % 4;
  oled_fill_rect(0,  16 , 128, 16, true);

  for(i = 0 ; i < 4 ; i ++) {
    oled_ext_draw_wstring(0, i * 16, sm_set_time_sel_name[first], MINI_FONT_TYPE_ASCII_8X16, MINI_FONT_TYPE_CHINESE_16X16, OLED_DRAW_XOR);
    first = (first + 1) % 4;
  } 
  oled_redraw_buffer(); 
}

// 0 hour
// 1 min
// 2 sec
static void sm_set_time_draw_time(uint8_t index)
{
  // 绘制帮助
  oled_clear();
  oled_ext_draw_wstring(0, 0, sm_set_time_sel_name[index], MINI_FONT_TYPE_ASCII_8X16, MINI_FONT_TYPE_CHINESE_16X16, OLED_DRAW_XOR);
  oled_ext_draw_wstring(0, 16, L"旋转调整", MINI_FONT_TYPE_ASCII_8X16, MINI_FONT_TYPE_CHINESE_16X16, OLED_DRAW_XOR);
  oled_ext_draw_wstring(0, 32, L"按下确认", MINI_FONT_TYPE_ASCII_8X16, MINI_FONT_TYPE_CHINESE_16X16, OLED_DRAW_XOR); 
  oled_redraw_buffer(); 
}

void do_set_time_init(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  NEO_LOGD(TAG, "do_set_time_init");

  // 将IV18设置为Timer状态
  iv18_reset_ps_timeo();
  iv18_clr();
  clock_set_display_mode(CLOCK_DISPLAY_MODE_TIME);
  sm_common_reset_timeo();
  sm_set_time_sel_index = 0;
}


static void do_set_time_sel(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  if(ev == EV_EC11_UP || ev == EV_V1 || ev == EV_EC11_C || ev == EV_EC11_FAST_C || ev == EV_EC11_CC || ev == EV_EC11_FAST_CC) {
    if(ev == EV_EC11_C || ev == EV_EC11_FAST_C) {
      sm_set_time_sel_index = (sm_set_time_sel_index + 1) % 4;
    } else if(ev == EV_EC11_CC || ev == EV_EC11_FAST_CC) {
      sm_set_time_sel_index = (sm_set_time_sel_index + 3) % 4;
    }
    sm_set_time_draw_select(sm_set_time_sel_index);
  } else if (ev == EV_EC11_PRESS) {
    switch(sm_set_time_sel_index) {
      case 0: task_set(EV_V2); break; // hour
      case 1: task_set(EV_V3); break; // min
      case 2: task_set(EV_V4); break; // sec
      case 3: task_set(EV_V1); break; // exit
    }
  }
}

static void do_set_time_hour(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  if(ev == EV_V2 || ev == EV_EC11_C || ev == EV_EC11_FAST_C || ev == EV_EC11_CC || ev == EV_EC11_FAST_CC) {
    if(ev == EV_EC11_C || ev == EV_EC11_FAST_C) {
      clock_inc_hour(ev == EV_EC11_FAST_C);
    } else if(ev == EV_EC11_CC || ev == EV_EC11_FAST_CC) {
      clock_dec_hour(ev == EV_EC11_FAST_CC);
    }
    if(ev == EV_V2) {
      iv18_set_blink(1);
      iv18_set_blink(2);
    } else {
      sm_common_reset_timeo();
      iv18_clr_blink(1);
      iv18_clr_blink(2);
    }
    sm_set_time_draw_time(0);
  } else if(ev == EV_EC11_PRESS) {
    clock_sync_to_rtc(CLOCK_SYNC_TIME);
    clock_sync_to_local();
    task_set(EV_V1); // quit back to SM_SET_TIME_SEL
  } else if(ev == EV_1S) {
    if(sm_common_test_timeo(SM_SET_TIME_BLINK_TIMEO)) {
      iv18_set_blink(1);
      iv18_set_blink(2);
    }
  }
}

static void do_set_time_min(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  if(ev == EV_V3 || ev == EV_EC11_C || ev == EV_EC11_FAST_C || ev == EV_EC11_CC || ev == EV_EC11_FAST_CC) {
    if(ev == EV_EC11_C || ev == EV_EC11_FAST_C) {
      clock_inc_min(ev == EV_EC11_FAST_C);
    } else if(ev == EV_EC11_CC || ev == EV_EC11_FAST_CC) {
      clock_dec_min(ev == EV_EC11_FAST_CC);
    }
    if(ev == EV_V3) {
      iv18_set_blink(4);
      iv18_set_blink(5);
    } else {
      sm_common_reset_timeo();
      iv18_clr_blink(4);
      iv18_clr_blink(5);
    }
    sm_set_time_draw_time(1);
  } else if(ev == EV_EC11_PRESS) {
    clock_sync_to_rtc(CLOCK_SYNC_TIME);
    clock_sync_to_local();
    task_set(EV_V1); // quit back to SM_SET_TIME_SEL
  } else if(ev == EV_1S) {
    if(sm_common_test_timeo(SM_SET_TIME_BLINK_TIMEO)) {
      iv18_set_blink(4);
      iv18_set_blink(5);
    }
  }
}

static void do_set_time_sec(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  if(ev == EV_V4 || ev == EV_EC11_C || ev == EV_EC11_FAST_C || ev == EV_EC11_CC || ev == EV_EC11_FAST_CC) {
    if(ev == EV_EC11_C || ev == EV_EC11_FAST_C) {
      clock_clr_sec();
    } else if(ev == EV_EC11_CC || ev == EV_EC11_FAST_CC) {
      clock_clr_sec();
    }
    if(ev == EV_V4) {
      iv18_set_blink(7);
      iv18_set_blink(8);
    } else {
      sm_common_reset_timeo();
      iv18_clr_blink(7);
      iv18_clr_blink(8);
    }
    sm_set_time_draw_time(2);
  } else if(ev == EV_EC11_PRESS) {
    clock_sync_to_rtc(CLOCK_SYNC_TIME);
    clock_sync_to_local();
    task_set(EV_V1); // quit back to SM_SET_TIME_SEL
  } else if(ev == EV_1S) {
    if(sm_common_test_timeo(SM_SET_TIME_BLINK_TIMEO)) {
      iv18_set_blink(7);
      iv18_set_blink(8);
    }
  }
}

static const sm_trans_t sm_trans_set_time_init[] = {
  {EV_EC11_UP, SM_SET_TIME, SM_SET_TIME_SEL, do_set_time_sel},
  {0, 0, 0, NULL}
};

static const sm_trans_t sm_trans_set_time_sel[] = {
  {EV_EC11_C, SM_SET_TIME, SM_SET_TIME_SEL, do_set_time_sel},
  {EV_EC11_FAST_C, SM_SET_TIME, SM_SET_TIME_SEL, do_set_time_sel},
  {EV_EC11_CC, SM_SET_TIME, SM_SET_TIME_SEL, do_set_time_sel},
  {EV_EC11_FAST_CC, SM_SET_TIME, SM_SET_TIME_SEL, do_set_time_sel},
  {EV_EC11_PRESS, SM_SET_TIME, SM_SET_TIME_SEL, do_set_time_sel},
  {EV_V1, SM_FUNC_SELECT, SM_FUNC_SELECT_INIT, do_func_select_init}, 
  {EV_V2, SM_SET_TIME, SM_SET_TIME_HOUR, do_set_time_hour},   
  {EV_V3, SM_SET_TIME, SM_SET_TIME_MIN, do_set_time_min},
  {EV_V4, SM_SET_TIME, SM_SET_TIME_SEC, do_set_time_sec},    
  {0, 0, 0, NULL}
};

static const sm_trans_t sm_trans_set_time_hour[] = {
  {EV_EC11_C, SM_SET_TIME, SM_SET_TIME_HOUR, do_set_time_hour},
  {EV_EC11_FAST_C, SM_SET_TIME, SM_SET_TIME_HOUR, do_set_time_hour},
  {EV_EC11_CC, SM_SET_TIME, SM_SET_TIME_HOUR, do_set_time_hour},
  {EV_EC11_FAST_CC, SM_SET_TIME, SM_SET_TIME_HOUR, do_set_time_hour},
  {EV_1S, SM_SET_TIME, SM_SET_TIME_HOUR, do_set_time_hour},  
  {EV_EC11_PRESS, SM_SET_TIME, SM_SET_TIME_HOUR, do_set_time_hour},
  {EV_V1, SM_SET_TIME, SM_SET_TIME_SEL, do_set_time_sel},
  {0, 0, 0, NULL}
};

static const sm_trans_t sm_trans_set_time_min[] = {
  {EV_EC11_C, SM_SET_TIME, SM_SET_TIME_MIN, do_set_time_min},
  {EV_EC11_FAST_C, SM_SET_TIME, SM_SET_TIME_MIN, do_set_time_min},
  {EV_EC11_CC, SM_SET_TIME, SM_SET_TIME_MIN, do_set_time_min},
  {EV_EC11_FAST_CC, SM_SET_TIME, SM_SET_TIME_MIN, do_set_time_min},
  {EV_EC11_PRESS, SM_SET_TIME, SM_SET_TIME_MIN, do_set_time_min}, 
  {EV_1S, SM_SET_TIME, SM_SET_TIME_MIN, do_set_time_min},    
  {EV_V1, SM_SET_TIME, SM_SET_TIME_SEL, do_set_time_sel},  
  {0, 0, 0, NULL}
};

static const sm_trans_t sm_trans_set_time_sec[] = {
  {EV_EC11_C, SM_SET_TIME, SM_SET_TIME_SEC, do_set_time_sec},
  {EV_EC11_FAST_C, SM_SET_TIME, SM_SET_TIME_SEC, do_set_time_sec},
  {EV_EC11_CC, SM_SET_TIME, SM_SET_TIME_SEC, do_set_time_sec},
  {EV_EC11_FAST_CC, SM_SET_TIME, SM_SET_TIME_SEC, do_set_time_sec},
  {EV_EC11_PRESS, SM_SET_TIME, SM_SET_TIME_SEC, do_set_time_sec}, 
  {EV_1S, SM_SET_TIME, SM_SET_TIME_SEC, do_set_time_sec},   
  {EV_V1, SM_SET_TIME, SM_SET_TIME_SEL, do_set_time_sel},    
  {0, 0, 0, NULL}
};

const sm_trans_t * sm_trans_set_time[] = {
  sm_trans_set_time_init,
  sm_trans_set_time_sel,
  sm_trans_set_time_hour,
  sm_trans_set_time_min,
  sm_trans_set_time_sec
};
