#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "delay.h"
#include  "logger.h"



static const char * TAG = "DELAY";

void delay_init(void)
{
  NEO_LOGI(TAG, "init");
}

void delay_ms(uint32_t ms)
{
  vTaskDelay(ms / portTICK_PERIOD_MS);
}

void delay_us(uint32_t us)
{
  esp_rom_delay_us(us);
}

