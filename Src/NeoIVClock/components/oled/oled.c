#include "oled.h"
#include "oled_font.h"
#include "logger.h"
#include "i2c_wrapper.h"
#include <string.h>

// SSD1306 OLED显示屏的驱动
// 128 x 64 Dot Matrix

static const char * TAG = "OLED";

#define OLED_I2C_ADDR 0x3C

static uint8_t oled_buffer[OLED_PAGES][OLED_WIDTH]; // 128 * 64 /8, 显存镜像，对应8个Page，每一个Page 128条扫描线

typedef struct _oled_dirty_t
{
    uint8_t begin;
    uint8_t length;
} oled_dirty_t;

static oled_dirty_t oled_buffer_dirty[OLED_PAGES]; // 每一个page的dirty标志， begin == length == 0 说明是clean的

static i2c_wrapper_dev_handle_t oled_dev_handle;

static void oled_send_command(uint8_t cmd)
{
    // Assume i2c_write(OLED_I2C_ADDR, 0x00, &cmd, 1);
    // NEO_LOGD(TAG, "[CMD] %02X", cmd);
    i2c_wrapper_write(&oled_dev_handle, 0x00, I2C_ADDR_MODE_8BIT, &cmd, 1);
}

static void oled_send_data(uint8_t data) 
{
    // NEO_LOGD(TAG, "[DAT] %02X", data);
    // Assume i2c_write(OLED_I2C_ADDR, 0x40, &data, 1);
    i2c_wrapper_write(&oled_dev_handle, 0x40, I2C_ADDR_MODE_8BIT, &data, 1);
}

static void oled_send_data_buffer(const uint8_t * addr, size_t size)
{
    i2c_wrapper_write(&oled_dev_handle, 0x40, I2C_ADDR_MODE_8BIT, addr, size);
}

///// 1. Fundamental Command
// 设置对比度：0~255，数值越大对比度越高
// 81
// A[7:0]
void oled_set_contrast(uint8_t contrast)
{
    oled_send_command(0x81);
    oled_send_command(contrast);
}

// Entire Display ON: true Output follows RAM content, false Output ignores RAM content
// A4/A5
void oled_set_entire_display_on(bool on)
{
    oled_send_command(on ? 0xA4 : 0xA5);
}

// 设置反色显示: false 为正常，true 为反色
// A6/A7
void oled_set_inverse(bool inverse)
{
    oled_send_command(inverse ? 0xA7 : 0xA6);
}

// Set Display ON/OFF: false Off，true On
// AE/AF
// 打开关闭显示
void oled_display_onoff(bool on)
{
    oled_send_command(on ? 0xAF : 0xAE);
}

///// 2. Scrolling Command
// Continuous Horizontal Scroll Setup
// 26/27
void oled_setup_h_scroll(bool left, uint8_t page_start, uint8_t page_end, uint8_t interval)
{
    oled_send_command(left ? 0x27 : 0x26);
    oled_send_command(0x00);
    oled_send_command(page_start & 0x7);
    oled_send_command(interval & 0x7);  
    oled_send_command(page_end & 0x7);  
    oled_send_command(0x00);
    oled_send_command(0xFF);      
}

// Continuous Vertical and Horizontal Scroll Setup
// 29/2A
void oled_setup_v_scroll_and_h_scroll(bool v_and_left, uint8_t page_start, uint8_t page_end, uint8_t interval, uint8_t v_scroll_offset)
{
    oled_send_command(v_and_left ? 0x2A : 0x29);
    oled_send_command(0x00);
    oled_send_command(page_start & 0x7);
    oled_send_command(interval & 0x7);  
    oled_send_command(page_end & 0x7);
    oled_send_command(v_scroll_offset & 0x3F);

}

// Deactivate scroll
// 2E
void oled_deactivate_scroll(void)
{
    oled_send_command(0x2E);
}

// Activate scroll
// 2F
void oled_activate_scroll(void)
{
    oled_send_command(0x2F);
}

// Set Vertical Scroll Area
// A3
void oled_set_v_scroll_area(uint8_t nrow_fix, uint8_t nrow_scroll)
{
    oled_send_command(0xA3);
    oled_send_command(nrow_fix & 0x3F);
    oled_send_command(nrow_scroll & 0x7F);
}

///// 3. Addressing Setting Command
// Set Memory Addressing Mode: 设置显示寻址模式
// 20
// A[1:0]
void oled_set_addressing_mode(oled_addressing_mode_t mode)
{
    oled_send_command(0x20);
    oled_send_command((uint8_t)mode);
}

// Set Higher Column Start Address for Page Addressing Mode设置列地址
// column_addr8位有效
// This command is only for page addressing mode
// 00~0F
// 10~1F
void oled_set_column_address_for_page_addressing(uint8_t column_addr)
{
    oled_send_command(column_addr & 0x0F);
    oled_send_command(((column_addr & 0xF0) >> 4 | 0x10));
}


// Set Page Start Address for Page Addressing Mode: 设置页地址
// page_addr只有末3位有效
// This command is only for page addressing mode
// B0~B7
void oled_set_page_address_for_page_addressing(uint8_t page_addr)
{
    oled_send_command(0xB0 | (page_addr & 0x7));
}

// Set Column Address: 设置列地址
// This command is only for horizontal or vertical addressing mode.
// column_addr_start, column_addr_end只有末7位有效
// 21
// A[6:0] B[6:0]
void oled_set_column_address(uint8_t column_addr_start, uint8_t column_addr_end)
{
    oled_send_command(0x21);
    oled_send_command(column_addr_start & 0x7);
    oled_send_command(column_addr_end & 0x7);
}

// Set Page Address: 设置页地址
// This command is only for horizontal or vertical addressing mode.
// page_addr_start, page_addr_end只有末3位有效
// 22
// A[2:0] B[2:0]
void oled_set_page_address(uint8_t page_addr_start, uint8_t page_addr_end)
{
    oled_send_command(0x22);
    oled_send_command(page_addr_start & 0x3);
    oled_send_command(page_addr_end & 0x3);
}


///// 4. Hardware Configuration (Panel resolution & layout related) Command
// Set Display Start Line: Set display RAM display start line register from 0-63
// 40~7F
void oled_set_display_start_line(uint8_t start_line)
{
    oled_send_command(0x40 | (start_line & 0x3F));
}


// Set Segment Re-map: 设置Segment重映射, 0为正常，1为左右反转
// A0/A1
void oled_set_segment_remap(bool remap)
{
    oled_send_command(remap ? 0xA1 : 0xA0);
}

// set multiplex ratio：15~63
// A8
// A[5:0]
void oled_set_multiplex_ratio(uint8_t ratio)
{
    oled_send_command(0xA8);
    oled_send_command(ratio & 0x3F);
}

// 设置列扫描方向：0从com0-comN，1反过来
// C0/C8
void oled_set_com_scan_direction(bool dir)
{
    oled_send_command(dir ? 0xC8 : 0xC0);
}

// Set Display Offset: Set vertical shift by COM from 0d~63d：0~63
// D3
// A[5:0]
void oled_set_display_offset(uint8_t offset)
{
    oled_send_command(0xD3);
    oled_send_command(offset & 0x3F);
}

// Set COM Pins Hardware Configuration
// DA
// A[5:4]
void oled_set_com_pin_hardware_configuration(bool alt_confg, bool enable_left_right_remap)
{
    oled_send_command(0xDA);
    oled_send_command((alt_confg ? (1 << 4) : 0) |(enable_left_right_remap ? (1 << 5) : 0) | 0x2);
}


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
void oled_set_display_clock(uint8_t div, uint8_t freq)
{
    oled_send_command(0xD5);
    oled_send_command((div & 0x0F) | ((freq & 0x0F) << 4));
}

// Set Pre-charge Period: 
// p1:  0~15
// p2:  0~15
// D9
// A[7:0]
void oled_set_precharge_period(uint8_t p1, uint8_t p2)
{
    oled_send_command(0xD9);
    oled_send_command((p1 & 0x0F) | ((p2 & 0x0F) << 4));
}

// Set VCOMH Deselect Level
// DB
// A[6:4]
void oled_set_vcomh_deselect_level(oled_vcomh_deselect_level_t level)
{
    oled_send_command(0xDB);
    oled_send_command((level & 0x7) << 4);
}


///// 6. Advance Graphic Command
// Set Fade Out and Blinking
// interval: 0~15： 8 Frames ~ 128 Frames
// 23
// A[6:0]
void oled_set_fade_blinking(oled_fade_blink_mode_t mode, uint8_t interval)
{
    oled_send_command(0x23);
    oled_send_command((interval & 0x0F) | ((mode & 0x03) << 4));
}


// Set Zoom In
// D6
// A[0]
void oled_set_zoom_in(bool enable)
{
    oled_send_command(0xD6);
    oled_send_command(enable ? 1 : 0);
}

///// 7. Charge Pump Command

// Charge Pump Setting
// 8D
// A[7:0]
void oled_set_charge_pump(bool enable)
{
    oled_send_command(0x8D);
    oled_send_command(enable ? 0x14 : 0x10);
}

////////
void oled_init(void)
{
    NEO_LOGI(TAG, "init");

    // register i2c device
    i2c_wrapper_add_dev(OLED_I2C_ADDR, 400000, &oled_dev_handle);

    oled_display_onoff(false);

    oled_set_addressing_mode(OLED_ADDRESSING_MODE_PAGE);

    oled_set_column_address_for_page_addressing(0x0);

    oled_set_display_start_line(0x0);

    oled_set_page_address_for_page_addressing(0x0);

    oled_set_contrast(0xFF);

    oled_set_segment_remap(true);

    oled_set_inverse(false);

    oled_set_multiplex_ratio(0x3F);

    oled_set_com_scan_direction(true);

    oled_set_display_offset(0);

    oled_set_display_clock(0, 8);

    oled_set_precharge_period(1, 0xF);

    oled_set_com_pin_hardware_configuration(true, false);

    oled_set_vcomh_deselect_level(OLED_VCOMH_DESELECT_LEVEL_083);

    oled_set_charge_pump(true);

    oled_display_onoff(true);

    oled_clear();
}

//////// 绘图API

// 根据oled_buffer_dirty的内容，将oled_buffer对应的数据写入显存，并清除对应标记
static void oled_redraw_buffer(void)
{
    uint8_t index;
    for (index = 0 ; index < 8; index ++) {
        if(oled_buffer_dirty[index].length != 0) {
            if(oled_buffer_dirty[index].length + oled_buffer_dirty[index].begin > OLED_WIDTH) {
                NEO_LOGW(TAG, "invalid begin %u or length %u", oled_buffer_dirty[index].begin, oled_buffer_dirty[index].length);
            } else {
                oled_set_page_address_for_page_addressing(index);
                oled_set_column_address_for_page_addressing(oled_buffer_dirty[index].begin);
                oled_send_data_buffer(oled_buffer[index] + oled_buffer_dirty[index].begin, oled_buffer_dirty[index].length);
            }
            oled_buffer_dirty[index].begin = oled_buffer_dirty[index].length = 0;
        }
    }
}

// 清屏
void oled_clear(void)
{
    uint8_t index;
    memset(oled_buffer, 0, sizeof(oled_buffer));
    memset(oled_buffer_dirty, 0, sizeof(oled_buffer_dirty));
    oled_display_onoff(false);
    for (index = 0 ; index < 8; index ++) {
        oled_set_page_address_for_page_addressing(index);
        oled_set_column_address_for_page_addressing(0);
        oled_send_data_buffer(oled_buffer[index], OLED_WIDTH);
    }
    oled_display_onoff(true);
}

// 在oled上填充一个实心矩形，坐标(x,y)是矩形左上角的点，w是宽度，h是高度，
// color是否反色，true为白色，false为黑色
void oled_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, bool color)
{
    uint8_t prefix_page_index, prefix_bits, prefix_mask;
    uint8_t page_cnt, i, j;
    uint8_t tail_page_index, tail_mask, tail_bits;

    /*
    NEO_LOGD(TAG, "oled_fill_rect [%u][%u][%u][%u][%d]", 
        x, y, w, h, color);
    */

    if(x > OLED_WIDTH - 1) x = OLED_WIDTH - 1;
    if(y > OLED_HEIGHT - 1)  y = OLED_HEIGHT;

    if(x + w > OLED_WIDTH) w = OLED_WIDTH - x;
    if(y + h > OLED_HEIGHT)  h = OLED_HEIGHT  - y;

    /*
    NEO_LOGD(TAG, "after cal [%u][%u][%u][%u][%d]", 
        x, y, w, h, color);
    */

    if(w == 0 || h == 0) return;

    // 计算前端未对齐需要处理多少零碎bit
    prefix_page_index = y / 8;
    prefix_bits = 8 - (y % 8);
    prefix_mask = 0xFF << (8 - prefix_bits);
    
    // 计算前端之后需要处理多少个整8字节？
    page_cnt = (h - prefix_bits) / 8;

    // 计算尾部需要处理多少个零碎bit
    tail_bits = h - prefix_bits - page_cnt * 8;
    tail_page_index = (y + h) / 8;
    tail_mask = 0xFF << tail_bits;

    /*
    NEO_LOGD(TAG, "prefix_page_index = %u", prefix_page_index);
    NEO_LOGD(TAG, "prefix_bits = %u", prefix_bits);
    NEO_LOGD(TAG, "prefix_mask = %02X", prefix_mask);
    NEO_LOGD(TAG, "page_cnt = %u", page_cnt);
    NEO_LOGD(TAG, "tail_page_index = %u", tail_page_index);
    NEO_LOGD(TAG, "tail_bits = %u", tail_bits);
    NEO_LOGD(TAG, "tail_mask = %02X", tail_mask);
    */

    // 写循环，遍历每一个扫描线
    for(i = x ; i < x + w ; i++) {
        if(prefix_bits) {
            oled_buffer[prefix_page_index][i] = 
                ((oled_buffer[prefix_page_index][i] & ~prefix_mask) | 
                ((color ? 0xFF : 0x00) & prefix_mask)); 
        }

        if(page_cnt) {
            for( j = prefix_page_index + 1 ; j <= prefix_page_index + page_cnt ; j ++) {
                oled_buffer[j][i] = (color ? 0xFF : 0x00);
            }
        }

        if(tail_bits) {
            oled_buffer[tail_page_index][i] = 
                ((oled_buffer[tail_page_index][i] & tail_mask) | 
                ((color ? 0xFF : 0x00) & ~tail_mask));
        }
    }

    // 更新oled_buffer_dirty
    for(i = prefix_page_index ; i <= tail_page_index ; i++) {
        oled_buffer_dirty[i].begin = x;
        oled_buffer_dirty[i].length = w;
    }

    // 同步变化给OLED屏幕
    oled_redraw_buffer();
}

// 在oled上显示一个6*8的ASCII字符(使用自带字库)，坐标(x,y)是字符左上角的点，
// invert是否反色
// type 渲染类型
// 注：绘制字符始终是掩膜绘制，也就是0的位是透明的
void oled_draw_char_6X8(uint8_t x, uint8_t y, char c, bool invert, oled_draw_type_t type)
{
    uint32_t char_cnt = sizeof(oled_ascii_F6x8) / 6;
    uint32_t index = c - ' ';
    uint8_t char_buffer[6];
    uint8_t i;

    if(index >= char_cnt) index = char_cnt - 1;

    for(i = 0 ; i < 6 ; i ++) {
        char_buffer[i] = invert ? ~oled_ascii_F6x8[index * 6 + i] : oled_ascii_F6x8[index * 6 + i];
    }
    oled_draw_bitmap(x, y, 6, 8, char_buffer, type);
}

// 在oled上显示一个8*16的ASCII字符(使用自带字库)，坐标(x,y)是字符左上角的点，
// invert是颜色
// type 渲染类型
// 注：绘制字符始终是掩膜绘制，也就是0的位是透明的
void oled_draw_char_8X16(uint8_t x, uint8_t y, char c, bool invert, oled_draw_type_t type)
{
    uint32_t char_cnt = sizeof(oled_ascii_F8X16) / 16;
    uint32_t index = c - ' ';
    uint8_t char_buffer[16];
    uint8_t i;

    if(index >= char_cnt) index = char_cnt - 1;

    for(i = 0 ; i < 16 ; i ++) {
        char_buffer[i] = invert ? ~oled_ascii_F8X16[index * 16 + i] : oled_ascii_F8X16[index * 16 + i];
    }
    oled_draw_bitmap(x, y, 8, 16, char_buffer, type);
}

// 在oled上显示一个bitmap，坐标(x,y)是bitmap左上角的点，w是宽度，h是高度，
// type 渲染类型
// bitmap是一个字节数组，采用列行式存储：
// bitmap大小为(w * ((h + 7) & ~7) / 8)字节，即将h向上取整到8的倍数，然后乘以w，得到总字节数
// bitmap的每个字节表示8个像素，从左到右表示w个从上到下的竖立的8位扫描线，最后的w个扫描线不足8位的部分用0填充
// 可以用pctolcd工具将图片转换为这种格式的bitmap
// 函数用Google Gemini写的。。。。
void oled_draw_bitmap(int32_t x, int32_t y, uint16_t w, uint16_t h, const uint8_t *bitmap, oled_draw_type_t type) 
{
     /* 1. 局部变量统一最前端声明 */
    int32_t dirty_x_start;
    int32_t dirty_x_end;
    int32_t dirty_y_start;
    int32_t dirty_y_end;
    uint8_t dirty_w;
    uint8_t page_start;
    uint8_t page_end;
    uint8_t p;

    uint16_t bx;
    uint16_t by_byte;
    uint16_t bitmap_aligned_h;
    uint16_t total_src_bytes;
    
    int32_t screen_x;
    int32_t current_bit_y;
    int32_t target_y;
    int32_t tgt_page;
    
    uint8_t shift_up;
    uint8_t shift_down;
    uint8_t src_data;
    uint8_t h_mask;
    uint8_t total_mask;
    uint8_t data_up;
    uint8_t data_down;

    /* 2. 边界裁剪 */
    if (!bitmap || w == 0 || h == 0 ||
        x >= OLED_WIDTH || (x + (int32_t)w) <= 0 ||
        y >= OLED_HEIGHT || (y + (int32_t)h) <= 0) {
        return;
    }

    /* 3. 脏区域直接覆盖计算 */
    dirty_x_start = (x < 0) ? 0 : x;
    dirty_x_end   = ((x + (int32_t)w) > OLED_WIDTH) ? (OLED_WIDTH - 1) : (x + (int32_t)w - 1);
    dirty_w       = (uint8_t)(dirty_x_end - dirty_x_start + 1);

    dirty_y_start = (y < 0) ? 0 : y;
    dirty_y_end   = ((y + (int32_t)h) > OLED_HEIGHT) ? (OLED_HEIGHT - 1) : (y + (int32_t)h - 1);

    page_start = (uint8_t)(dirty_y_start / 8);
    page_end   = (uint8_t)(dirty_y_end / 8);

    for (p = page_start; p <= page_end; p++) {
        oled_buffer_dirty[p].begin  = (uint8_t)dirty_x_start;
        oled_buffer_dirty[p].length = dirty_w;
    }

    /* 4. 核心优化：按字节快速渲染（处理烦人的边界条件） */
    bitmap_aligned_h = (h + 7) & ~7;
    total_src_bytes  = bitmap_aligned_h / 8;

    /* 计算由于 y 错位引发的移位参数 (使用正余数逻辑) */
    shift_up   = (y >= 0) ? (uint8_t)(y % 8) : (uint8_t)(8 - ((-y) % 8));
    if (shift_up == 8) shift_up = 0;
    shift_down = 8 - shift_up;

    for (bx = 0; bx < w; bx++) {
        screen_x = x + bx;
        if (screen_x < 0 || screen_x >= OLED_WIDTH) continue;

        for (by_byte = 0; by_byte < total_src_bytes; by_byte++) {
            src_data = bitmap[(by_byte * w) + bx];
            current_bit_y = y + (by_byte * 8);

            /* 高度非8对齐的截断掩码：防止读到最后一页补零的无效数据损坏屏幕 */
            h_mask = 0xFF;
            if (by_byte == (total_src_bytes - 1)) {
                h_mask = (uint8_t)((1 << (h - (by_byte * 8))) - 1);
            }
            src_data &= h_mask;

            /* --- 4.1 处理错位后“分裂”出来的上半部分数据 --- */
            target_y = current_bit_y;
            tgt_page = target_y / 8;
            /* 解决 C 语言负数除法向零取整的问题：如果目标 Y 为负数，不属于任何合法的 Page */
            if (target_y >= 0 && tgt_page < OLED_PAGES) {
                /* 计算上半部分移位结果以及有效光栅掩码 */
                data_up    = (uint8_t)(src_data << shift_up);
                total_mask = (uint8_t)(h_mask << shift_up);

                if (type == OLED_DRAW_OVERWRITE) {
                    oled_buffer[tgt_page][screen_x] &= ~total_mask;
                    oled_buffer[tgt_page][screen_x] |= data_up;
                } else if (type == OLED_DRAW_OR) {
                    oled_buffer[tgt_page][screen_x] |= data_up;
                } else if (type == OLED_DRAW_XOR) {
                    oled_buffer[tgt_page][screen_x] ^= data_up;
                }
            }

            /* --- 4.2 处理错位后“分裂”出来的下半部分数据 --- */
            if (shift_up != 0) { /* 只有当发生跨页错位时，才需要处理下半部分 */
                target_y = current_bit_y + 8;
                tgt_page = target_y / 8;
                if (target_y >= 0 && tgt_page < OLED_PAGES) {
                    /* 计算下半部分移位结果以及有效光栅掩码 */
                    data_down  = (uint8_t)(src_data >> shift_down);
                    total_mask = (uint8_t)(h_mask >> shift_down);

                    if (type == OLED_DRAW_OVERWRITE) {
                        oled_buffer[tgt_page][screen_x] &= ~total_mask;
                        oled_buffer[tgt_page][screen_x] |= data_down;
                    } else if (type == OLED_DRAW_OR) {
                        oled_buffer[tgt_page][screen_x] |= data_down;
                    } else if (type == OLED_DRAW_XOR) {
                        oled_buffer[tgt_page][screen_x] ^= data_down;
                    }
                }
            }
        }
    }

    /* 5. 同步变化给OLED屏幕 */
    oled_redraw_buffer();
}
