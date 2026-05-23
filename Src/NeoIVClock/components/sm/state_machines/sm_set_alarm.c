#include "sm_set_alarm.h"
#include "task.h"
#include "sm.h"
#include "logger.h"
#include "clock.h"
#include "iv18.h"
#include "alarm.h"

#include "sm_func_select.h"

const char * TAG = "SM_SET_ALARM";

/*
  SM_SET_ALARM_INIT,
  SM_SET_ALARM_SEL_ALARM1,

  SM_SET_ALARM_SET_ALARM1_HOUR,
  SM_SET_ALARM_SET_ALARM1_MIN,
  SM_SET_ALARM_SET_ALARM1_DAY,
  SM_SET_ALARM_ALARM0
*/
const char * sm_states_names_set_alarm[] = {
  "SM_SET_ALARM_INIT",
  "SM_SET_ALARM_SEL_TYPE",
  "SM_SET_ALARM_SEL_ALARM1",
  "SM_SET_ALARM_SEL_ALARM1_TYPE",
  "SM_SET_ALARM_SET_ALARM1_EN",
  "SM_SET_ALARM_SET_ALARM1_HOUR",
  "SM_SET_ALARM_SET_ALARM1_MIN",
  "SM_SET_ALARM_SET_ALARM1_DAY",
  "SM_SET_ALARM_SET_ALARM1_SND",  
  "SM_SET_ALARM_SET_ALARM0"
};


// ALARM0/ALARM1选择画面中的选项？
// 0: 设置整点报时
// 1: 设置普通闹钟
// 2: 退回上一级
static uint8_t alarm_sel_type_index;
// 画 ALARM0/ALARM1 选择画面
static void sm_set_alarm_draw_sel_type(uint8_t sel)
{
  // OLED 显示三个选项：设置整点报时、设置普通闹钟、退回上一级，根据sel决定高亮哪一个选项
}

// ALARM1 的第几个？
// 最大ALARM1_MAX_COUNT, 0 ~ (ALARM1_MAX_COUNT - 1) 是闹钟index
// ALARM1_MAX_COUNT退回上一级
static uint8_t alarm_sel_alarm1_index;
// 画 ALARM1 index 选择画面
static void sm_set_alarm_draw_sel_alarm1(uint8_t sel) 
{

}

// 选择ALARM1的哪一个属性来设置？
// 0: On/Off
// 1: Hour
// 2: Min
// 3: Day
// 4: Snd
// 5: 退回上一级
static uint8_t alarm_sel_alarm1_type_index;
// 画 ALARM1 字段选择画面
static void sm_set_alarm_draw_sel_alarm1_type(uint8_t sel)
{
  
}

// 画ALARM1 on/off的画面
static void sm_set_alarm_draw_sel_alarm1_en(uint8_t index, bool on)
{

}

// 画ALARM1 Hour的画面
static void sm_set_alarm_draw_sel_alarm1_hour(uint8_t index, uint8_t hour)
{

}

// 画ALARM1 Min的画面
static void sm_set_alarm_draw_sel_alarm1_min(uint8_t index, uint8_t min)
{

}

// 当前的day: 0~7
// 7 表示退出
static uint8_t alarm_sel_alarm1_type_day;
// 画ALARM1 Day的画面
static void sm_set_alarm_draw_sel_alarm1_day(uint8_t index, uint8_t mask, uint8_t cur_day)
{

}

// 画ALARM1 Snd的画面
static void sm_set_alarm_draw_sel_alarm1_snd(uint8_t index, uint8_t snd)
{

}

static void sm_set_alarm_draw_sel_alarm0_en(bool on)
{

}

void do_set_alarm_init(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  NEO_LOGD(TAG, "do_set_alarm_init");

  // 将IV18设置为时钟状态
  iv18_reset_ps_timeo();
  clock_set_display_mode(CLOCK_DISPLAY_MODE_TIME);
  alarm_sel_type_index   = 0;
  alarm_sel_alarm1_index = 0;
  alarm_sel_alarm1_type_index = 0;
  sm_set_alarm_draw_sel_type(0);
}


/*
  EV_V1: return from SM_SET_ALARM_SET_ALARM0
  EV_V2: return from SM_SET_ALARM_SEL_ALARM1
  EV_EC11_UP: jump from SM_FUNC_SELECT|SM_FUNC_SELECT_ALARM
*/
static void do_set_alarm_sel_type(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  if(ev == EV_V1 || ev == EV_V2 || ev == EV_EC11_UP || ev == EV_EC11_C || ev == EV_EC11_FAST_C || ev == EV_EC11_CC || ev == EV_EC11_FAST_CC) {
    if(ev == EV_EC11_C || ev == EV_EC11_FAST_C ) {
      alarm_sel_type_index = (alarm_sel_type_index + 1) % 3;
    } else if(ev == EV_EC11_CC || ev == EV_EC11_FAST_CC ) { 
      alarm_sel_type_index = (alarm_sel_type_index + 2) % 3;
    } 
    sm_set_alarm_draw_sel_type(alarm_sel_type_index);
    if(ev == EV_V1 || ev == EV_V2 || ev == EV_EC11_UP) {
      // 将IV18设置为时钟状态
      iv18_reset_ps_timeo();
      clock_set_display_mode(CLOCK_DISPLAY_MODE_TIME);
    }
  } else if(ev == EV_EC11_PRESS) {
    switch(alarm_sel_type_index) {
      case 0: task_set(EV_V1); break; // jump to SM_SET_ALARM_SET_ALARM0
      case 1: task_set(EV_V2); break; // jump to SM_SET_ALARM_SEL_ALARM1
      case 2: task_set(EV_V3); break; // quit to SM_FUNC_SELECT|SM_FUNC_SELECT_INIT
      default:
        NEO_LOGW(TAG, "invalid alarm_sel_type_index %d", alarm_sel_type_index);
      break;          
    }
  }
}

/*
  EV_V2: jump from SM_SET_ALARM_SEL_TYPE
  EV_V6: return from SM_SET_ALARM_SEL_ALARM1_TYPE
*/
static void do_set_alarm_sel_alarm1(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  if(ev == EV_V2 || ev == EV_V6 || ev == EV_EC11_C || ev == EV_EC11_FAST_C || ev == EV_EC11_CC || ev == EV_EC11_FAST_CC) {
    if(ev == EV_EC11_C || ev == EV_EC11_FAST_C) {
      alarm_sel_alarm1_index = (alarm_sel_alarm1_index + 1) % (ALARM1_MAX_COUNT + 1);
    } else if(ev == EV_EC11_CC || ev == EV_EC11_FAST_CC) {
      alarm_sel_alarm1_index = (alarm_sel_alarm1_index + ALARM1_MAX_COUNT) % (ALARM1_MAX_COUNT + 1);
    }
    sm_set_alarm_draw_sel_alarm1(alarm_sel_alarm1_index);
  } else if(ev == EV_EC11_PRESS) {
    if(alarm_sel_alarm1_index < ALARM1_MAX_COUNT)
      task_set(EV_V1); // jump to SM_SET_ALARM_SEL_ALARM1_TYPE
    else {
      task_set(EV_V2); // quit back to SM_SET_ALARM_SEL_TYPE
    }
  }
}

/*
EV_V1: jump from 
  SM_SET_ALARM_SET_ALARM1_EN
  SM_SET_ALARM_SET_ALARM1_HOUR
  SM_SET_ALARM_SET_ALARM1_MIN
  SM_SET_ALARM_SET_ALARM1_DAY
  SM_SET_ALARM_SET_ALARM1_SND
*/
static void do_set_alarm_sel_alarm1_type(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{

  if(ev == EV_V1 || ev == EV_EC11_C || ev == EV_EC11_FAST_C || ev == EV_EC11_CC || ev == EV_EC11_FAST_CC) {
    if(ev == EV_EC11_C || ev == EV_EC11_FAST_C) {
      alarm_sel_alarm1_type_index = (alarm_sel_alarm1_type_index + 1) % 6;
    } else if(ev == EV_EC11_CC || ev == EV_EC11_FAST_CC) {
      alarm_sel_alarm1_type_index = (alarm_sel_alarm1_type_index + 5) % 6;
    }
    sm_set_alarm_draw_sel_alarm1_type(alarm_sel_alarm1_type_index);
  } else if(ev == EV_EC11_PRESS) {
    switch(alarm_sel_alarm1_type_index) {
      case 0: 
        task_set(EV_V1);  // jmp to SM_SET_ALARM_SET_ALARM1_EN
        break;
      case 1: 
        task_set(EV_V2);  // jmp to SM_SET_ALARM_SET_ALARM1_HOUR
        break;
      case 2: 
        task_set(EV_V3);  // jmp to SM_SET_ALARM_SET_ALARM1_MIN
        break;
      case 3: 
        task_set(EV_V4);  // jmp to SM_SET_ALARM_SET_ALARM1_DAY
        alarm_sel_alarm1_type_day = 0; // need clear day index
        break;
      case 4: 
        task_set(EV_V5); // jmp to SM_SET_ALARM_SET_ALARM1_SND
        break;
      case 5: 
        task_set(EV_V6); // return to SM_SET_ALARM_SEL_ALARM1
        break;
      default:
        NEO_LOGW(TAG, "invalid alarm_sel_alarm1_type_index %d", alarm_sel_alarm1_type_index);
        break;
    }
  }
}

/*
  EV_V1: jump from SM_SET_ALARM_SEL_ALARM1_TYPE
*/
static void do_set_alarm_set_alarm1_en(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  bool is_on = alarm1_get_enabled(alarm_sel_alarm1_index);
  if(ev == EV_V1 || ev == EV_EC11_C || ev == EV_EC11_FAST_C || ev == EV_EC11_CC || ev == EV_EC11_FAST_CC) {
    if(ev != EV_V1) {
      is_on = alarm1_enable(alarm_sel_alarm1_index, !is_on);
    }
    sm_set_alarm_draw_sel_alarm1_en(alarm_sel_alarm1_index, is_on);
  } else if(ev == EV_EC11_PRESS) {
    alarm1_save_config(alarm_sel_alarm1_index);
    task_set(EV_V1); // quit back to SM_SET_ALARM_SEL_ALARM1_TYPE
  }
}

/*
  EV_V2: jump from SM_SET_ALARM_SEL_ALARM1_TYPE
*/
static void do_set_alarm_set_alarm1_hour(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  uint8_t hour = alarm1_get_hour(alarm_sel_alarm1_index);
  if(ev == EV_V2 || ev == EV_EC11_C || ev == EV_EC11_FAST_C || ev == EV_EC11_CC || ev == EV_EC11_FAST_CC) {
    if(ev == EV_EC11_C || ev == EV_EC11_FAST_C) {
      hour = alarm1_inc_hour(alarm_sel_alarm1_index, ev == EV_EC11_FAST_C);
    } else if(ev == EV_EC11_CC || ev == EV_EC11_FAST_CC) {
      hour = alarm1_dec_hour(alarm_sel_alarm1_index, ev == EV_EC11_FAST_CC);
    }
    sm_set_alarm_draw_sel_alarm1_hour(alarm_sel_alarm1_index, hour);
  } else if(ev == EV_EC11_PRESS) {
    alarm1_save_config(alarm_sel_alarm1_index);
    task_set(EV_V1); // quit back to SM_SET_ALARM_SEL_ALARM1_TYPE
  }
}

/*
  EV_V3: jump from SM_SET_ALARM_SEL_ALARM1_TYPE
*/
static void do_set_alarm_set_alarm1_min(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  uint8_t min = alarm1_get_hour(alarm_sel_alarm1_index);
  if(ev == EV_V3 || ev == EV_EC11_C || ev == EV_EC11_FAST_C || ev == EV_EC11_CC || ev == EV_EC11_FAST_CC) {
    if(ev == EV_EC11_C || ev == EV_EC11_FAST_C) {
      min = alarm1_inc_min(alarm_sel_alarm1_index, ev == EV_EC11_FAST_C);
    } else if(ev == EV_EC11_CC || ev == EV_EC11_FAST_CC) {
      min = alarm1_dec_min(alarm_sel_alarm1_index, ev == EV_EC11_FAST_CC);
    }
    sm_set_alarm_draw_sel_alarm1_min(alarm_sel_alarm1_index, min);
  } else if(ev == EV_EC11_PRESS) {
    alarm1_save_config(alarm_sel_alarm1_index);
    task_set(EV_V1); // quit back to SM_SET_ALARM_SEL_ALARM1_TYPE
  }
}

/*
  EV_V4: jump from SM_SET_ALARM_SEL_ALARM1_TYPE
*/
static void do_set_alarm_set_alarm1_day(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  uint8_t  day_mask = alarm1_get_day_mask(alarm_sel_alarm1_index);
  if(ev == EV_V4 || ev == EV_EC11_C || ev == EV_EC11_FAST_C || ev == EV_EC11_CC || ev == EV_EC11_FAST_CC) {
    if(ev == EV_EC11_C || ev == EV_EC11_FAST_C) {
      alarm_sel_alarm1_type_day = (alarm_sel_alarm1_type_day + 1) % 8;
    } else if(ev == EV_EC11_CC || ev == EV_EC11_FAST_CC) {
      alarm_sel_alarm1_type_day = (alarm_sel_alarm1_type_day + 7) % 8; 
    }
    if(alarm_sel_alarm1_type_day != 7) {
      // 将day_mask中对应位置的bit反转
      day_mask ^= (1 << alarm_sel_alarm1_type_day);
      alarm1_set_day_mask(alarm_sel_alarm1_index, day_mask);
    }    
    sm_set_alarm_draw_sel_alarm1_day(alarm_sel_alarm1_index, day_mask, alarm_sel_alarm1_type_day);
  } else if(ev == EV_EC11_PRESS) {
    if(alarm_sel_alarm1_type_day >= 7) {
      alarm1_save_config(alarm_sel_alarm1_index);
      task_set(EV_V1); // quit back to SM_SET_ALARM_SEL_ALARM1_TYPE
    }
  }
}

/*
  EV_V5: jump from SM_SET_ALARM_SEL_ALARM1_TYPE
*/
static void do_set_alarm_set_alarm1_snd(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  uint8_t snd = alarm1_get_snd(alarm_sel_alarm1_index);
  if(ev == EV_V5 || ev == EV_EC11_C || ev == EV_EC11_FAST_C || ev == EV_EC11_CC || ev == EV_EC11_FAST_CC) {
    if(ev == EV_EC11_C || ev == EV_EC11_FAST_C) {
      snd = alarm1_inc_snd(alarm_sel_alarm1_index, ev == EV_EC11_FAST_C);
    } else if(ev == EV_EC11_CC || ev == EV_EC11_FAST_CC) {
      snd = alarm1_dec_snd(alarm_sel_alarm1_index, ev == EV_EC11_FAST_CC);
    }
    sm_set_alarm_draw_sel_alarm1_snd(alarm_sel_alarm1_index, snd);
  } else if(ev == EV_EC11_PRESS) {
    alarm1_save_config(alarm_sel_alarm1_index);
    task_set(EV_V1); // quit back to SM_SET_ALARM_SEL_ALARM1_TYPE
  }
}

/*
  EV_V1: jump from SM_SET_ALARM_SEL_TYPE
*/
static void do_set_alarm_set_alarm0(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  bool is_on = alarm0_get_enabled();
  if(ev == EV_V1 || ev == EV_EC11_C || ev == EV_EC11_FAST_C || ev == EV_EC11_CC || ev == EV_EC11_FAST_CC) {
    if(ev != EV_V1) {
      is_on = alarm0_enable(!is_on);
    }
    sm_set_alarm_draw_sel_alarm0_en(is_on);
  } else if(ev == EV_EC11_PRESS) {
    alarm0_save_config();
    task_set(EV_V1); // quit back to SM_SET_ALARM_SEL_TYPE
  }
}

static const sm_trans_t sm_trans_set_alarm_init[] = {
  {EV_EC11_UP, SM_SET_ALARM, SM_SET_ALARM_SEL_TYPE, do_set_alarm_sel_type},
  {0, 0, 0, NULL}
};

static const sm_trans_t sm_trans_set_alarm_sel_type[] = {
  {EV_EC11_C, SM_SET_ALARM, SM_SET_ALARM_SEL_TYPE, do_set_alarm_sel_type},
  {EV_EC11_FAST_C, SM_SET_ALARM, SM_SET_ALARM_SEL_TYPE, do_set_alarm_sel_type},
  {EV_EC11_CC, SM_SET_ALARM, SM_SET_ALARM_SEL_TYPE, do_set_alarm_sel_type},
  {EV_EC11_FAST_CC, SM_SET_ALARM, SM_SET_ALARM_SEL_TYPE, do_set_alarm_sel_type},
  {EV_EC11_PRESS, SM_SET_ALARM, SM_SET_ALARM_SEL_TYPE, do_set_alarm_sel_type},
  {EV_V1, SM_SET_ALARM, SM_SET_ALARM_SET_ALARM0, do_set_alarm_set_alarm0}, 
  {EV_V2, SM_SET_ALARM, SM_SET_ALARM_SEL_ALARM1, do_set_alarm_sel_alarm1},
  {EV_V3, SM_FUNC_SELECT, SM_FUNC_SELECT_INIT, do_func_select_init},      
  {0, 0, 0, NULL}
};

static const sm_trans_t sm_trans_set_alarm_sel_alarm1[] = {
  {EV_EC11_C, SM_SET_ALARM, SM_SET_ALARM_SEL_ALARM1, do_set_alarm_sel_alarm1},
  {EV_EC11_FAST_C, SM_SET_ALARM, SM_SET_ALARM_SEL_ALARM1, do_set_alarm_sel_alarm1},
  {EV_EC11_CC, SM_SET_ALARM, SM_SET_ALARM_SEL_ALARM1, do_set_alarm_sel_alarm1},
  {EV_EC11_FAST_CC, SM_SET_ALARM, SM_SET_ALARM_SEL_ALARM1, do_set_alarm_sel_alarm1},
  {EV_EC11_PRESS, SM_SET_ALARM, SM_SET_ALARM_SEL_ALARM1, do_set_alarm_sel_alarm1},
  {EV_V1, SM_SET_ALARM, SM_SET_ALARM_SEL_ALARM1_TYPE, do_set_alarm_sel_alarm1_type},
  {EV_V2, SM_SET_ALARM, SM_SET_ALARM_SEL_TYPE, do_set_alarm_sel_type},
  {0, 0, 0, NULL}
};

static const sm_trans_t sm_trans_set_alarm_sel_alarm1_type[] = {
  {EV_EC11_C, SM_SET_ALARM, SM_SET_ALARM_SEL_ALARM1_TYPE, do_set_alarm_sel_alarm1_type},
  {EV_EC11_FAST_C, SM_SET_ALARM, SM_SET_ALARM_SEL_ALARM1_TYPE, do_set_alarm_sel_alarm1_type},
  {EV_EC11_CC, SM_SET_ALARM, SM_SET_ALARM_SEL_ALARM1_TYPE, do_set_alarm_sel_alarm1_type},
  {EV_EC11_FAST_CC, SM_SET_ALARM, SM_SET_ALARM_SEL_ALARM1_TYPE, do_set_alarm_sel_alarm1_type},
  {EV_EC11_PRESS, SM_SET_ALARM, SM_SET_ALARM_SEL_ALARM1_TYPE, do_set_alarm_sel_alarm1_type},
  {EV_V1, SM_SET_ALARM, SM_SET_ALARM_SET_ALARM1_EN, do_set_alarm_set_alarm1_en},
  {EV_V2, SM_SET_ALARM, SM_SET_ALARM_SET_ALARM1_HOUR, do_set_alarm_set_alarm1_hour},
  {EV_V3, SM_SET_ALARM, SM_SET_ALARM_SET_ALARM1_MIN, do_set_alarm_set_alarm1_min},
  {EV_V4, SM_SET_ALARM, SM_SET_ALARM_SET_ALARM1_DAY, do_set_alarm_set_alarm1_day},
  {EV_V5, SM_SET_ALARM, SM_SET_ALARM_SET_ALARM1_SND, do_set_alarm_set_alarm1_snd},  
  {EV_V6, SM_SET_ALARM, SM_SET_ALARM_SEL_ALARM1, do_set_alarm_sel_alarm1},
  {0, 0, 0, NULL}
};

static const sm_trans_t sm_trans_set_alarm_set_alarm1_en[] = {
  {EV_EC11_C, SM_SET_ALARM, SM_SET_ALARM_SET_ALARM1_EN, do_set_alarm_set_alarm1_en},
  {EV_EC11_FAST_C, SM_SET_ALARM, SM_SET_ALARM_SET_ALARM1_EN, do_set_alarm_set_alarm1_en},
  {EV_EC11_CC, SM_SET_ALARM, SM_SET_ALARM_SET_ALARM1_EN, do_set_alarm_set_alarm1_en},
  {EV_EC11_FAST_CC, SM_SET_ALARM, SM_SET_ALARM_SET_ALARM1_EN, do_set_alarm_set_alarm1_en},
  {EV_EC11_PRESS, SM_SET_ALARM, SM_SET_ALARM_SET_ALARM1_EN, do_set_alarm_set_alarm1_en},
  {EV_V1, SM_SET_ALARM, SM_SET_ALARM_SEL_ALARM1_TYPE, do_set_alarm_sel_alarm1_type},
  {0, 0, 0, NULL}
};

static const sm_trans_t sm_trans_set_alarm_set_alarm1_hour[] = {
  {EV_EC11_C, SM_SET_ALARM, SM_SET_ALARM_SET_ALARM1_HOUR, do_set_alarm_set_alarm1_hour},
  {EV_EC11_FAST_C, SM_SET_ALARM, SM_SET_ALARM_SET_ALARM1_HOUR, do_set_alarm_set_alarm1_hour},
  {EV_EC11_CC, SM_SET_ALARM, SM_SET_ALARM_SET_ALARM1_HOUR, do_set_alarm_set_alarm1_hour},
  {EV_EC11_FAST_CC, SM_SET_ALARM, SM_SET_ALARM_SET_ALARM1_HOUR, do_set_alarm_set_alarm1_hour},
  {EV_EC11_PRESS, SM_SET_ALARM, SM_SET_ALARM_SET_ALARM1_HOUR, do_set_alarm_set_alarm1_hour},
  {EV_V1, SM_SET_ALARM, SM_SET_ALARM_SEL_ALARM1_TYPE, do_set_alarm_sel_alarm1_type},
  {0, 0, 0, NULL}
};

static const sm_trans_t sm_trans_set_alarm_set_alarm1_min[] = {
  {EV_EC11_C, SM_SET_ALARM, SM_SET_ALARM_SET_ALARM1_MIN, do_set_alarm_set_alarm1_min},
  {EV_EC11_FAST_C, SM_SET_ALARM, SM_SET_ALARM_SET_ALARM1_MIN, do_set_alarm_set_alarm1_min},
  {EV_EC11_CC, SM_SET_ALARM, SM_SET_ALARM_SET_ALARM1_MIN, do_set_alarm_set_alarm1_min},
  {EV_EC11_FAST_CC, SM_SET_ALARM, SM_SET_ALARM_SET_ALARM1_MIN, do_set_alarm_set_alarm1_min},
  {EV_EC11_PRESS, SM_SET_ALARM, SM_SET_ALARM_SET_ALARM1_MIN, do_set_alarm_set_alarm1_min},
  {EV_V1, SM_SET_ALARM, SM_SET_ALARM_SEL_ALARM1_TYPE, do_set_alarm_sel_alarm1_type},
  {0, 0, 0, NULL}
};

static const sm_trans_t sm_trans_set_alarm_set_alarm1_day[] = {
  {EV_EC11_C, SM_SET_ALARM, SM_SET_ALARM_SET_ALARM1_DAY, do_set_alarm_set_alarm1_day},
  {EV_EC11_FAST_C, SM_SET_ALARM, SM_SET_ALARM_SET_ALARM1_DAY, do_set_alarm_set_alarm1_day},
  {EV_EC11_CC, SM_SET_ALARM, SM_SET_ALARM_SET_ALARM1_DAY, do_set_alarm_set_alarm1_day},
  {EV_EC11_FAST_CC, SM_SET_ALARM, SM_SET_ALARM_SET_ALARM1_DAY, do_set_alarm_set_alarm1_day},
  {EV_EC11_PRESS, SM_SET_ALARM, SM_SET_ALARM_SET_ALARM1_DAY, do_set_alarm_set_alarm1_day},
  {EV_V1, SM_SET_ALARM, SM_SET_ALARM_SEL_ALARM1_TYPE, do_set_alarm_sel_alarm1_type},
  {0, 0, 0, NULL}
};

static const sm_trans_t sm_trans_set_alarm_set_alarm1_snd[] = {
  {EV_EC11_C, SM_SET_ALARM, SM_SET_ALARM_SET_ALARM1_SND, do_set_alarm_set_alarm1_snd},
  {EV_EC11_FAST_C, SM_SET_ALARM, SM_SET_ALARM_SET_ALARM1_SND, do_set_alarm_set_alarm1_snd},
  {EV_EC11_CC, SM_SET_ALARM, SM_SET_ALARM_SET_ALARM1_SND, do_set_alarm_set_alarm1_snd},
  {EV_EC11_FAST_CC, SM_SET_ALARM, SM_SET_ALARM_SET_ALARM1_SND, do_set_alarm_set_alarm1_snd},
  {EV_EC11_PRESS, SM_SET_ALARM, SM_SET_ALARM_SET_ALARM1_SND, do_set_alarm_set_alarm1_snd},
  {EV_V1, SM_SET_ALARM, SM_SET_ALARM_SEL_ALARM1_TYPE, do_set_alarm_sel_alarm1_type},
  {0, 0, 0, NULL}
};

static const sm_trans_t sm_trans_set_alarm_set_alarm0[] = {
  {EV_EC11_C, SM_SET_ALARM, SM_SET_ALARM_SET_ALARM0, do_set_alarm_set_alarm0},
  {EV_EC11_FAST_C, SM_SET_ALARM, SM_SET_ALARM_SET_ALARM0, do_set_alarm_set_alarm0},
  {EV_EC11_CC, SM_SET_ALARM, SM_SET_ALARM_SET_ALARM0, do_set_alarm_set_alarm0},
  {EV_EC11_FAST_CC, SM_SET_ALARM, SM_SET_ALARM_SET_ALARM0, do_set_alarm_set_alarm0},
  {EV_EC11_PRESS, SM_SET_ALARM, SM_SET_ALARM_SET_ALARM0, do_set_alarm_set_alarm0},
  {EV_V1, SM_SET_ALARM, SM_SET_ALARM_SEL_TYPE, do_set_alarm_sel_type},
  {0, 0, 0, NULL}
};

const sm_trans_t * sm_trans_set_alarm[] = {
  sm_trans_set_alarm_init,
  sm_trans_set_alarm_sel_type,
  sm_trans_set_alarm_sel_alarm1,
  sm_trans_set_alarm_sel_alarm1_type,
  sm_trans_set_alarm_set_alarm1_en,
  sm_trans_set_alarm_set_alarm1_hour,
  sm_trans_set_alarm_set_alarm1_min,
  sm_trans_set_alarm_set_alarm1_day,
  sm_trans_set_alarm_set_alarm1_snd, 
  sm_trans_set_alarm_set_alarm0
};
