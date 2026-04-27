#ifndef NEO_IV_CLOCK_GPIO_WRAPPER_H
#define NEO_IV_CLOCK_GPIO_WRAPPER_H

#include <stdint.h>

void gpio_wrapper_init(void);

// IV18相关GPIO定义
#define IV18_EN_GPIO_PIN     6
#define IV18_CLK_GPIO_PIN    7
#define IV18_DIN_GPIO_PIN    8
#define IV18_LOAD_GPIO_PIN   9
#define IV18_BLANK_GPIO_PIN  10

// GPIO操作接口 
void gpio_wrapper_set_level(uint32_t gpio_num, uint32_t level);

#endif //NEO_IV_CLOCK_GPIO_WRAPPER_H
