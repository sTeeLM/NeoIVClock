#include "sm_clock.h"
#include "config.h"
#include "task.h"
#include "sm.h"
#include "logger.h"

#include "clock.h"
#include "iv18.h"

static const char * TAG = "SM_CLOCK";

const char * sm_states_names_clock[] = {
  "SM_CLOCK_INIT",
  "SM_CLOCK_TIME_A",
  "SM_CLOCK_TIME_B",
  "SM_CLOCK_DATE_A",
  "SM_CLOCK_DATE_B"
};


void do_clock_init(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  iv18_clr();
  iv18_reset_ps_timeo();
  clock_set_display_mode(CLOCK_DISPLAY_MODE_TIME);
  // draw oled
  
}

static void do_clock_time_a(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  if(ev == EV_V1) {
    // update oled
  } else if (ev == EV_1S) {
    // test timeo
    iv18_test_ps_timeo();
  } else if (ev == EV_ACC) {
    iv18_reset_ps_timeo();
  } else {
    iv18_reset_ps_timeo();
    clock_set_display_mode(CLOCK_DISPLAY_MODE_TIME);
  }
}

static void do_clock_time_b(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  if(ev == EV_V1) {
    // update oled
  } else if (ev == EV_1S) {
    iv18_test_ps_timeo();
  } else if (ev == EV_ACC) {
    iv18_reset_ps_timeo();
  } else {
    iv18_reset_ps_timeo();
    clock_set_display_mode(CLOCK_DISPLAY_MODE_TIME);
  }
}

static void do_clock_date_a(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  if(ev == EV_V1) {
    // update oled
  } else if (ev == EV_1S) {
    iv18_test_ps_timeo();
  } else if (ev == EV_ACC) {
    iv18_reset_ps_timeo();
  } else {
    iv18_reset_ps_timeo();
    clock_set_display_mode(CLOCK_DISPLAY_MODE_DATE);
  }
}

static void do_clock_date_b(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  if(ev == EV_V1) {
    // update oled
  } else if (ev == EV_1S) {
    iv18_test_ps_timeo();
  } else if (ev == EV_ACC) {
    iv18_reset_ps_timeo();
  } else {
    iv18_reset_ps_timeo();
    clock_set_display_mode(CLOCK_DISPLAY_MODE_DATE);
  }
}

static sm_trans_t sm_trans_clock_init[] = {
  {EV_EC11_UP, SM_CLOCK , SM_CLOCK_TIME_A, do_clock_init},
  {0, 0, 0, NULL}
};

static sm_trans_t sm_trans_clock_time_a[] = {
  {EV_V1, SM_CLOCK , SM_CLOCK_TIME_A, do_clock_time_a},
  {EV_1S, SM_CLOCK , SM_CLOCK_TIME_A, do_clock_time_a}, 
  {EV_ACC, SM_CLOCK , SM_CLOCK_TIME_A, do_clock_time_a}, 
  {EV_EC11_C, SM_CLOCK , SM_CLOCK_TIME_B, do_clock_time_b},
  {EV_EC11_CC, SM_CLOCK , SM_CLOCK_DATE_B, do_clock_date_b},
  {0, 0, 0, NULL}
};

static sm_trans_t sm_trans_clock_time_b[] = {
  {EV_V1, SM_CLOCK , SM_CLOCK_TIME_B, do_clock_time_b},
  {EV_1S, SM_CLOCK , SM_CLOCK_TIME_B, do_clock_time_b}, 
  {EV_ACC, SM_CLOCK , SM_CLOCK_TIME_B, do_clock_time_b},
  {EV_EC11_C, SM_CLOCK , SM_CLOCK_DATE_A, do_clock_date_a},
  {EV_EC11_CC, SM_CLOCK , SM_CLOCK_TIME_A, do_clock_time_a},
  {0, 0, 0, NULL}
};

static sm_trans_t sm_trans_clock_date_a[] = {
  {EV_V1, SM_CLOCK , SM_CLOCK_DATE_A, do_clock_date_a},
  {EV_1S, SM_CLOCK , SM_CLOCK_DATE_A, do_clock_date_a},
  {EV_ACC, SM_CLOCK , SM_CLOCK_DATE_A, do_clock_date_a},  
  {EV_EC11_C, SM_CLOCK , SM_CLOCK_DATE_B, do_clock_date_b},
  {EV_EC11_CC, SM_CLOCK , SM_CLOCK_TIME_B, do_clock_time_b},
  {0, 0, 0, NULL}
};

static sm_trans_t sm_trans_clock_date_b[] = {
  {EV_V1, SM_CLOCK , SM_CLOCK_DATE_B, do_clock_date_b},
  {EV_1S, SM_CLOCK , SM_CLOCK_DATE_B, do_clock_date_b},
  {EV_ACC, SM_CLOCK , SM_CLOCK_DATE_B, do_clock_date_b},    
  {EV_EC11_C, SM_CLOCK , SM_CLOCK_TIME_A, do_clock_time_a},
  {EV_EC11_CC, SM_CLOCK , SM_CLOCK_DATE_A, do_clock_date_a},
  {0, 0, 0, NULL}
};

sm_trans_t * sm_trans_clock[] = {
  sm_trans_clock_init,
  sm_trans_clock_time_a,
  sm_trans_clock_time_b,
  sm_trans_clock_date_a,
  sm_trans_clock_date_b,    
};
