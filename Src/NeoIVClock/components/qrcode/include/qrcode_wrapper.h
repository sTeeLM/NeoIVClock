#ifndef NEO_IV_CLOCK_QRCODE_WRAPPER_H
#define NEO_IV_CLOCK_QRCODE_WRAPPER_H

#include <stdint.h>

/**
 * @brief 将 Version 4 的 QRCode 点阵转换为标准的 SSD1306 OLED 垂直 Page 格式位图
 * 
 * @details 该函数不使用任何动态内存分配。它读取 QRCode 的 33x33 矩阵，
 *          并将其按照 SSD1306 的 GDDRAM 映射硬件特性（纵向每 8 个像素拼成 1 个字节，低位在上，高位在下；
 *          横向自左向右扫描）转换为可以直接进行 I2C/SPI 刷屏的单色 Bitmap。
 * 
 * @param[in] text 需要生成二维码的原始字符串（如配置网页 URL）
 * @return bitmap缓冲区
 */
uint8_t * qrcode_wrapper_draw_txt(const char *text);

#endif // NEO_IV_CLOCK_QRCODE_WRAPPER_H