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
#include "oled_ext.h"
#include "pms5003st.h"
#include "light_sensor.h"
#include "usart_wrapper.h"
#include "aux_main.h"
#include "sensor_data.h"
#include "reporter.h"
#include "oled_ext.h"
#include "mini_font.h"
#include "nm.h"
#include "aht20.h"
#include "ws2812b.h"
#include "ens160.h"
#include "esp32_temp.h"
#include "cjson_wrapper.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_cpu.h" 

#include <time.h>

static const char * TAG = "APP_MAIN";

void app_main(void)
{
  // 时区配置
  setenv("TZ", "CST-8", 1);
  tzset(); 

  // 核心组件的初始化
  logger_init();
  delay_init();

  NEO_LOGI(TAG, "Starting up...on CPU%u", esp_cpu_get_core_id());
  
  // 初始化硬件接口
  gpio_wrapper_init();
  usart_wrapper_init();
  i2c_wrapper_init();

  // 初始化底层硬件，注意顺序，ec11_init要在最前面
  ec11_init();
  rom_init();
  config_init(); // 这个是软件，由于后面的初始化可能会读配置，所以放在底层硬件初始化之后 
  mini_font_init(); // 字库可能被 OLED使用，所以放在这里初始化
  oled_init();  
  oled_ext_print_progress(0, L"启动..系统");
  ws2812b_init();
  esp32_temp_init();
  light_sensor_init();
  iv18_init();
  oled_ext_print_progress(10, L"启动..iv18");
  oled_ext_init();
  beeper_init();
  oled_ext_print_progress(15, L"启动..BEEPER");
  ds3231_rtc_init();
  oled_ext_print_progress(20, L"启动..DS3131");
  motion_sensor_init();
  oled_ext_print_progress(25, L"启动..震动传感器");
  bmp280_init();
  oled_ext_print_progress(30, L"启动..BM280");
  aht20_init();
  oled_ext_print_progress(35, L"启动..AHT20");
  dpf_player_init();
  oled_ext_print_progress(40, L"启动..DPF");
  pms5003st_init();
  oled_ext_print_progress(45, L"启动..PMS5003");
  ens160_init();
  oled_ext_print_progress(50, L"启动..ENS160");
  
  // 初始化其他软件设备
  cjson_wrapper_init(); 
  clock_init();
  oled_ext_print_progress(55, L"启动..时钟");
  alarm_init();
  timer_init();
  terminal_init();
  player_init();
  reporter_init();
  sensor_data_init();
  oled_ext_print_progress(60, L"启动..任务"); 
  nm_init();
  task_init();
  sm_init();
 
  beeper_beep();
  delay_ms(1000);
  beeper_beep_beep();

  // 在另一个core上启动事件循环
  aux_main_start();

  oled_ext_print_progress(80, L"启动..网络"); 
  nm_start_sta_daemon();

  oled_ext_print_progress(100, L"完成!"); 

  NEO_LOGI(TAG, "will enter event loop on CPU%d", esp_cpu_get_core_id());
  // 跑事件循环
  while(1) {
    task_run();
    task_rcv_ipc();
  }
}