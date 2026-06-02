#include "ws2812b.h"
#include "logger.h"
#include "gpio_wrapper.h"
#include "led_strip.h" 
#include "delay.h"

static const char * TAG = "WS2812";

static led_strip_handle_t ws2812b_strip_handle; // 全局彩灯控制句柄

static uint8_t ws2812b_background_color_red;
static uint8_t ws2812b_background_color_green;
static uint8_t ws2812b_background_color_blue;

// 初始化 WS2812B 硬件外设
void ws2812b_init(void) 
{

  // 1. 配置 LED 灯带的物理属性
  led_strip_config_t strip_config = {
    .strip_gpio_num = LED_STRIP_GPIO_PIN,   // 控制引脚
    .max_leds = 1,                          // 灯珠总数
    .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB, // WS2812B 标准格式为 GRB
    .led_model = LED_MODEL_WS2812,          // 指定灯珠型号
    .flags.invert_out = false,              // 不反转信号电平
  };

  // 2. 配置 RMT 硬件渲染引擎属性
  led_strip_rmt_config_t rmt_config = {
    .clk_src = RMT_CLK_SRC_DEFAULT,        // 使用系统默认时钟
    .resolution_hz = 10 * 1000 * 1000,     // 10MHz 计数分辨率，精准匹配纳秒时序
    .flags.with_dma = false,               // 如果灯珠数量少于 100 个，不需要开 DMA
  };

  NEO_LOGI(TAG, "init");

  // 3. 调用官方 API 创建底层驱动实例
  ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &ws2812b_strip_handle));

  // 4. 清空灯带（默认开机全灭）
  led_strip_clear(ws2812b_strip_handle);

  ws2812b_background_color_red   = 0;
  ws2812b_background_color_green = 0;
  ws2812b_background_color_blue  = 0;
}

// 控制指定索引灯珠的 RGB 颜色（不会立刻亮起，需要调用 refresh）
void ws2812b_set_pixel(uint8_t red, uint8_t green, uint8_t blue) 
{
    led_strip_set_pixel(ws2812b_strip_handle, 0, red, green, blue);
}

// 刷新缓冲区，让颜色真正显示在物理灯珠上
void ws2812b_refresh(void) 
{
    led_strip_refresh(ws2812b_strip_handle);
}

// 清空所有灯珠（全黑灭灯）
void ws2812b_clear(void) 
{
    led_strip_clear(ws2812b_strip_handle);
}

void ws2812b_set_background(uint8_t red, uint8_t green, uint8_t blue)
{
    ws2812b_background_color_red = red;
    ws2812b_background_color_green = green;
    ws2812b_background_color_blue = blue;  
    led_strip_set_pixel(ws2812b_strip_handle, 0, red, green, blue);
    led_strip_refresh(ws2812b_strip_handle);
}

void ws2812b_blink(uint8_t red, uint8_t green, uint8_t blue)
{
    led_strip_set_pixel(ws2812b_strip_handle, 0, red, green, blue);
    led_strip_refresh(ws2812b_strip_handle);
    delay_ms(10);
    led_strip_set_pixel(ws2812b_strip_handle, 0, 
        ws2812b_background_color_red, 
        ws2812b_background_color_green, 
        ws2812b_background_color_blue);
    led_strip_refresh(ws2812b_strip_handle);
}


