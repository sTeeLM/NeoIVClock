#include "aux_main.h"
#include "logger.h"
#include "task.h"
#include "freertos/idf_additions.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_cpu.h" 
#include "driver/gptimer.h"
#include "sm.h"

static const char * TAG = "AUX_MAIN";

static TaskHandle_t task_handle;

static void aux_main(void * param);

static uint32_t aux_main_ticks;

void aux_main_start(void)
{
  if(xTaskCreatePinnedToCore(
    aux_main,          // 任务函数
    "aux_main",        // 任务名称
    4000,               // 堆栈大小
    NULL,               // 参数
    1,                  // 优先级
    &task_handle,       // 任务句柄
    SM_AUX_CORE_ID    // 绑定到另一个核心
    ) != pdPASS) {
    NEO_LOGE(TAG, "xTaskCreatePinnedToCore failed");
    abort();
  }
}

static bool IRAM_ATTR aux_main_clock_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx)
{
    // 处理事件回调的一般流程：
    // 1. 从 user_ctx 中拿到用户上下文数据（需事先从 gptimer_register_event_callbacks 中传入）
    // 2. 从 edata 中获取警报事件数据，比如 edata->count_value
    // 3. 执行用户自定义操作
    // 4. 返回上述操作期间是否有高优先级的任务被唤醒了，以便通知调度器做切换任务

    aux_main_ticks ++;

    if(aux_main_ticks % 25 == 0) {
      task_set(EV_250MS);
      if(aux_main_ticks % 100 == 0) {
        task_set(EV_1S);
      }
      if(aux_main_ticks % 1000 == 0) {
        task_set(EV_10S);
      }
    }

    return false;
}

void aux_main(void * param)
{
  
  // 设置时钟中断
  gptimer_handle_t gptimer = NULL;
  gptimer_config_t timer_config = {
      .clk_src = GPTIMER_CLK_SRC_DEFAULT, // 选择默认的时钟源
      .direction = GPTIMER_COUNT_UP,      // 计数方向为向上计数
      .resolution_hz = 1 * 1000 * 1000,   // 分辨率为 1 MHz，即 1 次滴答为 1 微秒
  };
  gptimer_alarm_config_t alarm_config = {
    .reload_count = 0,      // 当警报事件发生时，定时器会自动重载到 0
    .alarm_count = 10000,     // 设置实际的警报周期，因为分辨率是 10ms
    .flags.auto_reload_on_alarm = true, // 使能自动重载功能
  };

  // 创建定时器实例
  gptimer_event_callbacks_t cbs = {
    .on_alarm = aux_main_clock_cb, // 当警报事件发生时，调用用户回调函数
  };

  aux_main_ticks = 0;

  NEO_LOGI(TAG, "aux_main run on CPU%d", esp_cpu_get_core_id());

  ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));
  // 设置定时器的警报动作
  ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer, &alarm_config));
  // 注册定时器事件回调函数，允许携带用户上下文
  ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer, &cbs, NULL));
  // 使能定时器
  ESP_ERROR_CHECK(gptimer_enable(gptimer));
  // 启动定时器
  ESP_ERROR_CHECK(gptimer_start(gptimer));

  NEO_LOGI(TAG, "will enter event loop on CPU%d", esp_cpu_get_core_id());

  while(1) {
    task_run();
    task_rcv_ipc();
  }
}