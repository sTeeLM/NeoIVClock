#include "alarm.h"
#include "beeper.h"
#include "button.h"
#include "clock.h"
#include "config.h"
#include "delay.h"
#include "iv18.h"
#include "ds3231_rtc.h"
#include "gpio_wrapper.h"
#include "i2c_wrapper.h"
#include "logger.h"
#include "motion_sensor.h"
#include "player.h"
#include "rom.h"
#include "sm.h"
#include "task.h"
#include "terminal.h"
#include "thermometer.h"
#include "timer.h"
#include "usart_wrapper.h"

void app_main(void)
{
  uint8_t i = 0, j = 0;


  // 核心组件的初始化
  logger_init();
  delay_init();

  NEO_LOGI("MAIN", "Starting up...");
  
  // 初始化硬件接口
  gpio_wrapper_init();
  usart_wrapper_init();
  i2c_wrapper_init();

  // 初始化底层硬件
  iv18_init();
  button_init();
  beeper_init();
  rom_init();
  ds3231_rtc_init();
  motion_sensor_init();
  player_init();
  thermometer_init();

  // 初始化软件设备
  //config_init();
  clock_init();
  alarm_init();
  timer_init();
  terminal_init();
  task_init();
  sm_init();
 
  iv18_clr();

  iv18_set_dig(2, 'H');
  iv18_set_dig(3, 'E'); 
  iv18_set_dig(4, 'L'); 
  iv18_set_dig(5, 'L'); 
  iv18_set_dig(6, 'O'); 

  // 跑事件循环
  while(1) {
    iv18_scan();
    delay_us(2000);
    i++;
    if(i % 100000 == 0) {
      j ++;
      iv18_set_brightness(j % 101); // 0~100循环调整亮度
    }
  }
}