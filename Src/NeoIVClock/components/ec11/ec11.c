#include "ec11.h"
#include "logger.h"
#include "gpio_wrapper.h"
#include "esp_err.h"
#include "driver/gpio.h"
#include "driver/gpio_filter.h"

static const char * TAG = "EC11";

static void IRAM_ATTR ec11_isr_handler_a (void* param)
{
    if(gpio_wrapper_get_level(EC11_A_GPIO_PIN) == 0) {
        if (gpio_wrapper_get_level(EC11_B_GPIO_PIN) == 0) {
            NEO_EARLY_LOGD("EC11", "Rotated C!");
        } else {
            NEO_EARLY_LOGD("EC11", "Rotated CC!");
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

}

bool ec11_is_factory_reset(void) 
{
    // Check if the button is pressed for factory reset
    // This is a placeholder implementation
    // Replace with actual button press logic
    return false; // Return true if factory reset is needed
}