#include "sm_stop_watch.h"
#include "sm_func_select.h"
#include "task.h"
#include "sm.h"
#include "logger.h"
#include "timer.h"
#include "clock.h"
#include "oled.h"
#include "oled_ext.h"
#include "iv18.h"

#include "sm_func_select.h"

static const char * TAG = "SM_STOP_WATCH";

const char * sm_states_names_stop_watch[] = {
  "SM_STOP_WATCH_INIT",
  "SM_STOP_WATCH_STOP",  // 停止状态（清零）
  "SM_STOP_WATCH_RUN",   // 运行状态
  "SM_STOP_WATCH_PAUSE"  // 暂停状态  
};

static uint8_t sm_stop_watch_stop_sel_index;
// 0: start
// 1: quit
static void sm_stop_watch_draw_stop(uint8_t index)
{
  oled_clear();
  oled_fill_rect(0,  index * 16 , 128, 16, true);
  oled_ext_draw_wstring(0, 0, L"启动", MINI_FONT_TYPE_ASCII_8X16, MINI_FONT_TYPE_CHINESE_16X16, OLED_DRAW_XOR);
  oled_ext_draw_wstring(0, 16, L"退出", MINI_FONT_TYPE_ASCII_8X16, MINI_FONT_TYPE_CHINESE_16X16, OLED_DRAW_XOR);
  oled_redraw_buffer();
}

static void sm_stop_watch_draw_run(void)
{
  wchar_t buf[16] = {};
  
  oled_clear();
  oled_ext_draw_wstring(0, 0, L"按下暂停", MINI_FONT_TYPE_ASCII_8X16, MINI_FONT_TYPE_CHINESE_16X16, OLED_DRAW_XOR);
  oled_ext_draw_wstring(0, 16, L"旋转记录", MINI_FONT_TYPE_ASCII_8X16, MINI_FONT_TYPE_CHINESE_16X16, OLED_DRAW_XOR);
  if(timer_get_current_slot()) {
    swprintf(buf, sizeof(buf)/sizeof(wchar_t), L"最新记录%02d", timer_get_current_slot());
    buf[sizeof(buf)/sizeof(wchar_t) - 1] = 0;
    oled_ext_draw_wstring(0, 32, buf, MINI_FONT_TYPE_ASCII_8X16, MINI_FONT_TYPE_CHINESE_16X16, OLED_DRAW_XOR);
    swprintf(buf, sizeof(buf)/sizeof(wchar_t), L"%02d:%02d:%02d:%02d", 
      timer_get_hour(timer_get_current_slot()),
      timer_get_min(timer_get_current_slot()), 
      timer_get_sec(timer_get_current_slot()), 
      timer_get_10ms(timer_get_current_slot()));
    buf[sizeof(buf)/sizeof(wchar_t) - 1] = 0;
    oled_ext_draw_wstring(0, 48, buf, MINI_FONT_TYPE_ASCII_8X16, MINI_FONT_TYPE_CHINESE_16X16, OLED_DRAW_XOR);
  } else {
    oled_ext_draw_wstring(0, 32, L"无记录", MINI_FONT_TYPE_ASCII_8X16, MINI_FONT_TYPE_CHINESE_16X16, OLED_DRAW_XOR);
  }
  oled_redraw_buffer();
}

static uint8_t sm_stop_watch_timer_pause_index;
// 0 -> clear
// 1 -> resume
// 2 ~ timer_get_slot_cnt(): saved history slot 1 ~ timer_get_slot_cnt() - 1
static void sm_stop_watch_draw_pause(uint8_t index)
{
  uint8_t first, i;
  uint8_t total_cnt = timer_get_slot_cnt() + 1;
  wchar_t buf[16] = {};

  // 绘制滚动菜单
  oled_clear();

  first = (index + total_cnt - 1) % total_cnt;
  oled_fill_rect(0,  16 , 128, 16, true);

  for(i = 0 ; i < 4 ; i ++) {
    switch(first) {
      case 0:
        oled_ext_draw_wstring(0, i * 16, L"清零", MINI_FONT_TYPE_ASCII_8X16, MINI_FONT_TYPE_CHINESE_16X16, OLED_DRAW_XOR);
      break;
      case 1:
        oled_ext_draw_wstring(0, i * 16, L"恢复运行", MINI_FONT_TYPE_ASCII_8X16, MINI_FONT_TYPE_CHINESE_16X16, OLED_DRAW_XOR);
      break;
      default:
        swprintf(buf, sizeof(buf)/sizeof(wchar_t), L"%02d: %02d:%02d:%02d:%02d", 
          first - 1, 
          timer_get_hour(first - 1),
          timer_get_min(first - 1), 
          timer_get_sec(first - 1), 
          timer_get_10ms(first - 1));
        buf[sizeof(buf)/sizeof(wchar_t) - 1] = 0;
        oled_ext_draw_wstring(0, i * 16, buf, MINI_FONT_TYPE_ASCII_8X16, MINI_FONT_TYPE_CHINESE_16X16, OLED_DRAW_XOR);
    }
    
    first = (first + 1) % total_cnt;
  } 
  oled_redraw_buffer(); 
}

void do_stop_watch_init(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  NEO_LOGD(TAG, "do_stop_watch_init");

  clock_set_display_mode(CLOCK_DISPLAY_MODE_DISABLE);
  timer_clr();
  timer_set_mode(TIMER_MODE_INC);
  timer_display_enable(true);
  timer_refresh_display(0);
  sm_stop_watch_stop_sel_index = 0;
  sm_stop_watch_timer_pause_index = 0;
}


static void do_stop_watch_stop(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  if(ev == EV_EC11_UP || ev == EV_V2 || ev == EV_EC11_C || ev == EV_EC11_FAST_C || ev == EV_EC11_CC || ev == EV_EC11_FAST_CC) {
    if(ev == EV_EC11_C || ev == EV_EC11_FAST_C || ev == EV_EC11_CC || ev == EV_EC11_FAST_CC) {
      sm_stop_watch_stop_sel_index = (sm_stop_watch_stop_sel_index + 1) % 2;
    } else {
      timer_clr();
      timer_refresh_display(0);
    }
    sm_stop_watch_draw_stop(sm_stop_watch_stop_sel_index);
  } else if(ev == EV_EC11_PRESS) {
    if(sm_stop_watch_stop_sel_index == 0) {
      task_set(EV_V2);
    } else {
      task_set(EV_V1);
    }
  }
}

static void do_stop_watch_run(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  if(ev == EV_V1 || ev == EV_V2) {
    timer_start();
  } else if(ev == EV_EC11_C || ev == EV_EC11_FAST_C || ev == EV_EC11_CC || ev == EV_EC11_FAST_CC) {
    timer_save();
    // 冻结一下显示，表示已经保存了数据
    timer_display_enable(false);
  } else if(ev == EV_1S) {
    // 解冻显示，继续运行
    timer_display_enable(true);
  } else if(ev == EV_EC11_PRESS) {
    timer_stop();
    task_set(EV_V1);
    sm_stop_watch_timer_pause_index = 1; // make "resume" default choice
  }
  if(ev != EV_250MS) {
    sm_stop_watch_draw_run();
  }
}

static void do_stop_watch_pause(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  if(ev == EV_V1 || ev == EV_EC11_C || ev == EV_EC11_FAST_C || ev == EV_EC11_CC || ev == EV_EC11_FAST_CC) {
    if(ev == EV_EC11_C || ev == EV_EC11_FAST_C) {
      sm_stop_watch_timer_pause_index = (sm_stop_watch_timer_pause_index + 1) % (timer_get_slot_cnt() + 1);
    } else if(ev == EV_EC11_CC || ev == EV_EC11_FAST_CC) {
      sm_stop_watch_timer_pause_index = (sm_stop_watch_timer_pause_index + timer_get_slot_cnt()) % (timer_get_slot_cnt() + 1);
    }
    sm_stop_watch_draw_pause(sm_stop_watch_timer_pause_index);
  } else if(ev == EV_EC11_PRESS) {
    if(sm_stop_watch_timer_pause_index == 0) {
      timer_stop();
      task_set(EV_V2);
    } else {
      task_set(EV_V1);
    }
  }
  
}

static const sm_trans_t sm_trans_stop_watch_init[] = {
  {EV_EC11_UP, SM_STOP_WATCH, SM_STOP_WATCH_STOP, do_stop_watch_stop},
  {0, 0, 0, NULL}
};

static const sm_trans_t sm_trans_stop_watch_stop[] = {
  {EV_EC11_C, SM_STOP_WATCH, SM_STOP_WATCH_STOP, do_stop_watch_stop},
  {EV_EC11_FAST_C, SM_STOP_WATCH, SM_STOP_WATCH_STOP, do_stop_watch_stop},
  {EV_EC11_CC, SM_STOP_WATCH, SM_STOP_WATCH_STOP, do_stop_watch_stop},
  {EV_EC11_FAST_CC, SM_STOP_WATCH, SM_STOP_WATCH_STOP, do_stop_watch_stop},
  {EV_EC11_PRESS, SM_STOP_WATCH, SM_STOP_WATCH_STOP, do_stop_watch_stop},
  {EV_V1, SM_FUNC_SELECT, SM_FUNC_SELECT_INIT, do_func_select_init},  
  {EV_V2, SM_STOP_WATCH, SM_STOP_WATCH_RUN, do_stop_watch_run},  
  {0, 0, 0, NULL}
};

static const sm_trans_t sm_trans_stop_watch_run[] = {
  {EV_1S, SM_STOP_WATCH, SM_STOP_WATCH_RUN, do_stop_watch_run},
  {EV_EC11_C, SM_STOP_WATCH, SM_STOP_WATCH_RUN, do_stop_watch_run},
  {EV_EC11_FAST_C, SM_STOP_WATCH, SM_STOP_WATCH_RUN, do_stop_watch_run},
  {EV_EC11_CC, SM_STOP_WATCH, SM_STOP_WATCH_RUN, do_stop_watch_run},
  {EV_EC11_FAST_CC, SM_STOP_WATCH, SM_STOP_WATCH_RUN, do_stop_watch_run},  
  {EV_EC11_PRESS, SM_STOP_WATCH, SM_STOP_WATCH_RUN, do_stop_watch_run},
  {EV_V1, SM_STOP_WATCH, SM_STOP_WATCH_PAUSE, do_stop_watch_pause},  
  {0, 0, 0, NULL}
};

static const sm_trans_t sm_trans_stop_watch_pause[] = {
  {EV_EC11_C, SM_STOP_WATCH, SM_STOP_WATCH_PAUSE, do_stop_watch_pause},
  {EV_EC11_FAST_C, SM_STOP_WATCH, SM_STOP_WATCH_PAUSE, do_stop_watch_pause},
  {EV_EC11_CC, SM_STOP_WATCH, SM_STOP_WATCH_PAUSE, do_stop_watch_pause},
  {EV_EC11_FAST_CC, SM_STOP_WATCH, SM_STOP_WATCH_PAUSE, do_stop_watch_pause},
  {EV_EC11_PRESS, SM_STOP_WATCH, SM_STOP_WATCH_PAUSE, do_stop_watch_pause},
  {EV_V1, SM_STOP_WATCH, SM_STOP_WATCH_RUN, do_stop_watch_run},  
  {EV_V2, SM_STOP_WATCH, SM_STOP_WATCH_STOP, do_stop_watch_stop},   
  {0, 0, 0, NULL}
};

const sm_trans_t * sm_trans_stop_watch[] = {
  sm_trans_stop_watch_init,
  sm_trans_stop_watch_stop,
  sm_trans_stop_watch_run,
  sm_trans_stop_watch_pause,
};
