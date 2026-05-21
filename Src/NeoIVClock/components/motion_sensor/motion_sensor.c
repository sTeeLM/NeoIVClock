#include "motion_sensor.h"
#include "logger.h"
#include "config.h"
#include "gpio_wrapper.h"
#include "esp_err.h"
#include "driver/gpio.h"
#include "driver/gpio_filter.h"
#include "task.h"

static const char * TAG = "MOTION";
static bool motion_en = false;
static void IRAM_ATTR motion_sensor_isr_handler (void* param)
{
  if(!motion_en) {
    return;
  }
  NEO_EARLY_LOGD(TAG, "Motion sensor triggered!");
  task_set(EV_ACC);
}

void motion_sensor_init(void)
{
  gpio_pin_glitch_filter_config_t glitch_cfg_motion = {
    .gpio_num = MOTION_SW_GPIO_PIN, 
    .clk_src = GLITCH_FILTER_CLK_SRC_DEFAULT,
  };
  gpio_glitch_filter_handle_t glitch_handle_motion;

  NEO_LOGI(TAG, "init");
  ESP_ERROR_CHECK(gpio_intr_enable(MOTION_SW_GPIO_PIN));
  ESP_ERROR_CHECK(gpio_set_intr_type(MOTION_SW_GPIO_PIN, GPIO_INTR_NEGEDGE));
  ESP_ERROR_CHECK(gpio_isr_handler_add(MOTION_SW_GPIO_PIN, motion_sensor_isr_handler, NULL));
  ESP_ERROR_CHECK(gpio_new_pin_glitch_filter(&glitch_cfg_motion, &glitch_handle_motion));
  ESP_ERROR_CHECK(gpio_glitch_filter_enable(glitch_handle_motion));
  motion_en = config_read_int("motion_en") != 0;
  NEO_LOGD(TAG, "motion_en = %d", motion_en);
}
