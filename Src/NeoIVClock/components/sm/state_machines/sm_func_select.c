#include "sm_func_select.h"
#include "oled.h"
#include "task.h"
#include "sm.h"
#include "logger.h"
#include "clock.h"
#include "iv18.h"
#include "oled.h"
#include "oled_ext.h"
#include "oled_ext_icon.h"

#include "sm_clock.h"
#include "sm_set_alarm.h"
#include "sm_set_time.h"
#include "sm_set_date.h"
#include "sm_set_param.h"
#include "sm_set_net.h"
#include "sm_timer.h"
#include "sm_stop_watch.h"
#include <stdbool.h>

/*
  SM_FUNC_SELECT_CLOCK,
  SM_FUNC_SELECT_ALARM,
  SM_FUNC_SELECT_SET_TIME,
  SM_FUNC_SELECT_SET_DATE,
  SM_FUNC_SELECT_SET_PARAM,

  SM_FUNC_SELECT_TIMER,
  SM_FUNC_SELECT_STOP_WATCH,
*/
const char * sm_states_names_func_select[] = {
  "SM_FUNC_SELECT_INIT",
  "SM_FUNC_SELECT_CLOCK",
  "SM_FUNC_SELECT_ALARM",
  "SM_FUNC_SELECT_SET_TIME",
  "SM_FUNC_SELECT_SET_DATE",
  "SM_FUNC_SELECT_SET_PARAM",
  "SM_FUNC_SELECT_SET_NET",  
  "SM_FUNC_SELECT_TIMER",
  "SM_FUNC_SELECT_STOP_WATCH"
};

static const char * TAG = "SM_FUNC_SELECT";

static const uint8_t * sm_func_icon_array[] = 
{
  oled_ext_icon_CLOCK,
  oled_ext_icon_ALARM,
  oled_ext_icon_SET_TIME,
  oled_ext_icon_SET_DATE,
  oled_ext_icon_SET_PARAM,
  oled_ext_icon_WIFI,
  oled_ext_icon_TIMER,
  oled_ext_icon_STOP_WATCH,
};

static uint8_t sm_func_select_last_index;

static void sm_func_select_show_icon(uint8_t icon_index)
{
  NEO_LOGD(TAG, "sm_func_select_show_icon %d", icon_index);

  sm_func_select_last_index = icon_index;

  oled_clear();

  // 绘制边框
  oled_fill_rect(0, 0, 128, 48, true);
  oled_fill_rect(0, 1, 128, 46, false);
  
  // 绘制icon
  icon_index %= 8;

  // 绘制中心白色块
  oled_fill_rect(40, 0, 48, 48, true);
  oled_draw_bitmap(-8, 0, 48, 48, sm_func_icon_array[(icon_index + 7) % 8], OLED_DRAW_OR);
  oled_draw_bitmap(40, 0, 48, 48, sm_func_icon_array[icon_index], OLED_DRAW_XOR);
  oled_draw_bitmap(88, 0, 48, 48, sm_func_icon_array[(icon_index + 1) % 8], OLED_DRAW_OR);  

  switch(icon_index) {
    case 0:
      // 时钟
      oled_ext_draw_wstring(48, 48, L"时钟", MINI_FONT_TYPE_ASCII_8X16, MINI_FONT_TYPE_CHINESE_16X16, OLED_DRAW_OR);
      break;
    case 1:
      // 闹钟
      oled_ext_draw_wstring(48, 48, L"闹钟", MINI_FONT_TYPE_ASCII_8X16, MINI_FONT_TYPE_CHINESE_16X16, OLED_DRAW_OR);
      break;    
    case 2:
      // 设置时间
      oled_ext_draw_wstring(32, 48, L"设置时间", MINI_FONT_TYPE_ASCII_8X16, MINI_FONT_TYPE_CHINESE_16X16, OLED_DRAW_OR);    
      break;    
    case 3:
      // 设置日期
      oled_ext_draw_wstring(32, 48, L"设置日期", MINI_FONT_TYPE_ASCII_8X16, MINI_FONT_TYPE_CHINESE_16X16, OLED_DRAW_OR);       
      break;
    case 4:
      // 设置参数
      oled_ext_draw_wstring(32, 48, L"设置参数", MINI_FONT_TYPE_ASCII_8X16, MINI_FONT_TYPE_CHINESE_16X16, OLED_DRAW_OR);       
      break;    
    case 5:
      // 设置网络
      oled_ext_draw_wstring(32, 48, L"设置网络", MINI_FONT_TYPE_ASCII_8X16, MINI_FONT_TYPE_CHINESE_16X16, OLED_DRAW_OR);      
      break;    
    case 6:
      // 计时器
      oled_ext_draw_wstring(40, 48, L"计时器", MINI_FONT_TYPE_ASCII_8X16, MINI_FONT_TYPE_CHINESE_16X16, OLED_DRAW_OR);
      break;     
    case 7:
      // 秒表
      oled_ext_draw_wstring(48, 48, L"秒表", MINI_FONT_TYPE_ASCII_8X16, MINI_FONT_TYPE_CHINESE_16X16, OLED_DRAW_OR);
      break;
    default:
      NEO_LOGW(TAG, "invalid index %d", icon_index);
      break;
  }
  
  oled_redraw_buffer();
}

void do_func_select_init(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  NEO_LOGD(TAG, "do_func_select_init");

  if( (from_func != SM_FUNC_SELECT || from_state != SM_FUNC_SELECT_INIT) && 
    (ev == EV_V1 || ev == EV_V2 || ev == EV_V3 || ev == EV_V4 
      || ev == EV_V5 ||  ev == EV_V6 || ev == EV_V7 || ev == EV_V8 || ev == EV_V9)) {
    // 从别的功能通过虚拟按键转入，我们生成一个EV_EC11_UP来统一入口
    task_set(EV_EC11_UP);
    return;
  }

  if(ev == EV_EC11_UP) {
    // 将IV18设置为时钟状态
    iv18_reset_ps_timeo();
    clock_set_display_mode(CLOCK_DISPLAY_MODE_TIME);
  }

  // 通过给自己发送不同的虚拟事件，让状态机实现在do_函数里根据不同逻辑跳转不同状态的能力
  switch(sm_func_select_last_index) {
    case 0 : task_set(EV_V1); break;
    case 1 : task_set(EV_V2); break;
    case 2 : task_set(EV_V3); break;
    case 3 : task_set(EV_V4); break;
    case 4 : task_set(EV_V5); break;
    case 5 : task_set(EV_V6); break;   
    case 6 : task_set(EV_V7); break;   
    case 7 : task_set(EV_V8); break; 
    default:
      NEO_LOGW(TAG, "invalid sm_func_select_last_index %d", sm_func_select_last_index);
      break;
  }
}

static void do_func_select_clock(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  sm_func_select_show_icon(0);
}

static void do_func_select_alarm(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  sm_func_select_show_icon(1);
}

static void do_func_select_set_time(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  sm_func_select_show_icon(2);
}

static void do_func_select_set_date(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  sm_func_select_show_icon(3);
}

static void do_func_select_set_param(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  sm_func_select_show_icon(4);
}

static void do_func_select_set_net(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  sm_func_select_show_icon(5);
}

static void do_func_select_timer(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  sm_func_select_show_icon(6);
}

static void do_func_select_stop_watch(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  sm_func_select_show_icon(7);
}

static const sm_trans_t sm_trans_func_select_init[] = {
  {EV_EC11_UP, SM_FUNC_SELECT, SM_FUNC_SELECT_INIT, do_func_select_init},
  {EV_V1, SM_FUNC_SELECT, SM_FUNC_SELECT_CLOCK, do_func_select_clock},
  {EV_V2, SM_FUNC_SELECT, SM_FUNC_SELECT_ALARM, do_func_select_alarm},
  {EV_V3, SM_FUNC_SELECT, SM_FUNC_SELECT_SET_TIME, do_func_select_set_time}, 
  {EV_V4, SM_FUNC_SELECT, SM_FUNC_SELECT_SET_DATE, do_func_select_set_date},
  {EV_V5, SM_FUNC_SELECT, SM_FUNC_SELECT_SET_PARAM, do_func_select_set_param},
  {EV_V6, SM_FUNC_SELECT, SM_FUNC_SELECT_SET_NET, do_func_select_set_net},
  {EV_V7, SM_FUNC_SELECT, SM_FUNC_SELECT_TIMER, do_func_select_timer},
  {EV_V8, SM_FUNC_SELECT, SM_FUNC_SELECT_STOP_WATCH, do_func_select_stop_watch},      
  {0, 0, 0, NULL}
};

static const sm_trans_t sm_trans_func_select_clock[] = {
  {EV_EC11_CC, SM_FUNC_SELECT, SM_FUNC_SELECT_ALARM, do_func_select_alarm},
  {EV_EC11_FAST_CC, SM_FUNC_SELECT, SM_FUNC_SELECT_ALARM, do_func_select_alarm}, 
  {EV_EC11_C, SM_FUNC_SELECT, SM_FUNC_SELECT_STOP_WATCH, do_func_select_stop_watch},
  {EV_EC11_FAST_C, SM_FUNC_SELECT, SM_FUNC_SELECT_STOP_WATCH, do_func_select_stop_watch}, 
  {EV_EC11_PRESS, SM_CLOCK, SM_CLOCK_INIT, do_clock_init},
  {0, 0, 0, NULL}
};

static const sm_trans_t sm_trans_func_select_alarm[] = {
  {EV_EC11_CC, SM_FUNC_SELECT, SM_FUNC_SELECT_SET_TIME, do_func_select_set_time},
  {EV_EC11_FAST_CC, SM_FUNC_SELECT, SM_FUNC_SELECT_SET_TIME, do_func_select_set_time}, 
  {EV_EC11_C, SM_FUNC_SELECT, SM_FUNC_SELECT_CLOCK, do_func_select_clock},
  {EV_EC11_FAST_C, SM_FUNC_SELECT, SM_FUNC_SELECT_CLOCK, do_func_select_clock},  
  {EV_EC11_PRESS, SM_SET_ALARM, SM_SET_ALARM_INIT, do_set_alarm_init},  
  {0, 0, 0, NULL}
};

static const sm_trans_t sm_trans_func_select_set_time[] = {
  {EV_EC11_CC, SM_FUNC_SELECT, SM_FUNC_SELECT_SET_DATE, do_func_select_set_date},
  {EV_EC11_FAST_CC, SM_FUNC_SELECT, SM_FUNC_SELECT_SET_DATE, do_func_select_set_date}, 
  {EV_EC11_C, SM_FUNC_SELECT, SM_FUNC_SELECT_ALARM, do_func_select_alarm},
  {EV_EC11_FAST_C, SM_FUNC_SELECT, SM_FUNC_SELECT_ALARM, do_func_select_alarm}, 
  {EV_EC11_PRESS, SM_SET_TIME, SM_SET_TIME_INIT, do_set_time_init}, 
  {0, 0, 0, NULL}
};

static const sm_trans_t sm_trans_func_select_set_date[] = {
  {EV_EC11_CC, SM_FUNC_SELECT, SM_FUNC_SELECT_SET_PARAM, do_func_select_set_param},
  {EV_EC11_FAST_CC, SM_FUNC_SELECT, SM_FUNC_SELECT_SET_PARAM, do_func_select_set_param}, 
  {EV_EC11_C, SM_FUNC_SELECT, SM_FUNC_SELECT_SET_TIME, do_func_select_set_time},
  {EV_EC11_FAST_C, SM_FUNC_SELECT, SM_FUNC_SELECT_SET_TIME, do_func_select_set_time}, 
  {EV_EC11_PRESS, SM_SET_DATE, SM_SET_DATE_INIT, do_set_date_init}, 
  {0, 0, 0, NULL}
};

static const sm_trans_t sm_trans_func_select_set_param[] = {
  {EV_EC11_CC, SM_FUNC_SELECT, SM_FUNC_SELECT_SET_NET, do_func_select_set_net},
  {EV_EC11_FAST_CC, SM_FUNC_SELECT, SM_FUNC_SELECT_SET_NET, do_func_select_set_net}, 
  {EV_EC11_C, SM_FUNC_SELECT, SM_FUNC_SELECT_SET_DATE, do_func_select_set_date},
  {EV_EC11_FAST_C, SM_FUNC_SELECT, SM_FUNC_SELECT_SET_DATE, do_func_select_set_date}, 
  {EV_EC11_PRESS, SM_SET_PARAM, SM_SET_PARAM_INIT, do_set_param_init}, 
  {0, 0, 0, NULL}
};

static const sm_trans_t sm_trans_func_select_set_net[] = {
  {EV_EC11_CC, SM_FUNC_SELECT, SM_FUNC_SELECT_TIMER, do_func_select_timer},
  {EV_EC11_FAST_CC, SM_FUNC_SELECT, SM_FUNC_SELECT_TIMER, do_func_select_timer}, 
  {EV_EC11_C, SM_FUNC_SELECT, SM_FUNC_SELECT_SET_PARAM, do_func_select_set_param},
  {EV_EC11_FAST_C, SM_FUNC_SELECT, SM_FUNC_SELECT_SET_PARAM, do_func_select_set_param},  
  {EV_EC11_PRESS, SM_SET_NET, SM_SET_NET_INIT, do_set_net_init}, 
  {0, 0, 0, NULL}
};

static const sm_trans_t sm_trans_func_select_timer[] = {
  {EV_EC11_CC, SM_FUNC_SELECT, SM_FUNC_SELECT_STOP_WATCH, do_func_select_stop_watch},
  {EV_EC11_FAST_CC, SM_FUNC_SELECT, SM_FUNC_SELECT_STOP_WATCH, do_func_select_stop_watch}, 
  {EV_EC11_C, SM_FUNC_SELECT, SM_FUNC_SELECT_SET_NET, do_func_select_set_net},
  {EV_EC11_FAST_C, SM_FUNC_SELECT, SM_FUNC_SELECT_SET_NET, do_func_select_set_net}, 
  {EV_EC11_PRESS, SM_TIMER, SM_TIMER_INIT, do_timer_init}, 
  {0, 0, 0, NULL}
};

static const sm_trans_t sm_trans_func_select_stop_watch[] = {
  {EV_EC11_CC, SM_FUNC_SELECT, SM_FUNC_SELECT_CLOCK, do_func_select_clock},
  {EV_EC11_FAST_CC, SM_FUNC_SELECT, SM_FUNC_SELECT_CLOCK, do_func_select_clock}, 
  {EV_EC11_C, SM_FUNC_SELECT, SM_FUNC_SELECT_TIMER, do_func_select_timer},
  {EV_EC11_FAST_C, SM_FUNC_SELECT, SM_FUNC_SELECT_TIMER, do_func_select_timer}, 
  {EV_EC11_PRESS, SM_STOP_WATCH, SM_STOP_WATCH_INIT, do_stop_watch_init}, 
  {0, 0, 0, NULL}
};

const sm_trans_t * sm_trans_func_select[] = {
  sm_trans_func_select_init,
  sm_trans_func_select_clock,
  sm_trans_func_select_alarm,
  sm_trans_func_select_set_time,
  sm_trans_func_select_set_date,
  sm_trans_func_select_set_param,
  sm_trans_func_select_set_net,
  sm_trans_func_select_timer,
  sm_trans_func_select_stop_watch,
};
