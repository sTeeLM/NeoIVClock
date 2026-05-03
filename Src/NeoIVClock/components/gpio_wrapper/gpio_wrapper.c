#include "gpio_wrapper.h"
#include "logger.h"
#include "driver/gpio.h"

static const char * TAG = "GPIO";

void gpio_wrapper_init(void)
{
  gpio_config_t io_iv18_conf = {};
  gpio_config_t io_i2c_conf = {};

  NEO_LOGI(TAG, "init");
  //NEO_LOGI(TAG, "before gpio_dump_io_configuration:\n");
  //gpio_dump_io_configuration(stdout, SOC_GPIO_VALID_GPIO_MASK);

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
 

  //NEO_LOGI(TAG, "after gpio_dump_io_configuration:\n");
  //gpio_dump_io_configuration(stdout, SOC_GPIO_VALID_GPIO_MASK);

}


void gpio_wrapper_set_level(uint32_t gpio_num, uint32_t level)
{
  ESP_ERROR_CHECK(gpio_set_level((gpio_num_t)gpio_num, level));
}
