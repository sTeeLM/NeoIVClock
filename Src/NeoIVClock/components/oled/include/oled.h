#ifndef NEO_IV_CLOCK_OLED_H
#define NEO_IV_CLOCK_OLED_H

#include <stdint.h>
#include <stdbool.h>

void oled_init(void);

//////// 底层命令封装

///// 1. Fundamental Command
// 设置对比度：0~255，数值越大对比度越高
// 0x81
// A[7:0]
void oled_set_contrast(uint8_t contrast);

// Entire Display ON: true Output follows RAM content, false Output ignores RAM content
// A4/A5
void oled_set_entire_display_on(bool on);

// 设置反色显示: false 为正常，true 为反色
// A6/A7
void oled_set_inverse(bool inverse);

// Set Display ON/OFF: false Off，true On
// AE/AF
// 打开关闭显示
void oled_display_onoff(bool on);

///// 2. Scrolling Command

// Continuous Horizontal Scroll Setup
// 26/27
void oled_setup_h_scroll(bool left, uint8_t page_start, uint8_t page_end, uint8_t interval);

// Continuous Vertical and Horizontal Scroll Setup
// 29/2A
void oled_setup_v_scroll_and_h_scroll(bool v_and_left, uint8_t page_start, uint8_t page_end, uint8_t interval, uint8_t v_scroll_offset);

// Deactivate scroll
// 2E
void oled_deactivate_scroll(void);

// Activate scroll
// 2F
void oled_activate_scroll(void);

// Set Vertical Scroll Area
// A3
void oled_set_v_scroll_area(uint8_t nrow_fix, uint8_t nrow_scroll);


///// 3. Addressing Setting Command
// Set Memory Addressing Mode: 设置显示寻址模式
// 20
// A[1:0]
typedef enum _oled_addressing_mode_t {
    OLED_ADDRESSING_MODE_HORIZONTAL = 0x0,
    OLED_ADDRESSING_MODE_VERTICAL = 0x1,
    OLED_ADDRESSING_MODE_PAGE = 0x2,
} oled_addressing_mode_t;

void oled_set_addressing_mode(oled_addressing_mode_t mode);

// Set Higher Column Start Address for Page Addressing Mode设置列地址
// column_addr8位有效
// This command is only for page addressing mode
// 00~0F
// 10~1F
void oled_set_column_address_for_page_addressing(uint8_t column_addr);


// Set Page Start Address for Page Addressing Mode: 设置页地址
// page_addr只有末3位有效
// This command is only for page addressing mode
// B0~B7
void oled_set_page_address_for_page_addressing(uint8_t page_addr);

// Set Column Address: 设置列地址
// This command is only for horizontal or vertical addressing mode.
// column_addr_start, column_addr_end只有末7位有效
// 21
// A[6:0] B[6:0]
void oled_set_column_address(uint8_t column_addr_start, uint8_t column_addr_end);

// Set Page Address: 设置页地址
// This command is only for horizontal or vertical addressing mode.
// page_addr_start, page_addr_end只有末3位有效
// 22
// A[2:0] B[2:0]
void oled_set_page_address(uint8_t page_addr_start, uint8_t page_addr_end);


///// 4. Hardware Configuration (Panel resolution & layout related) Command
// Set Display Start Line: Set display RAM display start line register from 0-63
// 40~7F
void oled_set_display_start_line(uint8_t start_line);


// Set Segment Re-map: 设置Segment重映射, 0为正常，1为左右反转
// A0/A1
void oled_set_segment_remap(bool remap);

// set multiplex ratio：15~63
// A8
// A[5:0]
void oled_set_multiplex_ratio(uint8_t ratio);

// 设置列扫描方向：0从com0-comN，1反过来
// C0/C8
void oled_set_com_scan_direction(bool dir);

// Set Display Offset: Set vertical shift by COM from 0d~63d：0~63
// D3
// A[5:0]
void oled_set_display_offset(uint8_t offset);

// Set COM Pins Hardware Configuration
// DA
// A[5:4]
void oled_set_com_pin_hardware_configuration(bool alt_confg, bool enable_left_right_remap);


///// 5. Timing & Driving Scheme Setting Command
// Set Display Clock Divide Ratio/Oscillator Frequency
// div:  0~15
// freq: 0~15
///// DCLK = (freq + 1) / (div + 1)
// frame frequency of display = DCLK / (K * multiplex_ratio + 1)
// K is the number of display clocks per row
// K = Phase 1 period + Phase 2 period + BANK0 pulse width = 2 + 2 + 50 = 54 at power on reset
// D5
// A[7:0]
void oled_set_display_clock(uint8_t div, uint8_t freq);

// Set Pre-charge Period: 
// p1:  0~15
// p2:  0~15
// D9
// A[7:0]
void oled_set_precharge_period(uint8_t p1, uint8_t p2);

// Set VCOMH Deselect Level
// DB
// A[6:4]
typedef enum _oled_vcomh_deselect_level_t {
  OLED_VCOMH_DESELECT_LEVEL_065 = 0,
  OLED_VCOMH_DESELECT_LEVEL_077 = 0x2,
  OLED_VCOMH_DESELECT_LEVEL_083 = 0x3,
} oled_vcomh_deselect_level_t;
void oled_set_vcomh_deselect_level(oled_vcomh_deselect_level_t level);


///// 6. Advance Graphic Command
// Set Fade Out and Blinking
// interval: 0~15： 8 Frames ~ 128 Frames
// 23
// A[6:0]
typedef enum _oled_fade_blink_mode_t {
  OLED_FADE_BLINK_MODE_DISABLE = 0,
  OLED_FADE_BLINK_MODE_FADE  = 0x2,
  OLED_FADE_BLINK_MODE_BLINK = 0x3,
} oled_fade_blink_mode_t;
void oled_set_fade_blinking(oled_fade_blink_mode_t mode, uint8_t interval);


// Set Zoom In
// D6
// A[0]
void oled_set_zoom_in(bool enable);

///// 7. Charge Pump Command

// Charge Pump Setting
// 8D
// A[7:0]
void oled_set_charge_pump(bool enable);


//////// 绘图API
// 清屏
void oled_clear(void);

// 在oled上填充一个实心矩形，坐标(x,y)是矩形左上角的点，w是宽度，h是高度，
// color是否反色，true为白色，false为黑色
void oled_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, bool color);

typedef enum _oled_draw_type_t
{
  OLED_DRAW_OVERWRITE,
  OLED_DRAW_OR,
  OLED_DRAW_XOR
} oled_draw_type_t;

// 在oled上显示一个6*8的ASCII字符(使用自带字库)，坐标(x,y)是字符左上角的点，
// invert是否反色
// type 渲染类型
// 注：绘制字符始终是掩膜绘制，也就是0的位是透明的
void oled_draw_char_6X8(uint8_t x, uint8_t y, char c, bool invert, oled_draw_type_t type);

// 在oled上显示一个8*16的ASCII字符(使用自带字库)，坐标(x,y)是字符左上角的点，
// invert是颜色
// type 渲染类型
// 注：绘制字符始终是掩膜绘制，也就是0的位是透明的
void oled_draw_char_8X16(uint8_t x, uint8_t y, char c, bool invert, oled_draw_type_t type);

// 在oled上显示一个bitmap，坐标(x,y)是bitmap左上角的点，w是宽度，h是高度，
// type 渲染类型
// bitmap是一个字节数组，采用列行式存储：
// bitmap大小为(w * ((h + 7) & ~7) / 8)字节，即将h向上取整到8的倍数，然后乘以w，得到总字节数
// bitmap的每个字节表示8个像素，从左到右表示w个从上到下的竖立的8位扫描线，最后的w个扫描线不足8位的部分用0填充
// 可以用pctolcd工具将图片转换为这种格式的bitmap
void oled_draw_bitmap(int32_t x, int32_t y, uint16_t w, uint16_t h, const uint8_t *bitmap, oled_draw_type_t type);

#endif // NEO_IV_CLOCK_OLED_H