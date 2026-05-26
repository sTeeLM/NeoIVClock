#include "ec11.h"
#include "logger.h"
#include "gpio_wrapper.h"
#include "esp_err.h"
#include "driver/gpio.h"
#include "driver/gpio_filter.h"

#include "task.h"
#include "sm.h"
#include "clock.h"
#include "beeper.h"

static const char * TAG = "EC11";

// 连续转多少个刻度之后，变成粗调（增大step）
#define EC11_MAX_FAST_CNT 10
#define EC11_LPRESS_DELAY 3

static uint8_t ec11_key_down;
static uint8_t ec11_key_lpress_send;
static uint32_t ec11_key_down_sec;
static uint8_t ec11_key_down_cnt;
static uint8_t ec11_key_c_cnt;
static uint8_t ec11_key_cc_cnt;

void ec11_key_proc(task_event_t ev)
{

  switch (ev) {
    case EV_EC11_CC:
      NEO_LOGD(TAG, "ec11_key_proc EV_EC11_CC");  
      break;           
    case EV_EC11_C:
      NEO_LOGD(TAG, "ec11_key_proc EV_EC11_C");  
      break; 
    case EV_EC11_FAST_CC:
      NEO_LOGD(TAG, "ec11_key_proc EV_EC11_FAST_CC");  
      break;           
    case EV_EC11_FAST_C:
      NEO_LOGD(TAG, "ec11_key_proc EV_EC11_FAST_C");  
      break; 
    case EV_EC11_DOWN:
      NEO_LOGD(TAG, "ec11_key_proc EV_EC11_DOWN"); 
      beeper_beep(); 
      break; 
    case EV_EC11_PRESS:
      NEO_LOGD(TAG, "ec11_key_proc EV_EC11_PRESS");  
      break;           
    case EV_EC11_LPRESS:
      NEO_LOGD(TAG, "ec11_key_proc EV_EC11_LPRESS");  
      break;     
    case EV_EC11_UP:
      NEO_LOGD(TAG, "ec11_key_proc EV_EC11_UP");  
      break;     
    default:
      ;
  }
  sm_run(ev);
}

void ec11_scan_proc(task_event_t ev)
{
  if(gpio_wrapper_get_level(EC11_C_GPIO_PIN) == 0) {
    if(ec11_key_down == 0) {
      ec11_key_down = 1;
      task_set(EV_EC11_DOWN);
      ec11_key_down_sec = clock_get_now_sec();
    }
  } else {
    if(ec11_key_down == 1) {
      if(!ec11_key_lpress_send) {
        task_set(EV_EC11_PRESS);
      }
      ec11_key_down = 0;
      ec11_key_lpress_send = 0;
      task_set(EV_EC11_UP);
    }
  }
  
  if(ec11_key_down) {
    if(clock_diff_now_sec(ec11_key_down_sec) > EC11_LPRESS_DELAY) {
      task_set(EV_EC11_LPRESS);
      ec11_key_lpress_send = 1;
    }
  }
}

static void IRAM_ATTR ec11_isr_handler_a (void* param)
{
    if(gpio_wrapper_get_level(EC11_A_GPIO_PIN) == 0) {
        if (gpio_wrapper_get_level(EC11_B_GPIO_PIN) != 0) {
            if(ec11_key_cc_cnt < EC11_MAX_FAST_CNT) {
                task_set(EV_EC11_CC);
                ec11_key_cc_cnt ++;
            } else {
                task_set(EV_EC11_FAST_CC);
            }
            ec11_key_c_cnt = 0;
        } else {
            if(ec11_key_c_cnt < EC11_MAX_FAST_CNT) {
                task_set(EV_EC11_C);
                ec11_key_c_cnt ++;
            } else {
                task_set(EV_EC11_FAST_C);
            }
            ec11_key_cc_cnt = 0;
        }
    }
}

static void IRAM_ATTR ec11_isr_handler_c (void* param)
{
    if(gpio_wrapper_get_level(EC11_C_GPIO_PIN) == 0) {
        NEO_EARLY_LOGD("EC11", "Pressed!");
    }
}

void ec11_init(void)
{
    gpio_pin_glitch_filter_config_t glitch_cfg_a = {
        .gpio_num = EC11_A_GPIO_PIN, 
        .clk_src = GLITCH_FILTER_CLK_SRC_DEFAULT,
    };
     gpio_pin_glitch_filter_config_t glitch_cfg_c = {
        .gpio_num = EC11_C_GPIO_PIN, // 10us
        .clk_src = GLITCH_FILTER_CLK_SRC_DEFAULT,
    };
    gpio_glitch_filter_handle_t glitch_handle_a;
    gpio_glitch_filter_handle_t glitch_handle_c;

    NEO_LOGI(TAG, "init");
    ESP_ERROR_CHECK(gpio_intr_enable(EC11_A_GPIO_PIN));
    ESP_ERROR_CHECK(gpio_set_intr_type(EC11_A_GPIO_PIN, GPIO_INTR_NEGEDGE));
    ESP_ERROR_CHECK(gpio_isr_handler_add(EC11_A_GPIO_PIN, ec11_isr_handler_a, NULL));
    ESP_ERROR_CHECK(gpio_new_pin_glitch_filter(&glitch_cfg_a, &glitch_handle_a));
    ESP_ERROR_CHECK(gpio_glitch_filter_enable(glitch_handle_a));

    ESP_ERROR_CHECK(gpio_intr_enable(EC11_C_GPIO_PIN));
    ESP_ERROR_CHECK(gpio_set_intr_type(EC11_C_GPIO_PIN, GPIO_INTR_NEGEDGE));
    ESP_ERROR_CHECK(gpio_isr_handler_add(EC11_C_GPIO_PIN, ec11_isr_handler_c, NULL));
    ESP_ERROR_CHECK(gpio_new_pin_glitch_filter(&glitch_cfg_c, &glitch_handle_c));
    ESP_ERROR_CHECK(gpio_glitch_filter_enable(glitch_handle_c));

    ec11_key_down        = 0;
    ec11_key_lpress_send = 0;
    ec11_key_down_sec    = 0;
    ec11_key_down_cnt    = 0;
    ec11_key_c_cnt       = 0;
    ec11_key_cc_cnt      = 0;    
}

bool ec11_is_factory_reset(void) 
{
    return gpio_wrapper_get_level(EC11_C_GPIO_PIN) == 0; // Return true if factory reset is needed
    //return true;
}