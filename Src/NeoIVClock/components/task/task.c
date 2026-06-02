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
#include <stdatomic.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#define TASK_IPC_QUEUE_SIZE 5
#define TASK_IPC_TIMEO_MS   10
static QueueHandle_t task_ipc_msg_queue[2];

static const char * TAG = "TASK";
/*
  EV_250MS = 0,          // 大约每250ms转一下
  EV_1S,                 // 大约每1s转一下  
  EV_10S,                // 大约每10s转一下  
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
  EV_SENSOR_STAGE0,      // Sensor进入stage0: pms关闭，其他传感器更新数据
  EV_SENSOR_STAGE1,      // Sensor进入stage1: 打开pms开始预热但是不更新数据，其他传感器更新数据
  EV_SENSOR_STAGE2,      // Sensor进入stage2: 所有传感器更新数据
  EV_NM_CONFIG_BEGIN,    // Network Manager启动了配置服务器
  EV_NM_CONFIG_END,      // Network Manager完成了配置
  EV_NM_TIME_SYNC,       // Network Manager做了网络时间同步
  EV_V1,                 // 虚拟事件1
  EV_V2,                 // 虚拟事件2
  EV_V3,                 // 虚拟事件3
  EV_V4,                 // 虚拟事件4
  EV_V5,                 // 虚拟事件5
  EV_V6,                 // 虚拟事件6 
  EV_V7,                 // 虚拟事件7
  EV_V8,                 // 虚拟事件8
  EV_V9,                 // 虚拟事件9
*/
const char * task_names[] =
{
  "EV_250MS",
  "EV_1S",
  "EV_10S",
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
  "EV_SENSOR_STAGE0",
  "EV_SENSOR_STAGE1",
  "EV_SENSOR_STAGE2",
  "EV_NM_CONFIG_BEGIN",
  "EV_NM_CONFIG_END",
  "EV_NM_TIME_SYNC",
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
  null_proc,     // EV_250MS
  task_1s_proc,  // EV_1S
  null_proc,     // EV_10S
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
  sensor_data_proc, // EV_SENSOR_STAGE0
  sensor_data_proc, // EV_SENSOR_STAGE1
  sensor_data_proc, // EV_SENSOR_STAGE2
  null_proc, // EV_NM_CONFIG_BEGIN
  null_proc, // EV_NM_CONFIG_END
  clock_time_sync_proc, // EV_NM_TIME_SYNC
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

#define  TASK_MAX_EVENT_CNT 64

static atomic_uint_least64_t ev_bits[2];
static volatile uint8_t  ev_args[2][TASK_MAX_EVENT_CNT];


// 原子 Test and Clear：将指定位置 0，并返回其原本的状态（1->true, 0->false）
static bool task_bitmask_test_and_clear(atomic_uint_least64_t *bitmap, int bit_index) 
{
    uint64_t mask = ~(1ULL << bit_index);
    // atomic_fetch_and 将目标位置 0，并返回修改前的旧值
    uint64_t old_val = atomic_fetch_and(bitmap, mask);
    // 检查旧值中该位是否为 1
    return (old_val & (1ULL << bit_index)) != 0;
}

// 原子 Set：将指定位置 1（无条件覆盖）
static void task_bitmask_set(atomic_uint_least64_t *bitmap, int bit_index) 
{
    uint64_t mask = (1ULL << bit_index);
    // 使用按位或操作置 1
    atomic_fetch_or(bitmap, mask);
}

typedef struct _task_ipc_t
{
  task_event_t ev;
  uint8_t      arg;
} task_ipc_t;

void task_init (void)
{
  //memset(ev_bits, 0, sizeof(ev_bits));
  atomic_store(&ev_bits[0], 0);
  atomic_store(&ev_bits[1], 0);
  memset(ev_args, 0, sizeof(ev_args));  

  task_ipc_msg_queue[0] = xQueueCreate(TASK_IPC_QUEUE_SIZE, sizeof(task_ipc_t));
  task_ipc_msg_queue[1] = xQueueCreate(TASK_IPC_QUEUE_SIZE, sizeof(task_ipc_t));
}

static task_ipc_t ev_ipc[2];

void task_set_ipc_arg(task_event_t ev, uint8_t arg)
{
  uint32_t cpuid = esp_cpu_get_core_id();
  ev_ipc[cpuid].ev = ev;
  ev_ipc[cpuid].arg = arg;

  NEO_LOGD(TAG, "task_set_ipc %s", task_names[ev]);

  if(xQueueSend(task_ipc_msg_queue[cpuid], (void *) &ev_ipc[cpuid], pdMS_TO_TICKS(TASK_IPC_TIMEO_MS)) != pdPASS) {
    NEO_LOGW(TAG, "xQueueSend failed");
  }
}

void task_set_ipc(task_event_t ev)
{
  task_set_ipc_arg(ev, 0);
}

void task_rcv_ipc(void)
{
  uint32_t cpuid = esp_cpu_get_core_id();
  task_ipc_t ipc;
  cpuid = (cpuid + 1) % 2;
  if(xQueueReceive(task_ipc_msg_queue[cpuid], &ipc, pdMS_TO_TICKS(1)) == pdTRUE) {
    NEO_LOGD(TAG, "task_rcv_ipc %s %u", task_names[ipc.ev], ipc.arg);
    task_set_ev_arg(ipc.ev, ipc.arg);
  }
}

void task_run(void)
{
  uint8_t c;
  for(c = 0; c < EV_CNT; c++) {
    if(task_test_clr(c)) {
      task_procs[c](c);
    }
  }
}

void task_set(task_event_t ev)
{
  task_set_ev_arg(ev, 0);
}


void task_set_cpu(uint32_t cpu_id, task_event_t ev)
{
  task_bitmask_set(&ev_bits[cpu_id], ev);
  //ev_bits[cpu_id] |= 1 << ev;
}

bool task_test_clr(task_event_t ev)
{
  //ev_bits[esp_cpu_get_core_id()] &= ~(1 << ev);
  return task_bitmask_test_and_clear(&ev_bits[esp_cpu_get_core_id()], ev);
}

uint8_t task_get_arg(task_event_t ev)
{
  return ev_args[esp_cpu_get_core_id()][ev];
}

void task_set_ev_arg(task_event_t ev, uint8_t arg)
{
    //ev_bits[esp_cpu_get_core_id()] |= 1 << ev;
    task_bitmask_set(&ev_bits[esp_cpu_get_core_id()], ev);
    ev_args[esp_cpu_get_core_id()][ev] = arg;
}