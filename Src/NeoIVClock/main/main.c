#include "alarm.h"
#include "beeper.h"
#include "ec11.h"
#include "clock.h"
#include "config.h"
#include "delay.h"
#include "iv18.h"
#include "bmp280.h"
#include "dpf_player.h"
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
#include "timer.h"
#include "oled.h"
#include "usart_wrapper.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_cpu.h" 

void app_main(void)
{
  uint8_t j = 0;

  // 核心组件的初始化
  logger_init();
  delay_init();

  NEO_LOGI("MAIN", "Starting up...on CPU%u", esp_cpu_get_core_id());
  
  // 初始化硬件接口
  gpio_wrapper_init();
  usart_wrapper_init();
  i2c_wrapper_init();

  // 初始化底层硬件，注意顺序，ec11_init要在最前面
  ec11_init();
  rom_init();
  config_init(); // 这个是软件，由于后面的初始化可能会读配置，所以放在底层硬件初始化之后  
  iv18_init();
  beeper_init();
  ds3231_rtc_init();
  motion_sensor_init();
  bmp280_init();
  dpf_player_init();
  oled_init();

  // 初始化其他软件设备
  clock_init();
  alarm_init();
  timer_init();
  terminal_init();
  player_init();
  task_init();
  sm_init();
 
  beeper_beep();

  delay_ms(2000);

  beeper_beep_beep();

  dpf_player_play_dir_file(4, 1);

  //uint8_t data[6] = {0xC4,0x74,0x2F,0x7C,0x15,0x04};

  //oled_draw_bitmap(0, 10, 6, 8, data, OLED_DRAW_OVERWRITE);

  
  oled_draw_char_6X8(0, 0, '{', false, OLED_DRAW_OVERWRITE);
  oled_draw_char_6X8(0, 8, '|', false, OLED_DRAW_OVERWRITE);
  oled_draw_char_6X8(0, 16, '}', false, OLED_DRAW_OVERWRITE);
  oled_draw_char_6X8(0, 24, '~', false, OLED_DRAW_OVERWRITE);

  // 跑事件循环
  while(1) {
    delay_ms(1000);
    iv18_set_brightness((j++) % 101); // 0~100循环调整亮度
    clock_dump();
    bmp280_read_data(NULL);
  }


}