#include "cjson_wrapper.h"
#include "cJSON.h"
#include "esp_heap_caps.h"
#include "logger.h"

static const char *TAG = "CJSON";

// 1. 自定义基于 PSRAM 的内存分配函数
void *cjson_wrapper_malloc_psram(size_t size) {
  // MALLOC_CAP_SPIRAM 标志强制在片外 PSRAM 中开辟空间
  return heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
}

// 2. 自定义基于 PSRAM 的内存释放函数
void cjson_wrapper_free_psram(void *ptr)
{ 
  heap_caps_free(ptr); 
}

void cjson_wrapper_init(void) {
  cJSON_Hooks hooks = {};

  NEO_LOGI(TAG, "init");
  // 3. 组装 cJSON 钩子
  hooks.malloc_fn = cjson_wrapper_malloc_psram;
  hooks.free_fn = cjson_wrapper_free_psram;

  // 4. 全局注副钩子（整机生效）
  cJSON_InitHooks(&hooks);

  NEO_LOGI(TAG, "internal heap size: %d bytes", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
  NEO_LOGI(TAG, "     spi heap size: %d bytes", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));  
}
