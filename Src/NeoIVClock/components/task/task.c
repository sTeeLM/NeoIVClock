#include "task.h"
#include "logger.h"
#include "sm.h"
#include "ec11.h"
#include "clock.h"
#include "iv18.h"
#include "alarm.h"
#include "player.h"
#include "sensor_data.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#define TASK_IPC_QUEUE_SIZE 5
#define TASK_IPC_TIMEO_MS   10
static QueueHandle_t task_ipc_msg_queue[2];

static const char * TAG = "TASK";
/*
  EV_250MS = 0,          // 大约每250ms转一下
  EV_1S,                 // 大约每1s转一下  
  EV_EC11_SCAN,          // 扫描EC11 
  EV_EC11_C,             // 顺时针旋转
  EV_EC11_CC,            // 逆时针旋转
  EV_EC11_FAST_C,        // 顺时针快速旋转
  EV_EC11_FAST_CC,       // 逆时针快速旋转
  EV_EC11_DOWN,          // 按下
  EV_EC11_PRESS,         // 按下并抬起
  EV_EC11_LPRESS,        // 长按
  EV_EC11_UP,            // 抬起  
  EV_ACC,                // 有晃动
  EV_TIMER,              // timer 倒计时结束
  EV_ALARM0,             // Alarm0响起
  EV_ALARM1,             // Alarm1响起
  EV_PLAYER_STOP,        // 播放器停止
  EV_CAL_RTC,            // 校准RTC
  EV_SENSOR_UPDATE,      // Sensor数据有更新
  EV_SENSOR_STAGE1,      // Sensor进入stage1: 打开pms开始预热但是不更新数据，其他传感器更新数据
  EV_SENSOR_STAGE2,      // Sensor进入stage3: 所有传感器更新数据
  EV_SENSOR_REPORT,      // Sensor上报数据，并且进入stage0: 关闭pms
  EV_V1,                 // 虚拟事件1
  EV_V2,                 // 虚拟事件2
  EV_V3,                 // 虚拟事件3
  EV_V4,                 // 虚拟事件4
  EV_V5,                 // 虚拟事件5
  EV_V6,                 // 虚拟事件6 
  EV_V7,                 // 虚拟事件7
  EV_V8,                 // 虚拟事件8
  EV_V9,                 // 虚拟事件9
  EV_CNT  
*/
const char * task_names[] =
{
  "EV_250MS",
  "EV_1S",
  "EV_EC11_SCAN",
  "EV_EC11_C",
  "EV_EC11_CC",
  "EV_EC11_FAST_C",
  "EV_EC11_FAST_CC",
  "EV_EC11_DOWN",
  "EV_EC11_PRESS",
  "EV_EC11_LPRESS",
  "EV_EC11_UP",  
  "EV_ACC",
  "EV_TIMER",
  "EV_ALARM0",
  "EV_ALARM1", 
  "EV_PLAYER_STOP",
  "EV_CAL_RTC",
  "EV_SENSOR_UPDATE",
  "EV_SENSOR_STAGE1",
  "EV_SENSOR_STAGE2",
  "EV_SENSOR_REPORT",
  "EV_V1",
  "EV_V2",
  "EV_V3",
  "EV_V4",
  "EV_V5",
  "EV_V6",
  "EV_V7",
  "EV_V8",
  "EV_V9",
};

static void null_proc(task_event_t ev)
{
  sm_run(ev);
}

// 这里放置不在状态机器中的，但是需要定时执行的逻辑
static void task_1s_proc(task_event_t ev)
{
  // 只在cpu1上运行，注意EV_250MS/EV_1S在两个core上都会产生！
  if(esp_cpu_get_core_id() == SM_APP_CORE_ID)
    iv18_proc(ev);

  sm_run(ev);
}

static const TASK_PROC task_procs[EV_CNT] = 
{
  null_proc, // EV_250MS
  task_1s_proc, // EV_1S
  ec11_scan_proc, // EV_EC11_SCAN
  ec11_key_proc, // EV_EC11_C
  ec11_key_proc, // EV_EC11_CC
  ec11_key_proc, // EV_EC11_FAST_C
  ec11_key_proc, // EV_EC11_FAST_CC
  ec11_key_proc, // EV_EC11_DOWN
  ec11_key_proc, // EV_EC11_PRESS
  ec11_key_proc, // EV_EC11_LPRESS
  ec11_key_proc, // EV_EC11_UP  
  null_proc, // EV_ACC
  null_proc, // EV_TIMER
  alarm_proc, // EV_ALARM0
  alarm_proc, // EV_ALARM1
  player_proc, // EV_PLAYER_STOP
  clock_recal_rtc_proc,           // EV_CAL_RTC
  null_proc, // EV_SENSOR_UPDATE
  sensor_data_proc, // EV_SENSOR_STAGE1
  sensor_data_proc, // EV_SENSOR_STAGE2
  sensor_data_proc, // EV_SENSOR_REPORT
  null_proc, // EV_V1
  null_proc, // EV_V2
  null_proc, // EV_V3
  null_proc, // EV_V4
  null_proc, // EV_V5
  null_proc, // EV_V6
  null_proc, // EV_V7
  null_proc, // EV_V8
  null_proc, // EV_V9
};


static uint32_t ev_bits[2];
static uint8_t  ev_args[2][32];

void task_init (void)
{
  memset(ev_bits, 0, sizeof(ev_bits));
  memset(ev_args, 0, sizeof(ev_args));  

  task_ipc_msg_queue[0] = xQueueCreate(TASK_IPC_QUEUE_SIZE, sizeof(task_event_t));
  task_ipc_msg_queue[1] = xQueueCreate(TASK_IPC_QUEUE_SIZE, sizeof(task_event_t));
}

static task_event_t ev_ipc[2];

void task_set_ipc(task_event_t ev)
{
  uint32_t cpuid = esp_cpu_get_core_id();
  ev_ipc[cpuid] = ev;

  NEO_LOGD(TAG, "task_set_ipc %s", task_names[ev]);

  if(xQueueSend(task_ipc_msg_queue[cpuid], (void *) &ev_ipc[cpuid], pdMS_TO_TICKS(TASK_IPC_TIMEO_MS)) != pdPASS) {
    NEO_LOGW(TAG, "xQueueSend failed");
  }
}

void task_rcv_ipc(void)
{
  uint32_t cpuid = esp_cpu_get_core_id();
  task_event_t ev;
  cpuid = (cpuid + 1) % 2;
  if(xQueueReceive(task_ipc_msg_queue[cpuid], &ev, pdMS_TO_TICKS(1)) == pdTRUE) {
    NEO_LOGD(TAG, "task_rcv_ipc %s", task_names[ev]);
    task_set(ev);
  }
}

void task_run(void)
{
  uint8_t c;
  for(c = 0; c < EV_CNT; c++) {
    if(task_test(c)) {
      task_clr(c);
      task_procs[c](c);
    }
  }
}

void task_dump(void)
{
  uint8_t i;
  for (i = 0 ; i < EV_CNT; i ++) {
    NEO_LOGD(TAG, "[%02bd][%s] %c\n", i, task_names[i], task_test(i) ? '1' : '0');
  }
}

void task_set(task_event_t ev)
{
  ev_bits[esp_cpu_get_core_id()] |= 1 << ev;
}


void task_set_cpu(uint32_t cpu_id, task_event_t ev)
{
  ev_bits[cpu_id] |= 1 << ev;
}

void task_clr(task_event_t ev)
{
  ev_bits[esp_cpu_get_core_id()] &= ~(1 << ev);
}

bool task_test(task_event_t ev)
{
  return (ev_bits[esp_cpu_get_core_id()] & (1 << ev)) != 0;
}

uint8_t task_get_arg(task_event_t ev)
{
  return ev_args[esp_cpu_get_core_id()][ev];
}

void task_set_ev_arg(task_event_t ev, uint8_t arg)
{
    ev_bits[esp_cpu_get_core_id()] |= 1 << ev;
    ev_args[esp_cpu_get_core_id()][ev] = arg;
}