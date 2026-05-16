#include <stdio.h>
#include "logger.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_cpu.h" 

static const char* TAG = "LOGGER";

static // 自定义打印格式
int logger_custom_log(const char *fmt, va_list args) {
    printf("[%d] ", esp_cpu_get_core_id()); // 插入核心ID
    return vprintf(fmt, args);
}

void logger_init(void)
{
    NEO_LOGI(TAG, "init");
    esp_log_set_vprintf(logger_custom_log);
}
