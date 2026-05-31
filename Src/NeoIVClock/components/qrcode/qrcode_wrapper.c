#include "qrcode.h"
#include "esp_attr.h"
#include <string.h>

#define QR_VERSION    4
#define QR_SIZE       33 // Version 4 固定是 33x33

// ==========================================
// 1. 静态分配 PSRAM 缓冲区（符合零 malloc 原则）
// ==========================================
// 存储 qrcode 内部运算位图（Version 4 计算得 176 字节）
EXT_RAM_BSS_ATTR static uint8_t qrcode_wrapper_raw_data[176];

// 存储最终生成的、可以直接塞给 SSD1306 驱动渲染的单色标准 Bitmap
// SSD1306 单色位图大小计算：宽度 33 像素，高度 33 像素。
// 高度 33 像素需要：ceil(33 / 8) = 5 个 Page（行字节数）。
// 总字节数 = 宽度 33 * 5 = 165 字节。
EXT_RAM_BSS_ATTR static uint8_t qrcode_wrapper_bitmap[165];

uint8_t * qrcode_wrapper_draw_txt(const char *text)
{
  if (text == NULL) return NULL;

  QRCode qrcode;
  // 1. 清空全局静态缓冲区
  memset(qrcode_wrapper_raw_data, 0, sizeof(qrcode_wrapper_raw_data));
  memset(qrcode_wrapper_bitmap, 0, sizeof(qrcode_wrapper_bitmap));

  // 2. 初始化并生成二维码（Version 4, 低纠错等级）
  if (qrcode_initText(&qrcode, qrcode_wrapper_raw_data, QR_VERSION, ECC_LOW, text) != 0) {
    return NULL; // 文本过长，Version 4 无法容纳
  }

  // 3. 核心转换逻辑：将 (X, Y) 坐标点阵重组为 SSD1306 的 Page 字节流
  // SSD1306 的每一列由 5 个 Page 组成（前 4 个 Page 占 32 行，第 5 个 Page 占剩下的 1 行）
  for (uint8_t y = 0; y < QR_SIZE; y++) {
    uint8_t page_index = y / 8; // 当前行属于第几个 Page (0 ~ 4)
    uint8_t bit_index  = y % 8; // 当前行在当前 Page 字节中的哪一位 (0 ~ 7)

    for (uint8_t x = 0; x < QR_SIZE; x++) {
      // 检查当前二维码坐标点是否为黑点
      if (qrcode_getModule(&qrcode, x, y)) {
        // 计算在最终一维数组 qrcode_wrapper_bitmap 中的偏移量
        // SSD1306 扫描方式：先画完一行的第一个 Page，再画下一列。所以列(x)是主序
        int bitmap_pos = (page_index * QR_SIZE) + x;
        
        // 乐鑫和标准 SSD1306 硬件规定：低位（BIT0）对应屏幕上方，高位（BIT7）对应下方
        qrcode_wrapper_bitmap[bitmap_pos] |= (1 << bit_index);
      }
    }
  }

  return qrcode_wrapper_bitmap;
}
