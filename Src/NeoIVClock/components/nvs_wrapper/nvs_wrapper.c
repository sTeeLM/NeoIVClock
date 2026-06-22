#include "nvs_wrapper.h"
#include "nvs_flash.h"
#include "logger.h"

static const char *TAG = "NVS_WRAPPER";

void nvs_wrapper_init(void)
{
  esp_err_t ret = ESP_OK;

  NEO_LOGI(TAG, "init");

  ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    NEO_LOGW(TAG, "nvs flash init failed, try erase and init again");
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);
  NEO_LOGI(TAG, "nvs flash init done");
}
