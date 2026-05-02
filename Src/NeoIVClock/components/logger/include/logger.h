#ifndef NEO_IV_CLOCK_LOGGER_H
#define NEO_IV_CLOCK_LOGGER_H

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE

#include "esp_log.h"

void logger_init(void);

#define NEO_LOGI(TAG, fmt, ...) ESP_LOGI(TAG, fmt, ##__VA_ARGS__)
#define NEO_LOGE(TAG, fmt, ...) ESP_LOGE(TAG, fmt, ##__VA_ARGS__)
#define NEO_LOGW(TAG, fmt, ...) ESP_LOGW(TAG, fmt, ##__VA_ARGS__)
#define NEO_LOGD(TAG, fmt, ...) ESP_LOGD(TAG, fmt, ##__VA_ARGS__)
#define NEO_LOGV(TAG, fmt, ...) ESP_LOGV(TAG, fmt, ##__VA_ARGS__)

#define NEO_LOGI_HEX(TAG, buffer, size) ESP_LOG_BUFFER_HEXDUMP(TAG, buffer, size, ESP_LOG_INFO)
#define NEO_LOGE_HEX(TAG, buffer, size) ESP_LOG_BUFFER_HEXDUMP(TAG, buffer, size, ESP_LOG_ERROR)
#define NEO_LOGW_HEX(TAG, buffer, size) ESP_LOG_BUFFER_HEXDUMP(TAG, buffer, size, ESP_LOG_WARN)
#define NEO_LOGD_HEX(TAG, buffer, size) ESP_LOG_BUFFER_HEXDUMP(TAG, buffer, size, ESP_LOG_DEBUG)
#define NEO_LOGV_HEX(TAG, buffer, size) ESP_LOG_BUFFER_HEXDUMP(TAG, buffer, size, ESP_LOG_VERBOSE)

#endif // NEO_IV_CLOCK_LOGGER_H
