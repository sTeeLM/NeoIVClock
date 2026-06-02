#include "beeper.h"
#include "logger.h"
#include "config.h"
#include "driver/ledc.h"
#include "driver/gptimer.h"
#include "gpio_wrapper.h"
#include "delay.h"

#include "esp_rom_gpio.h"
#include "soc/gpio_sig_map.h"
#include <stdatomic.h>

static const char * TAG = "BEEPER";

typedef struct _beeper_cnt_t{
  uint32_t count;
} beeper_cnt_t;

static beeper_cnt_t beeper_cnt;
static gptimer_handle_t beeper_gptimer;
static bool beeper_is_on = false;

static atomic_flag beeper_lock = ATOMIC_FLAG_INIT;

static bool beeper_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx)
{
  beeper_cnt_t * cnt = (beeper_cnt_t *)user_ctx;
  if(cnt->count == 0) {
    ESP_ERROR_CHECK(gptimer_stop(beeper_gptimer)); 
    ESP_ERROR_CHECK(gptimer_disable(beeper_gptimer));
    ESP_ERROR_CHECK(ledc_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, 0));
    atomic_flag_clear(&beeper_lock);
  } else if(cnt->count % 2 == 0) {
    ESP_ERROR_CHECK(ledc_timer_pause(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1)); 
    cnt->count--;
  } else {
    ESP_ERROR_CHECK(ledc_timer_resume(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1));
    cnt->count--;
  }
  return false; // 返回false表示不需要yield
}

void beeper_init(void)
{
 // Prepare and then apply the LEDC PWM timer configuration
  ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .duty_resolution  = LEDC_TIMER_13_BIT,
        .timer_num        = LEDC_TIMER_1,
        .freq_hz          = 850, // Frequency in Hertz. Set frequency at 1 kHz
        .clk_cfg          = LEDC_AUTO_CLK
  };
  
  // Prepare and then apply the LEDC PWM channel configuration
  ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL_1,
        .timer_sel      = LEDC_TIMER_1,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = BEEPER_GPIO_PIN,
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0
  };  

  
  gptimer_config_t timer_config = {
      .clk_src   =  GPTIMER_CLK_SRC_XTAL,
      .direction =  GPTIMER_COUNT_UP,
      .resolution_hz = 32768, // 设置分辨率为 32768Hz，这样 1 个 tick 就是 1 个脉冲
  };

  gptimer_alarm_config_t alarm_config = {
      .alarm_count = 2048,                 // 达到 1 次计数（即 500ms）触发中断
      .reload_count = 0,                  // 硬件自动重载回 0
      .flags.auto_reload_on_alarm = true, // 开启【硬件层面】自动重载
  };

  gptimer_event_callbacks_t timer_cbs = {
    .on_alarm = beeper_cb, // register user callback
  };

  NEO_LOGI(TAG, "init");

  ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &beeper_gptimer));
  ESP_ERROR_CHECK(gptimer_set_alarm_action(beeper_gptimer, &alarm_config));
  ESP_ERROR_CHECK(gptimer_register_event_callbacks(beeper_gptimer, &timer_cbs, &beeper_cnt));

  ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));
  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
  ESP_ERROR_CHECK(ledc_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, 0));

  beeper_is_on = config_read_int("bp_en") != 0;
  NEO_LOGD(TAG, "beeper_is_on = %d", beeper_is_on);
}


void beeper_beep(void)
{
  NEO_LOGD(TAG, "beeper_beep");
  if(!beeper_is_on) {
    return;
  }

  if (!atomic_flag_test_and_set(&beeper_lock)) {
    beeper_cnt.count = 1;
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, 4096));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1));  
    ESP_ERROR_CHECK(gptimer_enable(beeper_gptimer));
    ESP_ERROR_CHECK(gptimer_start(beeper_gptimer)); 
  } else {
    NEO_EARLY_LOGW(TAG, "beeper_beep too much");
  }
}

void beeper_beep_beep(void)
{
  NEO_LOGD(TAG, "beeper_beep_beep");
  if(!beeper_is_on) {
    return;
  }  
  if (!atomic_flag_test_and_set(&beeper_lock)) {
    beeper_cnt.count = 3;
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, 4096));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1));  
    ESP_ERROR_CHECK(gptimer_enable(beeper_gptimer));
    ESP_ERROR_CHECK(gptimer_start(beeper_gptimer));
  } else {
    NEO_EARLY_LOGW(TAG, "beeper_beep_beep too much");
  } 
}

// 发出咔嗒声
void beeper_ta(void)
{
  if(!beeper_is_on) {
    return;
  }    
  if (!atomic_flag_test_and_set(&beeper_lock)) {
    esp_rom_gpio_connect_out_signal(BEEPER_GPIO_PIN, SIG_GPIO_OUT_IDX, false, false);
    gpio_wrapper_set_level(BEEPER_GPIO_PIN, 1);
    delay_ms(1);
    gpio_wrapper_set_level(BEEPER_GPIO_PIN, 0);
    esp_rom_gpio_connect_out_signal(BEEPER_GPIO_PIN, LEDC_LS_SIG_OUT0_IDX + LEDC_CHANNEL_1, false, false);
    atomic_flag_clear(&beeper_lock);
  } else {
    NEO_EARLY_LOGW(TAG, "beeper_ta too much");
  } 
}

bool beeper_test_enable(void)
{
  return beeper_is_on;
}

bool beeper_enable(bool enable)
{
  beeper_is_on = enable;
  return beeper_is_on;
}
void beeper_save_config()
{
  config_write_int("bp_en", beeper_is_on != 0);
}