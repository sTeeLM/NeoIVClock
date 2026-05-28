#include "sm_timer.h"
#include "clock.h"
#include "config.h"
#include "iv18.h"
#include "sm_func_select.h"
#include "task.h"
#include "sm.h"
#include "timer.h"
#include "player.h"
#include "logger.h"
#include "oled.h"
#include "oled_ext.h"

#include "sm_common.h"
#include "sm_func_select.h"

static const char * TAG = "SM_TIMER";

#define SM_TIMER_BLINK_TIMEO 1

const char * sm_states_names_timer[] = {
  "SM_TIMER_INIT",
  "SM_TIMER_SEL",      // 选择timer时间，或者自行设置
  "SM_TIMER_SET_SEL",  // 选择设置哪个字段
  "SM_TIMER_SET_HOUR", // 设置小时
  "SM_TIMER_SET_MIN",  // 设置分钟
  "SM_TIMER_SET_SEC",  // 设置秒
  "SM_TIMER_RUN",      // 倒计时运行
  "SM_TIMER_PAUSE",    // 倒计时暂停
  "SM_TIMER_STOP",     // 倒计时结束
};

typedef enum _sm_timer_type_t {
  SM_TIMER_TYPE_1MIN = 0,
  SM_TIMER_TYPE_3MIN,
  SM_TIMER_TYPE_5MIN,
  SM_TIMER_TYPE_10MIN,      
  SM_TIMER_TYPE_15MIN,
  SM_TIMER_TYPE_30MIN,
  SM_TIMER_TYPE_1HOUR,
  SM_TIMER_TYPE_2HOUR,
  SM_TIMER_TYPE_CUSTOM,
  SM_TIMER_TYPE_QUIT,
  SM_TIMER_TYPE_CNT
} sm_timer_type_t;
static wchar_t * sm_timer_type_names[] = 
{
  L"倒计时1分钟",
  L"倒计时3分钟",
  L"倒计时5分钟",
  L"倒计时10分钟", 
  L"倒计时15分钟",  
  L"倒计时30分钟",  
  L"倒计时1小时",
  L"倒计时2小时",
  L"自定义", 
  L"退出", 
};
static sm_timer_type_t sm_timer_type_index;
static void sm_timer_draw_sel(sm_timer_type_t index)
{
  uint8_t first, i;
  wchar_t buf[16] = {};

  // set timer
  switch(index) {
    case SM_TIMER_TYPE_1MIN:   
      timer_set(0, 1, 0); break;
    case SM_TIMER_TYPE_3MIN:
      timer_set(0, 3, 0); break;
    case SM_TIMER_TYPE_5MIN:  
      timer_set(0, 5, 0); break;    
    case SM_TIMER_TYPE_10MIN:
      timer_set(0, 10, 0); break;      
    case SM_TIMER_TYPE_15MIN:
      timer_set(0, 15, 0); break;  
    case SM_TIMER_TYPE_30MIN:
      timer_set(0, 30, 0); break;      
    case SM_TIMER_TYPE_1HOUR:
      timer_set(1, 0, 0); break;  
    case SM_TIMER_TYPE_2HOUR:
      timer_set(2, 0, 0); break;     
    case SM_TIMER_TYPE_CUSTOM:
    case SM_TIMER_TYPE_QUIT:      
    default:
      timer_set(0, 0, 0); break;     
  }

  // draw iv18
  timer_refresh_display(0);
  
  // 画滚动菜单
  oled_clear();
  first = (index + SM_TIMER_TYPE_CNT - 1) % SM_TIMER_TYPE_CNT;
  oled_fill_rect(0,  16 , 128, 16, true);

  for(i = 0 ; i < 4 ; i ++) {
    swprintf(buf, sizeof(buf)/sizeof(wchar_t), L"%ls", sm_timer_type_names[first]);
    buf[sizeof(buf)/sizeof(wchar_t) - 1] = 0;
    oled_ext_draw_wstring(0, i * 16, buf, MINI_FONT_TYPE_ASCII_8X16, MINI_FONT_TYPE_CHINESE_16X16, OLED_DRAW_XOR);
    first = (first + 1) % SM_TIMER_TYPE_CNT;
  }
  oled_redraw_buffer(); 
}

typedef enum _sm_timer_sel_type_t {
  SM_TIMER_SEL_TYPE_HOUR = 0,
  SM_TIMER_SEL_TYPE_MIN,
  SM_TIMER_SEL_TYPE_SEC,
  SM_TIMER_SEL_TYPE_RUN,
  SM_TIMER_SEL_TYPE_QUIT,
  SM_TIMER_SEL_TYPE_CNT
} sm_timer_sel_type_t;
static wchar_t * sm_timer_sel_type_names[] = 
{
  L"设置小时",
  L"设置分钟",
  L"设置秒",
  L"倒计时开始", 
  L"退出",  
};
static sm_timer_sel_type_t sm_timer_sel_type_index;
static void sm_timer_draw_sel_type(sm_timer_sel_type_t index)
{
 uint8_t first, i;
  wchar_t buf[16] = {};

  iv18_clr_blink(1);
  iv18_clr_blink(2);
  iv18_clr_blink(3);
  iv18_clr_blink(4); 
  iv18_clr_blink(5);
  iv18_clr_blink(6); 

  switch(index) {
    case SM_TIMER_SEL_TYPE_HOUR:
      iv18_set_blink(1);
      iv18_set_blink(2);
      break;
    case SM_TIMER_SEL_TYPE_MIN:
      iv18_set_blink(3);
      iv18_set_blink(4);
      break;
    case SM_TIMER_SEL_TYPE_SEC:
      iv18_set_blink(5);
      iv18_set_blink(6);
      break;
    case SM_TIMER_SEL_TYPE_RUN:
    case SM_TIMER_SEL_TYPE_QUIT:
    default:;
  }

  timer_refresh_display(0);

  // 画滚动菜单
  oled_clear();
  first = (index + SM_TIMER_SEL_TYPE_CNT - 1) % SM_TIMER_SEL_TYPE_CNT;
  oled_fill_rect(0,  16 , 128, 16, true);

  for(i = 0 ; i < 4 ; i ++) {
    swprintf(buf, sizeof(buf)/sizeof(wchar_t), L"%ls", sm_timer_sel_type_names[first]);
    buf[sizeof(buf)/sizeof(wchar_t) - 1] = 0;
    oled_ext_draw_wstring(0, i * 16, buf, MINI_FONT_TYPE_ASCII_8X16, MINI_FONT_TYPE_CHINESE_16X16, OLED_DRAW_XOR);
    first = (first + 1) % SM_TIMER_SEL_TYPE_CNT;
  }
  oled_redraw_buffer(); 
}
static void sm_timer_draw_hour_min_sec(sm_timer_sel_type_t index)
{
  timer_refresh_display(0);

  oled_clear();
  oled_ext_draw_wstring(0, 0, sm_timer_sel_type_names[index], MINI_FONT_TYPE_ASCII_8X16, MINI_FONT_TYPE_CHINESE_16X16, OLED_DRAW_XOR);
  oled_ext_draw_wstring(0, 16, L"旋转调整", MINI_FONT_TYPE_ASCII_8X16, MINI_FONT_TYPE_CHINESE_16X16, OLED_DRAW_XOR);
  oled_ext_draw_wstring(0, 32, L"按下确认", MINI_FONT_TYPE_ASCII_8X16, MINI_FONT_TYPE_CHINESE_16X16, OLED_DRAW_XOR); 
  oled_redraw_buffer(); 
}

typedef enum _sm_timer_status_type_t
{
  SM_TIMER_STATUS_RUNNING = 0,
  SM_TIMER_STATUS_PAUSE,
  SM_TIMER_STATUS_STOPPED
} sm_timer_status_type_t;
uint8_t sm_timer_status_pause_sel_index;
static void sm_timer_draw_status(sm_timer_status_type_t status, uint8_t index)
{
  oled_clear();
  switch(status) {
    case SM_TIMER_STATUS_RUNNING:
      oled_ext_draw_wstring(0, 0, L"倒计时运行中", MINI_FONT_TYPE_ASCII_8X16, MINI_FONT_TYPE_CHINESE_16X16, OLED_DRAW_XOR);
      oled_ext_draw_wstring(0, 16, L"按下暂停", MINI_FONT_TYPE_ASCII_8X16, MINI_FONT_TYPE_CHINESE_16X16, OLED_DRAW_XOR);
    break;
    case SM_TIMER_STATUS_PAUSE:
      oled_fill_rect(0,  (index + 1) * 16 , 128, 16, true);
      oled_ext_draw_wstring(0, 0, L"倒计时暂停", MINI_FONT_TYPE_ASCII_8X16, MINI_FONT_TYPE_CHINESE_16X16, OLED_DRAW_XOR);
      oled_ext_draw_wstring(0, 16, L"继续", MINI_FONT_TYPE_ASCII_8X16, MINI_FONT_TYPE_CHINESE_16X16, OLED_DRAW_XOR);
      oled_ext_draw_wstring(0, 32, L"结束", MINI_FONT_TYPE_ASCII_8X16, MINI_FONT_TYPE_CHINESE_16X16, OLED_DRAW_XOR);
    break;
    case SM_TIMER_STATUS_STOPPED:
      oled_ext_draw_wstring(0, 0, L"倒计时结束", MINI_FONT_TYPE_ASCII_8X16, MINI_FONT_TYPE_CHINESE_16X16, OLED_DRAW_XOR);
      oled_ext_draw_wstring(0, 16, L"按下返回", MINI_FONT_TYPE_ASCII_8X16, MINI_FONT_TYPE_CHINESE_16X16, OLED_DRAW_XOR);
    break;
  }
  oled_redraw_buffer(); 
}

void do_timer_init(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  NEO_LOGD(TAG, "do_timer_init");

  clock_set_display_mode(CLOCK_DISPLAY_MODE_DISABLE);
  timer_clr();
  timer_set_mode(TIMER_MODE_DEC);
  timer_display_enable(true);
  timer_refresh_display(0);
  sm_timer_type_index = SM_TIMER_TYPE_3MIN;
}

static void do_timer_sel(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  if(ev == EV_EC11_UP || ev == EV_V1  || ev == EV_EC11_C || ev == EV_EC11_FAST_C || ev == EV_EC11_CC || ev == EV_EC11_FAST_CC) {
    if(ev == EV_EC11_C || ev == EV_EC11_FAST_C ) {
      sm_timer_type_index = (sm_timer_type_index + 1) % SM_TIMER_TYPE_CNT;
    } else if(ev == EV_EC11_CC || ev == EV_EC11_FAST_CC) {
      sm_timer_type_index = (sm_timer_type_index + SM_TIMER_TYPE_CNT - 1) % SM_TIMER_TYPE_CNT;
    }
    if(ev == EV_EC11_UP || ev == EV_V1) {
      iv18_clr();          
    }
    sm_timer_draw_sel(sm_timer_type_index);
  } else if(ev == EV_EC11_PRESS) {
    switch(sm_timer_type_index) {
      case SM_TIMER_TYPE_1MIN:    
      case SM_TIMER_TYPE_3MIN:
      case SM_TIMER_TYPE_5MIN:      
      case SM_TIMER_TYPE_10MIN:
      case SM_TIMER_TYPE_15MIN:
      case SM_TIMER_TYPE_30MIN:
      case SM_TIMER_TYPE_1HOUR:
      case SM_TIMER_TYPE_2HOUR:
        task_set(EV_V3);
        break;
      case SM_TIMER_TYPE_CUSTOM:
        sm_timer_sel_type_index = SM_TIMER_SEL_TYPE_MIN;
        task_set(EV_V2);
        break;  
      case SM_TIMER_TYPE_QUIT:
        task_set(EV_V1);
        break;        
      default:;
    }
  }
}

static void do_timer_set_sel(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  if(ev == EV_V1 || ev == EV_V2 || ev == EV_EC11_C || ev == EV_EC11_FAST_C || ev == EV_EC11_CC || ev == EV_EC11_FAST_CC) {
    if(ev == EV_EC11_C || ev == EV_EC11_FAST_C ) {
      sm_timer_sel_type_index = (sm_timer_sel_type_index + 1) % SM_TIMER_SEL_TYPE_CNT;
    } else if(ev == EV_EC11_CC || ev == EV_EC11_FAST_CC) {
      sm_timer_sel_type_index = (sm_timer_sel_type_index + SM_TIMER_SEL_TYPE_CNT - 1) % SM_TIMER_SEL_TYPE_CNT;
    }
    sm_timer_draw_sel_type(sm_timer_sel_type_index);
  } else if(ev == EV_EC11_PRESS) {
    switch(sm_timer_sel_type_index) {
      case SM_TIMER_SEL_TYPE_HOUR:
        task_set(EV_V3);
      break;
      case SM_TIMER_SEL_TYPE_MIN: 
        task_set(EV_V4);
      break;      
      case SM_TIMER_SEL_TYPE_SEC: 
        task_set(EV_V5);
      break; 
      case SM_TIMER_SEL_TYPE_RUN:
        task_set(EV_V2);
      break; 
      case SM_TIMER_SEL_TYPE_QUIT:
        task_set(EV_V1);
      break; 
      default:;    
    }
  }
}

static void do_timer_set_hour_min_sec(task_event_t ev)
{
  if(ev == EV_V3 || ev == EV_V4 || ev == EV_V5 || ev == EV_EC11_C || ev == EV_EC11_FAST_C || ev == EV_EC11_CC || ev == EV_EC11_FAST_CC) {
    if(ev == EV_EC11_C || ev == EV_EC11_FAST_C ) {
      if(sm_timer_sel_type_index == SM_TIMER_SEL_TYPE_HOUR) {
        timer_inc_hour(ev == EV_EC11_FAST_C);
      } else if(sm_timer_sel_type_index == SM_TIMER_SEL_TYPE_MIN) {
        timer_inc_min(ev == EV_EC11_FAST_C);
      } else {
        timer_inc_sec(ev == EV_EC11_FAST_C);
      }
    } else if(ev == EV_EC11_CC || ev == EV_EC11_FAST_CC) {
      if(sm_timer_sel_type_index == SM_TIMER_SEL_TYPE_HOUR) {
        timer_dec_hour(ev == EV_EC11_FAST_CC);
      } else if(sm_timer_sel_type_index == SM_TIMER_SEL_TYPE_MIN) {
        timer_dec_min(ev == EV_EC11_FAST_CC);
      } else {
        timer_dec_sec(ev == EV_EC11_FAST_CC);
      }
    }
    if(ev == EV_V3) {
      iv18_set_blink(1);
      iv18_set_blink(2);
    } else if(ev == EV_V4) {
      iv18_set_blink(3);
      iv18_set_blink(4);
    } else if(ev == EV_V5) {
      iv18_set_blink(5);
      iv18_set_blink(6);
    } else {
      sm_common_reset_timeo();
      iv18_clr_blink(1);
      iv18_clr_blink(2);
      iv18_clr_blink(3);
      iv18_clr_blink(4);      
      iv18_clr_blink(5);
      iv18_clr_blink(6);
    }    
    sm_timer_draw_hour_min_sec(sm_timer_sel_type_index);
  } else if(ev == EV_EC11_PRESS) {
    task_set(EV_V1);
  } else if(ev == EV_1S) {
    if(sm_common_test_timeo(SM_TIMER_BLINK_TIMEO)) {
      if(sm_timer_sel_type_index == SM_TIMER_SEL_TYPE_HOUR) {
        iv18_set_blink(1);
        iv18_set_blink(2);
      } else if(sm_timer_sel_type_index == SM_TIMER_SEL_TYPE_MIN) {
        iv18_set_blink(3);
        iv18_set_blink(4);
      } else if(sm_timer_sel_type_index == SM_TIMER_SEL_TYPE_SEC){
        iv18_set_blink(5);
        iv18_set_blink(6);
      }
    }
  }
}

static void do_timer_set_hour(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  do_timer_set_hour_min_sec(ev);
}

static void do_timer_set_min(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  do_timer_set_hour_min_sec(ev);
}

static void do_timer_set_sec(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  do_timer_set_hour_min_sec(ev);
}

static void do_timer_run(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  timer_start();
  sm_timer_draw_status(SM_TIMER_STATUS_RUNNING, 0);
}

static void do_timer_pause(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  if(ev == EV_EC11_C || ev == EV_EC11_FAST_C || ev == EV_EC11_CC || ev == EV_EC11_FAST_CC) {
    sm_timer_status_pause_sel_index = (sm_timer_status_pause_sel_index + 1) % 2;
  } else if(ev == EV_EC11_PRESS && from_state == SM_TIMER_RUN) {
    timer_stop();
    sm_timer_status_pause_sel_index = 0;
  } else if(ev == EV_EC11_PRESS) {
    if(sm_timer_status_pause_sel_index == 0) {
      task_set(EV_V2); // resume
    } else {
      task_set(EV_V1); // clear
    }
  }
  sm_timer_draw_status(SM_TIMER_STATUS_PAUSE, sm_timer_status_pause_sel_index);
}

static void do_timer_stop(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  if(ev == EV_TIMER) {
    timer_clr();
    timer_refresh_display(0);
    iv18_set_blink(1);
    iv18_set_blink(2);
    iv18_set_blink(3);
    iv18_set_blink(4);      
    iv18_set_blink(5);
    iv18_set_blink(6);
    iv18_set_blink(7);
    iv18_set_blink(8);        
    if(ev == EV_TIMER) {
      // play snd
      player_play_snd(PLAYER_SND_DIR_EFFETS, config_read_int("timer_snd"));
    }
  } else if(ev == EV_EC11_PRESS) {
      // stop snd
      player_stop_play();
    task_set(EV_V1);
  }
  sm_timer_draw_status(SM_TIMER_STATUS_STOPPED, 0);
}

static const sm_trans_t sm_trans_timer_init[] = {
  {EV_EC11_UP, SM_TIMER , SM_TIMER_SEL, do_timer_sel},
  {0, 0, 0, NULL}
};

static const sm_trans_t sm_trans_timer_sel[] = {
  {EV_EC11_C, SM_TIMER , SM_TIMER_SEL, do_timer_sel},
  {EV_EC11_FAST_C, SM_TIMER , SM_TIMER_SEL, do_timer_sel},
  {EV_EC11_CC, SM_TIMER , SM_TIMER_SEL, do_timer_sel},
  {EV_EC11_FAST_CC, SM_TIMER , SM_TIMER_SEL, do_timer_sel}, 
  {EV_EC11_PRESS, SM_TIMER , SM_TIMER_SEL, do_timer_sel}, 
  {EV_V1, SM_FUNC_SELECT, SM_FUNC_SELECT_INIT, do_func_select_init},  
  {EV_V2, SM_TIMER, SM_TIMER_SET_SEL, do_timer_set_sel},
  {EV_V3, SM_TIMER, SM_TIMER_RUN, do_timer_run},  
  {0, 0, 0, NULL}
};

static const sm_trans_t sm_trans_timer_set_sel[] = {
  {EV_EC11_C, SM_TIMER , SM_TIMER_SET_SEL, do_timer_set_sel},
  {EV_EC11_FAST_C, SM_TIMER , SM_TIMER_SET_SEL, do_timer_set_sel},
  {EV_EC11_CC, SM_TIMER , SM_TIMER_SET_SEL, do_timer_set_sel},
  {EV_EC11_FAST_CC, SM_TIMER , SM_TIMER_SET_SEL, do_timer_set_sel}, 
  {EV_EC11_PRESS, SM_TIMER , SM_TIMER_SET_SEL, do_timer_set_sel},
  {EV_V1, SM_TIMER , SM_TIMER_SEL, do_timer_sel},
  {EV_V2, SM_TIMER , SM_TIMER_RUN, do_timer_run}, 
  {EV_V3, SM_TIMER , SM_TIMER_SET_HOUR, do_timer_set_hour}, 
  {EV_V4, SM_TIMER , SM_TIMER_SET_MIN, do_timer_set_min}, 
  {EV_V5, SM_TIMER , SM_TIMER_SET_SEC, do_timer_set_sec}, 
  {0, 0, 0, NULL}
};

static const sm_trans_t sm_trans_timer_set_hour[] = {
  {EV_1S, SM_TIMER , SM_TIMER_SET_HOUR, do_timer_set_hour},
  {EV_EC11_C, SM_TIMER , SM_TIMER_SET_HOUR, do_timer_set_hour},
  {EV_EC11_FAST_C, SM_TIMER , SM_TIMER_SET_HOUR, do_timer_set_hour},
  {EV_EC11_CC, SM_TIMER , SM_TIMER_SET_HOUR, do_timer_set_hour},
  {EV_EC11_FAST_CC, SM_TIMER , SM_TIMER_SET_HOUR, do_timer_set_hour}, 
  {EV_EC11_PRESS, SM_TIMER , SM_TIMER_SET_HOUR, do_timer_set_hour},
  {EV_V1, SM_TIMER , SM_TIMER_SET_SEL, do_timer_set_sel},  
  {0, 0, 0, NULL}
};

static const sm_trans_t sm_trans_timer_set_min[] = {
  {EV_1S, SM_TIMER , SM_TIMER_SET_HOUR, do_timer_set_min},
  {EV_EC11_C, SM_TIMER , SM_TIMER_SET_MIN, do_timer_set_min},
  {EV_EC11_FAST_C, SM_TIMER , SM_TIMER_SET_MIN, do_timer_set_min},
  {EV_EC11_CC, SM_TIMER , SM_TIMER_SET_MIN, do_timer_set_min},
  {EV_EC11_FAST_CC, SM_TIMER , SM_TIMER_SET_MIN, do_timer_set_min}, 
  {EV_EC11_PRESS, SM_TIMER , SM_TIMER_SET_MIN, do_timer_set_min},
  {EV_V1, SM_TIMER , SM_TIMER_SET_SEL, do_timer_set_sel},   
  {0, 0, 0, NULL}
};

static const sm_trans_t sm_trans_timer_set_sec[] = {
  {EV_1S, SM_TIMER , SM_TIMER_SET_HOUR, do_timer_set_sec},
  {EV_EC11_C, SM_TIMER , SM_TIMER_SET_SEC, do_timer_set_sec},
  {EV_EC11_FAST_C, SM_TIMER , SM_TIMER_SET_SEC, do_timer_set_sec},
  {EV_EC11_CC, SM_TIMER , SM_TIMER_SET_SEC, do_timer_set_sec},
  {EV_EC11_FAST_CC, SM_TIMER , SM_TIMER_SET_SEC, do_timer_set_sec}, 
  {EV_EC11_PRESS, SM_TIMER , SM_TIMER_SET_SEC, do_timer_set_sec},
  {EV_V1, SM_TIMER , SM_TIMER_SET_SEL, do_timer_set_sel},    
  {0, 0, 0, NULL}
};

static const sm_trans_t sm_trans_timer_run[] = {
  {EV_EC11_PRESS, SM_TIMER , SM_TIMER_PAUSE, do_timer_pause},
  {EV_TIMER, SM_TIMER , SM_TIMER_STOP, do_timer_stop},
  {0, 0, 0, NULL}
};

static const sm_trans_t sm_trans_timer_pause[] = {
  {EV_EC11_C, SM_TIMER , SM_TIMER_PAUSE, do_timer_pause},
  {EV_EC11_FAST_C, SM_TIMER , SM_TIMER_PAUSE, do_timer_pause},
  {EV_EC11_CC, SM_TIMER , SM_TIMER_PAUSE, do_timer_pause},
  {EV_EC11_FAST_CC, SM_TIMER , SM_TIMER_PAUSE, do_timer_pause}, 
  {EV_EC11_PRESS, SM_TIMER , SM_TIMER_PAUSE, do_timer_pause},
  {EV_V1, SM_TIMER , SM_TIMER_SEL, do_timer_sel},
  {EV_V2, SM_TIMER , SM_TIMER_RUN, do_timer_run},  
  {0, 0, 0, NULL}
};

static const sm_trans_t sm_trans_timer_stop[] = {
  {EV_EC11_PRESS, SM_TIMER , SM_TIMER_STOP, do_timer_stop},
  {EV_V1, SM_TIMER , SM_TIMER_SEL, do_timer_sel},  
  {0, 0, 0, NULL}
};

const sm_trans_t * sm_trans_timer[] = {
  sm_trans_timer_init,
  sm_trans_timer_sel,
  sm_trans_timer_set_sel,  
  sm_trans_timer_set_hour,
  sm_trans_timer_set_min,
  sm_trans_timer_set_sec,
  sm_trans_timer_run,
  sm_trans_timer_pause,
  sm_trans_timer_stop
};
