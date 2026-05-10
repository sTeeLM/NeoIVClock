#ifndef NEO_IV_CLOCK_GPIO_WRAPPER_H
#define NEO_IV_CLOCK_GPIO_WRAPPER_H

#include <stdint.h>

void gpio_wrapper_init(void);

// IV18相关GPIO定义
#define IV18_DIN_GPIO_PIN    38
#define IV18_LOAD_GPIO_PIN   39
#define IV18_CLK_GPIO_PIN    40
#define IV18_BLANK_GPIO_PIN  41
#define IV18_EN_GPIO_PIN     42


// I2C相关GPIO定义
#define I2C_SCL_GPIO_PIN     36 
#define I2C_SDA_GPIO_PIN     37

// DS3231的时钟信号GPIO定义
#define DS3231_CLK_GPIO_PIN    15

// EC11相关GPIO定义
#define EC11_A_GPIO_PIN       13
#define EC11_B_GPIO_PIN       12
#define EC11_C_GPIO_PIN       14

// Beeper 相关GPIO定义
#define BEEPER_GPIO_PIN       16

// 运动传感器开关相关GPIO定义
#define MOTION_SW_GPIO_PIN    17

// DPF Player相关GPIO定义
#define DPF_PLAYER_BSY_GPIO_PIN 18
#define DPF_PLAYER_RX_GPIO_PIN  19
#define DPF_PLAYER_TX_GPIO_PIN  20

// GPIO操作接口 
void gpio_wrapper_set_level(uint32_t gpio_num, uint8_t level);
uint8_t gpio_wrapper_get_level(uint32_t gpio_num);
#endif //NEO_IV_CLOCK_GPIO_WRAPPER_H
