#include "gpio_wrapper.h"
#include "hal/gpio_types.h"
#include "logger.h"
#include "driver/gpio.h"

static const char * TAG = "GPIO";

void gpio_wrapper_init(void)
{
  gpio_config_t io_iv18_conf = {};
  gpio_config_t io_i2c_conf = {};
  gpio_config_t io_ds3231_conf = {};
  gpio_config_t io_ec11_conf = {};
  gpio_config_t io_beeper_conf = {};
  gpio_config_t io_motion_conf = {};
  gpio_config_t io_dpf_player_conf = {};
  gpio_config_t io_pm5003st_conf = {};

  NEO_LOGI(TAG, "init");
  //NEO_LOGI(TAG, "before gpio_dump_io_configuration:\n");
  //gpio_dump_io_configuration(stdout, SOC_GPIO_VALID_GPIO_MASK);
  esp_intr_dump(NULL);

  // 设置iv18相关GPIO
  //disable interrupt
  io_iv18_conf.intr_type = GPIO_INTR_DISABLE;
  //set as output mode
  io_iv18_conf.mode = GPIO_MODE_OUTPUT;
  //bit mask of the pins that you want to set,e.g.GPIO18/19
  io_iv18_conf.pin_bit_mask = 
    (1ULL << IV18_EN_GPIO_PIN)  | 
    (1ULL << IV18_CLK_GPIO_PIN) |
    (1ULL << IV18_DIN_GPIO_PIN) |
    (1ULL << IV18_LOAD_GPIO_PIN) |
    (1ULL << IV18_BLANK_GPIO_PIN);
  //disable pull-down mode
  io_iv18_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  //disable pull-up mode
  io_iv18_conf.pull_up_en = GPIO_PULLUP_DISABLE;
  //configure GPIO with the given settings
  ESP_ERROR_CHECK(gpio_config(&io_iv18_conf));

 
  // 设置i2c相关GPIO
  io_i2c_conf.intr_type = GPIO_INTR_DISABLE;
  io_i2c_conf.mode = GPIO_MODE_INPUT_OUTPUT_OD;
  io_i2c_conf.pin_bit_mask = 
    (1ULL << I2C_SCL_GPIO_PIN) |
    (1ULL << I2C_SDA_GPIO_PIN);
  io_i2c_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_i2c_conf.pull_up_en = GPIO_PULLDOWN_DISABLE;
  ESP_ERROR_CHECK(gpio_config(&io_i2c_conf));
 
  
  // 设置ds3231相关GPIO
  io_ds3231_conf.intr_type = GPIO_INTR_DISABLE;
  io_ds3231_conf.mode = GPIO_MODE_INPUT;
  io_ds3231_conf.pin_bit_mask = 
    (1ULL << DS3231_CLK_GPIO_PIN);
  io_ds3231_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_ds3231_conf.pull_up_en = GPIO_PULLDOWN_DISABLE;
  ESP_ERROR_CHECK(gpio_config(&io_ds3231_conf));

  // 设置EC11相关GPIO
  io_ec11_conf.intr_type = GPIO_INTR_DISABLE;
  io_ec11_conf.mode = GPIO_MODE_INPUT;
  io_ec11_conf.pin_bit_mask = 
    (1ULL << EC11_B_GPIO_PIN) |
    (1ULL << EC11_C_GPIO_PIN) |
    (1ULL << EC11_A_GPIO_PIN);
  io_ec11_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_ec11_conf.pull_up_en = GPIO_PULLUP_ENABLE;
  ESP_ERROR_CHECK(gpio_config(&io_ec11_conf));

  // 设置Beeper相关GPIO
  io_beeper_conf.mode = GPIO_MODE_OUTPUT;
  io_beeper_conf.intr_type = GPIO_INTR_DISABLE; 
  io_beeper_conf.pin_bit_mask = (1ULL << BEEPER_GPIO_PIN);
  io_beeper_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_beeper_conf.pull_up_en = GPIO_PULLUP_DISABLE;
  ESP_ERROR_CHECK(gpio_config(&io_beeper_conf));

  // 设置运动传感器开关相关GPIO
  io_motion_conf.mode = GPIO_MODE_INPUT;
  io_motion_conf.intr_type = GPIO_INTR_DISABLE;
  io_motion_conf.pin_bit_mask = (1ULL << MOTION_SW_GPIO_PIN);
  io_motion_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_motion_conf.pull_up_en = GPIO_PULLUP_ENABLE;
  ESP_ERROR_CHECK(gpio_config(&io_motion_conf));

  // 设置DPF Player相关GPIO
  io_dpf_player_conf.mode = GPIO_MODE_INPUT;
  io_dpf_player_conf.intr_type = GPIO_INTR_DISABLE;
  io_dpf_player_conf.pin_bit_mask = (1ULL << DPF_PLAYER_BSY_GPIO_PIN);
  io_dpf_player_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_dpf_player_conf.pull_up_en = GPIO_PULLUP_ENABLE;
  ESP_ERROR_CHECK(gpio_config(&io_dpf_player_conf));

  // 设置DPM5003ST相关GPIO定义
  io_pm5003st_conf.mode = GPIO_MODE_OUTPUT;
  io_pm5003st_conf.intr_type = GPIO_INTR_DISABLE;
  io_pm5003st_conf.pin_bit_mask = (1ULL << PM5003ST_SET_GPIO_PIN) | (1ULL << PM5003ST_RESET_GPIO_PIN);
  io_pm5003st_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_pm5003st_conf.pull_up_en = GPIO_PULLUP_DISABLE;
  ESP_ERROR_CHECK(gpio_config(&io_pm5003st_conf));

  gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1);

  //NEO_LOGI(TAG, "after gpio_dump_io_configuration:\n");
  //gpio_dump_io_configuration(stdout, SOC_GPIO_VALID_GPIO_MASK);

}


void gpio_wrapper_set_level(uint32_t gpio_num, uint8_t level)
{
  ESP_ERROR_CHECK(gpio_set_level((gpio_num_t)gpio_num, level));
}

uint8_t gpio_wrapper_get_level(uint32_t gpio_num)
{
  uint8_t ret = 0;
  int val = gpio_get_level((gpio_num_t)gpio_num);
  if(val < 0) {
    NEO_EARLY_LOGE(TAG, "gpio_get_level failed for gpio %u", gpio_num);
  } else {
    ret = (uint8_t)val;
  }
  return ret;
}