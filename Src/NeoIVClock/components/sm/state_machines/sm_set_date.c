#include "sm_set_date.h"
#include "task.h"
#include "sm.h"
#include "logger.h"
#include "clock.h"
#include "iv18.h"
#include "oled.h"
#include "oled_ext.h"

#include "sm_common.h"
#include "sm_func_select.h"

#define SM_SET_DATE_BLINK_TIMEO 1

const char * sm_states_names_set_date[] = {
  "SM_SET_DATE_INIT",
  "SM_SET_DATE_SEL",
  "SM_SET_DATE_YEAR",
  "SM_SET_DATE_MON",
  "SM_SET_DATE_DATE"  
};

// 菜单中的选项
static uint8_t sm_set_date_sel_index;
// 0 year
// 1 month
// 2 date
// 3 exit
static wchar_t * sm_set_date_sel_name[] = 
{
  L"设置年份",
  L"设置月份",
  L"设置日期",
  L"退出",    
};

static void sm_set_date_draw_select(uint8_t index)
{
  uint8_t first, i;

  iv18_clr_blink(1);
  iv18_clr_blink(2);
  iv18_clr_blink(3);
  iv18_clr_blink(4);
  iv18_clr_blink(5);
  iv18_clr_blink(6);  
  iv18_clr_blink(7);
  iv18_clr_blink(8); 

  switch(index) {
    case 0:
      iv18_set_blink(1);
      iv18_set_blink(2);
      iv18_set_blink(3);
      iv18_set_blink(4);
    break;
    case 1:
      iv18_set_blink(5);
      iv18_set_blink(6);
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
    oled_ext_draw_wstring(0, i * 16, sm_set_date_sel_name[first], MINI_FONT_TYPE_ASCII_8X16, MINI_FONT_TYPE_CHINESE_16X16, OLED_DRAW_XOR);
    first = (first + 1) % 4;
  } 
  oled_redraw_buffer(); 
}

static void sm_set_date_draw_date(uint8_t index)
{
  // 绘制帮助
  oled_clear();
  oled_ext_draw_wstring(0, 0, sm_set_date_sel_name[index], MINI_FONT_TYPE_ASCII_8X16, MINI_FONT_TYPE_CHINESE_16X16, OLED_DRAW_XOR);
  oled_ext_draw_wstring(0, 16, L"旋转调整", MINI_FONT_TYPE_ASCII_8X16, MINI_FONT_TYPE_CHINESE_16X16, OLED_DRAW_XOR);
  oled_ext_draw_wstring(0, 32, L"按下确认", MINI_FONT_TYPE_ASCII_8X16, MINI_FONT_TYPE_CHINESE_16X16, OLED_DRAW_XOR); 
  oled_redraw_buffer(); 
}

void do_set_date_init(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  // 将IV18设置为日期设置状态
  iv18_reset_ps_timeo();
  iv18_clr();
  clock_set_display_mode(CLOCK_DISPLAY_MODE_COMPOUND_DATE);
  sm_common_reset_timeo();
  sm_set_date_sel_index = 0;
}

static void do_set_date_sel(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  if(ev == EV_EC11_UP || ev == EV_V1 || ev == EV_EC11_C || ev == EV_EC11_FAST_C || ev == EV_EC11_CC || ev == EV_EC11_FAST_CC) {
    if(ev == EV_EC11_C || ev == EV_EC11_FAST_C) {
      sm_set_date_sel_index = (sm_set_date_sel_index + 1) % 4;
    } else if(ev == EV_EC11_CC || ev == EV_EC11_FAST_CC) {
      sm_set_date_sel_index = (sm_set_date_sel_index + 3) % 4;
    }
    sm_set_date_draw_select(sm_set_date_sel_index);
  } else if (ev == EV_EC11_PRESS) {
    switch(sm_set_date_sel_index) {
      case 0: task_set(EV_V2); break; // year
      case 1: task_set(EV_V3); break; // month
      case 2: task_set(EV_V4); break; // date
      case 3: task_set(EV_V1); break; // exit
    }
  }
}

static void do_set_date_year(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  if(ev == EV_V2 || ev == EV_EC11_C || ev == EV_EC11_FAST_C || ev == EV_EC11_CC || ev == EV_EC11_FAST_CC) {
    if(ev == EV_EC11_C || ev == EV_EC11_FAST_C) {
      clock_inc_year(ev == EV_EC11_FAST_C);
    } else if(ev == EV_EC11_CC || ev == EV_EC11_FAST_CC) {
      clock_dec_year(ev == EV_EC11_FAST_CC);
    }
    if(ev == EV_V2) {
      iv18_set_blink(1);
      iv18_set_blink(2);
      iv18_set_blink(3);
      iv18_set_blink(4);
    } else {
      sm_common_reset_timeo();
      iv18_clr_blink(1);
      iv18_clr_blink(2);
      iv18_clr_blink(3);
      iv18_clr_blink(4);      
    }
    sm_set_date_draw_date(0);
  } else if(ev == EV_EC11_PRESS) {
    clock_sync_to_rtc(CLOCK_SYNC_DATE);
    clock_sync_to_local();
    task_set(EV_V1); // quit back to SM_SET_DATE_SEL
  } else if(ev == EV_1S) {
    if(sm_common_test_timeo(SM_SET_DATE_BLINK_TIMEO)) {
      iv18_set_blink(1);
      iv18_set_blink(2);
      iv18_set_blink(3);
      iv18_set_blink(4);
    }
  }
}

static void do_set_date_mon(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  if(ev == EV_V3 || ev == EV_EC11_C || ev == EV_EC11_FAST_C || ev == EV_EC11_CC || ev == EV_EC11_FAST_CC) {
    if(ev == EV_EC11_C || ev == EV_EC11_FAST_C) {
      clock_inc_month();
    } else if(ev == EV_EC11_CC || ev == EV_EC11_FAST_CC) {
      clock_dec_month();
    }
    if(ev == EV_V3) {
      iv18_set_blink(5);
      iv18_set_blink(6);
    } else {
      sm_common_reset_timeo();
      iv18_clr_blink(5);
      iv18_clr_blink(6);
    }
    sm_set_date_draw_date(1);
  } else if(ev == EV_EC11_PRESS) {
    clock_sync_to_rtc(CLOCK_SYNC_DATE);
    clock_sync_to_local();
    task_set(EV_V1); // quit back to SM_SET_TIME_SEL
  } else if(ev == EV_1S) {
    if(sm_common_test_timeo(SM_SET_DATE_BLINK_TIMEO)) {
      iv18_set_blink(5);
      iv18_set_blink(6);
    }
  }
}

static void do_set_date_date(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  if(ev == EV_V4 || ev == EV_EC11_C || ev == EV_EC11_FAST_C || ev == EV_EC11_CC || ev == EV_EC11_FAST_CC) {
    if(ev == EV_EC11_C || ev == EV_EC11_FAST_C) {
      clock_inc_date(ev == EV_EC11_FAST_C);
    } else if(ev == EV_EC11_CC || ev == EV_EC11_FAST_CC) {
      clock_dec_date(ev == EV_EC11_FAST_CC);
    }
    if(ev == EV_V4) {
      iv18_set_blink(7);
      iv18_set_blink(8);
    } else {
      sm_common_reset_timeo();
      iv18_clr_blink(7);
      iv18_clr_blink(8);
    }
    sm_set_date_draw_date(2);
  } else if(ev == EV_EC11_PRESS) {
    clock_sync_to_rtc(CLOCK_SYNC_DATE);
    clock_sync_to_local();
    task_set(EV_V1); // quit back to SM_SET_TIME_SEL
  } else if(ev == EV_1S) {
    if(sm_common_test_timeo(SM_SET_DATE_BLINK_TIMEO)) {
      iv18_set_blink(7);
      iv18_set_blink(8);
    }
  }
}

static const sm_trans_t sm_trans_set_date_init[] = {
  {EV_EC11_UP, SM_SET_DATE, SM_SET_DATE_SEL, do_set_date_sel},
  {0, 0, 0, NULL}
};

static const sm_trans_t sm_trans_set_date_sel[] = {
  {EV_EC11_C, SM_SET_DATE, SM_SET_DATE_SEL, do_set_date_sel},
  {EV_EC11_FAST_C, SM_SET_DATE, SM_SET_DATE_SEL, do_set_date_sel},
  {EV_EC11_CC, SM_SET_DATE, SM_SET_DATE_SEL, do_set_date_sel},
  {EV_EC11_FAST_CC, SM_SET_DATE, SM_SET_DATE_SEL, do_set_date_sel},
  {EV_EC11_PRESS, SM_SET_DATE, SM_SET_DATE_SEL, do_set_date_sel},
  {EV_V1, SM_FUNC_SELECT, SM_FUNC_SELECT_INIT, do_func_select_init}, 
  {EV_V2, SM_SET_DATE, SM_SET_DATE_YEAR, do_set_date_year},   
  {EV_V3, SM_SET_DATE, SM_SET_DATE_MON, do_set_date_mon},
  {EV_V4, SM_SET_DATE, SM_SET_DATE_DATE, do_set_date_date},    
  {0, 0, 0, NULL}
};

static const sm_trans_t sm_trans_set_date_year[] = {
  {EV_EC11_C, SM_SET_DATE, SM_SET_DATE_YEAR, do_set_date_year},
  {EV_EC11_FAST_C, SM_SET_DATE, SM_SET_DATE_YEAR, do_set_date_year},
  {EV_EC11_CC, SM_SET_DATE, SM_SET_DATE_YEAR, do_set_date_year},
  {EV_EC11_FAST_CC, SM_SET_DATE, SM_SET_DATE_YEAR, do_set_date_year},
  {EV_1S, SM_SET_DATE, SM_SET_DATE_YEAR, do_set_date_year},  
  {EV_EC11_PRESS, SM_SET_DATE, SM_SET_DATE_YEAR, do_set_date_year},
  {EV_V1, SM_SET_DATE, SM_SET_DATE_SEL, do_set_date_sel},
  {0, 0, 0, NULL}
};

static const sm_trans_t sm_trans_set_date_mon[] = {
  {EV_EC11_C, SM_SET_DATE, SM_SET_DATE_MON, do_set_date_mon},
  {EV_EC11_FAST_C, SM_SET_DATE, SM_SET_DATE_MON, do_set_date_mon},
  {EV_EC11_CC, SM_SET_DATE, SM_SET_DATE_MON, do_set_date_mon},
  {EV_EC11_FAST_CC, SM_SET_DATE, SM_SET_DATE_MON, do_set_date_mon},
  {EV_1S, SM_SET_DATE, SM_SET_DATE_MON, do_set_date_mon},  
  {EV_EC11_PRESS, SM_SET_DATE, SM_SET_DATE_MON, do_set_date_mon},
  {EV_V1, SM_SET_DATE, SM_SET_DATE_SEL, do_set_date_sel},
  {0, 0, 0, NULL}
};

static const sm_trans_t sm_trans_set_date_date[] = {
  {EV_EC11_C, SM_SET_DATE, SM_SET_DATE_DATE, do_set_date_date},
  {EV_EC11_FAST_C, SM_SET_DATE, SM_SET_DATE_DATE, do_set_date_date},
  {EV_EC11_CC, SM_SET_DATE, SM_SET_DATE_DATE, do_set_date_date},
  {EV_EC11_FAST_CC, SM_SET_DATE, SM_SET_DATE_DATE, do_set_date_date},
  {EV_1S, SM_SET_DATE, SM_SET_DATE_DATE, do_set_date_date},  
  {EV_EC11_PRESS, SM_SET_DATE, SM_SET_DATE_DATE, do_set_date_date},
  {EV_V1, SM_SET_DATE, SM_SET_DATE_SEL, do_set_date_sel},
  {0, 0, 0, NULL}
};

const sm_trans_t * sm_trans_set_date[] = {
  sm_trans_set_date_init,
  sm_trans_set_date_sel,
  sm_trans_set_date_year,
  sm_trans_set_date_mon,
  sm_trans_set_date_date
};
