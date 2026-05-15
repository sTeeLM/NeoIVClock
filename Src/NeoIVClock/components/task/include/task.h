#ifndef NEW_IV_CLOCK_TASK_H
#define NEW_IV_CLOCK_TASK_H

#include <stdint.h>

#include "esp_cpu.h"

// max 32
typedef enum _task_event_t
{
  EV_NULL                = 0,            
  EV_250MS               = 1, // 大约每250ms转一下
  EV_1S                  = 2, // 大约每1s转一下  
  EV_EC11_SCAN           = 3, // 扫描EC11 
  EV_EC11_C              = 4, // 顺时针旋转
  EV_EC11_CC             = 5, // 逆时针旋转
  EV_EC11_FAST_C         = 6, // 顺时针快速旋转
  EV_EC11_FAST_CC        = 7, // 逆时针快速旋转
  EV_EC11_DOWN           = 8, // 按下
  EV_EC11_UP             = 9, // 抬起
  EV_EC11_PRESS          = 10, // 按下并抬起
  EV_EC11_LPRESS         = 11, // 长按
  EV_ACC                 = 12, // 有晃动
  EV_TIMER               = 13, // timer 倒计时结束
  EV_ALARM0              = 14, // Alarm0响起
  EV_ALARM1              = 15, // Alarm1响起
  EV_PLAYER_STOP         = 16,
  EV_CNT  
} task_event_t;

extern const char * task_names[];

typedef void (*TASK_PROC)(task_event_t);

void task_init (void);
void task_dump(void);




// 注意这个函数是没有锁的，只有在初始化阶段第二个CPU事件循环还没有运行的时候可以调用
void task_set_cpu(uint32_t cpu_id, task_event_t ev);

void task_set(task_event_t ev);
void task_clr(task_event_t ev);
bool task_test(task_event_t ev);
uint8_t task_get_arg(task_event_t ev);
void task_set_ev_arg(task_event_t ev, uint8_t arg);
void task_run(void);

#endif  // NEW_IV_CLOCK_TASK_H
