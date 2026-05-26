#include "sm_set_param.h"
#include "task.h"
#include "sm.h"
#include "logger.h"
#include "oled.h"
#include "oled_ext.h"
#include <stdint.h>

#include "sm_func_select.h"

#include "iv18.h"
#include "clock.h"
#include "beeper.h"
#include "motion_sensor.h"
#include "player.h"

#include "config.h"


const char * sm_states_names_set_param[] = {
  "SM_SET_PRARM_INIT",
  "SM_SET_PARAM_SET_SWITCH",  // 所有简单离散值的设置
  "SM_SET_PARAM_IV18_BRIGHT", // IV18亮度
  "SM_SET_PARAM_PLY_VOL",     // 播放器音量
  "SM_SET_PARAM_TIMER_SND"    // timer音效  
};

static const char * TAG = "SM_SET_PARAM";

/*
  // 是否以12小时制显示时间，0表示24小时制，1表示12小时制
  {"time_12", CONFIG_TYPE_UINT8,   {.val8 = 0}},
  // 世纪数，例如19表示19xx年
  {"century", CONFIG_TYPE_UINT8,   {.val8 = 20}},
  // 是否打开运动检测
  {"motion_en",CONFIG_TYPE_UINT8,  {.val8 = 1}},
  // 是否打开按键声音和timer声音
  {"bp_en",CONFIG_TYPE_UINT8,      {.val8 = 1}},
  // 温度显示为摄氏度，还是华氏度？0表示华氏度，1表示摄氏度
  {"temp_unit", CONFIG_TYPE_UINT8,  {.val8 = 1}}, 
  // 气压显示单位，0表示hpa，1表示Hgmm，2表示Atm
  {"press_unit", CONFIG_TYPE_UINT8, {.val8 = 0}},
  // 定时关闭IV18? 0:关闭 1:10s，2:30s，3:1分钟
  {"iv18_ps_sec", CONFIG_TYPE_UINT8,  {.val8 = 10}},
  // IV18亮度，0为自动根据环境光线调整，1～100为对应亮度数值
  {"iv18_brightness", CONFIG_TYPE_UINT8,  {.val8 = 0}},
  // 播放器音量，0~10, 0表示静音，10表示最大声
  {"ply_vol", CONFIG_TYPE_UINT8, {.val8 = 10}},
  // 传感器数据上报间隔，0:10s，1:30s，2:1分钟，3:10分钟
  {"reporter_sec", CONFIG_TYPE_UINT8, {.val8 = 0}},
  // timer音效
  {"timer_snd", CONFIG_TYPE_UINT8, {.val8 = 0}},
 
*/
typedef enum _sm_set_param_type_t
{
  SM_SET_PARAM_TYPE_TIME_12 = 0,
  SM_SET_PARAM_TYPE_MOTION_EN,
  SM_SET_PARAM_TYPE_BP_EN,
  SM_SET_PARAM_TYPE_TEMP_UNIT,
  SM_SET_PARAM_TYPE_PRESS_UNIT,
  SM_SET_PARAM_TYPE_IV18_PS_SEC,
  SM_SET_PARAM_TYPE_IV18_BRIGHT,
  SM_SET_PARAM_TYPE_PLY_VOL,
  SM_SET_PARAM_TYPE_REPORTER_SEC,
  SM_SET_PARAM_TYPE_TIMER_SND,
  SM_SET_PARAM_TYPE_QUIT,
  SM_SET_PARAM_TYPE_CNT
} sm_set_param_type_t;

static uint8_t sm_set_param_type_index;

static wchar_t * sm_set_param_switch_name[] = 
{
  L"时钟格式:%ls",
  L"震动唤醒:%ls",
  L"蜂鸣器:%ls",
  L"温度单位:%ls",
  L"气压单位:%ls",
  L"自动休眠:%ls",
  L"IV18亮度:%ls",
  L"声效音量:%ls",
  L"上报间隔:%ls",
  L"定时器音效:%02d", 
  L"退出"
};

static void sm_set_param_draw_switch(uint8_t index)
{
  uint8_t first, i;
  wchar_t buf[16] = {0};
  /* 滚动菜单 */
  oled_clear();
  
  first = (index + SM_SET_PARAM_TYPE_CNT - 1) % SM_SET_PARAM_TYPE_CNT;

  oled_fill_rect(0,  16 , 128, 16, true);

  for(i = 0 ; i < 4 ; i ++) {
    switch(first) {
      case SM_SET_PARAM_TYPE_TIME_12:
        swprintf(buf, sizeof(buf)/sizeof(wchar_t), sm_set_param_switch_name[first], clock_test_hour12() ? L"12小时":L"24小时");
      break;
      case SM_SET_PARAM_TYPE_MOTION_EN:
        swprintf(buf, sizeof(buf)/sizeof(wchar_t), sm_set_param_switch_name[first], motion_sensor_test_enable() ? L"开启":L"关闭");
      break;
      case SM_SET_PARAM_TYPE_BP_EN:
        swprintf(buf, sizeof(buf)/sizeof(wchar_t), sm_set_param_switch_name[first], beep_test_enable() ? L"开启":L"关闭");
      break;
      case SM_SET_PARAM_TYPE_TEMP_UNIT: {
        uint8_t temp_unit = config_read_int("temp_unit");
        swprintf(buf, sizeof(buf)/sizeof(wchar_t), sm_set_param_switch_name[first],  temp_unit == 0 ? L"华氏" : L"摄氏");
      } break;
      case SM_SET_PARAM_TYPE_PRESS_UNIT: {
        uint8_t press_unit = config_read_int("press_unit");
        swprintf(buf, sizeof(buf)/sizeof(wchar_t), sm_set_param_switch_name[first],  press_unit == 0 ? L"hpa" : (press_unit == 1 ? L"Hgmm" : L"Atm") );
      } break;
      case SM_SET_PARAM_TYPE_IV18_PS_SEC: {
        /*0:关闭 1:10s，2:30s，3:1分钟*/
        uint8_t iv18_ps_sec = config_read_int("iv18_ps_sec");
        swprintf(buf, sizeof(buf)/sizeof(wchar_t), sm_set_param_switch_name[first], 
         iv18_ps_sec == 0 ? L"关闭" : 
         (iv18_ps_sec == 1 ? L"10秒" : 
         (iv18_ps_sec == 2 ? L"30秒" : L"1分钟")) );
      } break;
      case SM_SET_PARAM_TYPE_IV18_BRIGHT: {
        uint8_t iv18_brightness = iv18_get_brightness();
        wchar_t buf1[16] = {0};
        if(iv18_brightness == 0) {
          swprintf(buf1, sizeof(buf1)/sizeof(wchar_t),L"自动");
        } else if(iv18_brightness == 100) {
          swprintf(buf1, sizeof(buf1)/sizeof(wchar_t),L"常亮");
        } else {
          swprintf(buf1, sizeof(buf1)/sizeof(wchar_t),L"%d%%", iv18_brightness);
        }
        buf1[sizeof(buf1)/sizeof(wchar_t) - 1] = 0;
        swprintf(buf, sizeof(buf)/sizeof(wchar_t), sm_set_param_switch_name[first], buf1);
      } break;
      case SM_SET_PARAM_TYPE_PLY_VOL: {
        uint8_t ply_vol = player_get_vol();
        wchar_t buf1[16] = {0};
        if(ply_vol == 0) {
          swprintf(buf1, sizeof(buf1)/sizeof(wchar_t),L"关闭");
        } else {
          swprintf(buf1, sizeof(buf1)/sizeof(wchar_t),L"%d%%", ply_vol);
        }
        buf1[sizeof(buf1)/sizeof(wchar_t) - 1] = 0;
        swprintf(buf, sizeof(buf)/sizeof(wchar_t), sm_set_param_switch_name[first], buf1);
      }break;
      case SM_SET_PARAM_TYPE_REPORTER_SEC: {
        /* 0:10s，1:30s，2:1分钟，3:10分钟 */
        uint8_t reporter_sec = config_read_int("reporter_sec");
        swprintf(buf, sizeof(buf)/sizeof(wchar_t), sm_set_param_switch_name[first], 
         reporter_sec == 0 ? L"关闭" : 
         (reporter_sec == 1 ? L"10秒" : 
         (reporter_sec == 2 ? L"30秒" : (reporter_sec == 3 ? L"1分钟" : L"10分钟"))) );
      } break; 
      case SM_SET_PARAM_TYPE_TIMER_SND: {
        swprintf(buf, sizeof(buf)/sizeof(wchar_t), sm_set_param_switch_name[first], config_read_int("timer_snd"));
      } break;
      case SM_SET_PARAM_TYPE_QUIT: {
        swprintf(buf, sizeof(buf)/sizeof(wchar_t), sm_set_param_switch_name[first]);
      } break;
      default:;
    }
    buf[sizeof(buf)/sizeof(wchar_t) - 1] = 0;
    oled_ext_draw_wstring(0, i * 16, buf, MINI_FONT_TYPE_ASCII_8X16, MINI_FONT_TYPE_CHINESE_16X16, OLED_DRAW_XOR);
    first = (first + 1) % SM_SET_PARAM_TYPE_CNT;
  }
  oled_redraw_buffer();
}

static void sm_set_param_tiggle_switch(uint8_t index)
{

}

void do_set_param_init(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  NEO_LOGD(TAG, "do_set_param_init");
  // 将IV18设置为时钟状态
  iv18_reset_ps_timeo();
  clock_set_display_mode(CLOCK_DISPLAY_MODE_TIME);
  sm_set_param_type_index = SM_SET_PARAM_TYPE_MOTION_EN;
  sm_set_param_draw_switch(0);
}

static void do_set_param_set_switch(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  if(ev == EV_EC11_UP || ev == EV_EC11_C || ev == EV_EC11_FAST_C || ev == EV_EC11_CC || ev == EV_EC11_FAST_CC) {
    if(ev == EV_EC11_C || ev == EV_EC11_FAST_C) {
      sm_set_param_type_index = (sm_set_param_type_index + 1) % SM_SET_PARAM_TYPE_CNT;
    } else if(ev == EV_EC11_CC || ev == EV_EC11_FAST_CC) {
      sm_set_param_type_index = (sm_set_param_type_index + SM_SET_PARAM_TYPE_CNT - 1) % SM_SET_PARAM_TYPE_CNT;
    }
    sm_set_param_draw_switch(sm_set_param_type_index);
  } else if(ev == EV_EC11_PRESS) {
    if(sm_set_param_type_index == SM_SET_PARAM_TYPE_QUIT) {
      task_set(EV_V1);
    } else if(sm_set_param_type_index == SM_SET_PARAM_TYPE_IV18_BRIGHT) {
      task_set(EV_V2);
    } else if(sm_set_param_type_index == SM_SET_PARAM_TYPE_PLY_VOL) {
      task_set(EV_V3);
    } else if(sm_set_param_type_index == SM_SET_PARAM_TYPE_TIMER_SND) {
      task_set(EV_V4);
    } else {
      sm_set_param_tiggle_switch(sm_set_param_type_index);
      sm_set_param_draw_switch(sm_set_param_type_index);
    }
  }
}

static void do_set_param_set_iv18_bright(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{

}

static void do_set_param_set_ply_vol(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{

}

static void do_set_param_set_timer_snd(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{

}

static const sm_trans_t sm_trans_set_param_init[] = {
  {EV_EC11_UP, SM_SET_PARAM, SM_SET_PARAM_SET_SWITCH, do_set_param_set_switch},
  {0, 0, 0, NULL}
};

static const sm_trans_t sm_trans_set_param_switch[] = {
  {EV_EC11_C, SM_SET_PARAM, SM_SET_PARAM_SET_SWITCH, do_set_param_set_switch},
  {EV_EC11_FAST_C, SM_SET_PARAM, SM_SET_PARAM_SET_SWITCH, do_set_param_set_switch},
  {EV_EC11_CC, SM_SET_PARAM, SM_SET_PARAM_SET_SWITCH, do_set_param_set_switch},
  {EV_EC11_FAST_CC, SM_SET_PARAM, SM_SET_PARAM_SET_SWITCH, do_set_param_set_switch},
  {EV_EC11_PRESS, SM_SET_PARAM, SM_SET_PARAM_SET_SWITCH, do_set_param_set_switch},
  {EV_V1, SM_FUNC_SELECT, SM_FUNC_SELECT_INIT, do_func_select_init},  
  {EV_V2, SM_SET_PARAM, SM_SET_PARAM_IV18_BRIGHT, do_set_param_set_iv18_bright},  
  {EV_V3, SM_SET_PARAM, SM_SET_PARAM_PLY_VOL, do_set_param_set_ply_vol}, 
  {EV_V4, SM_SET_PARAM, SM_SET_PARAM_TIMER_SND, do_set_param_set_timer_snd},    
  {0, 0, 0, NULL}
};

static const sm_trans_t sm_trans_set_param_iv18_bright[] = {
  {EV_EC11_C, SM_SET_PARAM, SM_SET_PARAM_IV18_BRIGHT, do_set_param_set_iv18_bright},
  {EV_EC11_FAST_C, SM_SET_PARAM, SM_SET_PARAM_IV18_BRIGHT, do_set_param_set_iv18_bright},
  {EV_EC11_CC, SM_SET_PARAM, SM_SET_PARAM_IV18_BRIGHT, do_set_param_set_iv18_bright},
  {EV_EC11_FAST_CC, SM_SET_PARAM, SM_SET_PARAM_IV18_BRIGHT, do_set_param_set_iv18_bright},
  {EV_EC11_PRESS, SM_SET_PARAM, SM_SET_PARAM_IV18_BRIGHT, do_set_param_set_iv18_bright},
  {EV_V1, SM_SET_PARAM, SM_SET_PARAM_SET_SWITCH, do_set_param_set_switch},  
  {0, 0, 0, NULL}
};

static const sm_trans_t sm_trans_set_param_play_vol[] = {
  {EV_EC11_C, SM_SET_PARAM, SM_SET_PARAM_PLY_VOL, do_set_param_set_ply_vol},
  {EV_EC11_FAST_C, SM_SET_PARAM, SM_SET_PARAM_PLY_VOL, do_set_param_set_ply_vol},
  {EV_EC11_CC, SM_SET_PARAM, SM_SET_PARAM_PLY_VOL, do_set_param_set_ply_vol},
  {EV_EC11_FAST_CC, SM_SET_PARAM, SM_SET_PARAM_PLY_VOL, do_set_param_set_ply_vol},
  {EV_EC11_PRESS, SM_SET_PARAM, SM_SET_PARAM_PLY_VOL, do_set_param_set_ply_vol},
  {EV_V1, SM_SET_PARAM, SM_SET_PARAM_SET_SWITCH, do_set_param_set_switch},  
  {0, 0, 0, NULL}
};
static const sm_trans_t sm_trans_set_param_timer_snd[] = {
  {EV_EC11_C, SM_SET_PARAM, SM_SET_PARAM_TIMER_SND, do_set_param_set_timer_snd},
  {EV_EC11_FAST_C, SM_SET_PARAM, SM_SET_PARAM_TIMER_SND, do_set_param_set_timer_snd},
  {EV_EC11_CC, SM_SET_PARAM, SM_SET_PARAM_TIMER_SND, do_set_param_set_timer_snd},
  {EV_EC11_FAST_CC, SM_SET_PARAM, SM_SET_PARAM_TIMER_SND, do_set_param_set_timer_snd},
  {EV_EC11_PRESS, SM_SET_PARAM, SM_SET_PARAM_TIMER_SND, do_set_param_set_timer_snd},
  {EV_V1, SM_SET_PARAM, SM_SET_PARAM_TIMER_SND, do_set_param_set_switch},  
  {0, 0, 0, NULL}
};

const sm_trans_t * sm_trans_set_param[] = {
  sm_trans_set_param_init,
  sm_trans_set_param_switch,
  sm_trans_set_param_iv18_bright,
  sm_trans_set_param_play_vol,
  sm_trans_set_param_timer_snd
};
