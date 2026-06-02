#ifndef NEW_IV_CLOCK_TASK_H
#define NEW_IV_CLOCK_TASK_H

#include <stdint.h>

#include "esp_cpu.h"

// max 64
typedef enum _task_event_t
{           
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
  EV_CNT  
} task_event_t;

extern const char * task_names[];

typedef void (*TASK_PROC)(task_event_t);

void task_init (void);

// 注意这个函数是没有锁的，只有在初始化阶段第二个CPU事件循环还没有运行的时候可以调用
void task_set_cpu(uint32_t cpu_id, task_event_t ev);

void task_set(task_event_t ev);
bool task_test_clr(task_event_t ev);
uint8_t task_get_arg(task_event_t ev);
void task_set_ev_arg(task_event_t ev, uint8_t arg);
void task_run(void);

// 跨core发送消息，注意不能在中断上下文中调用
void task_set_ipc(task_event_t ev);
void task_set_ipc_arg(task_event_t ev, uint8_t arg);
void task_rcv_ipc(void);

#endif  // NEW_IV_CLOCK_TASK_H
