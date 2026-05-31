#ifndef NEO_IV_CLOCK_CEXT_H
#define NEO_IV_CLOCK_CEXT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define cext_is_leap_year(y) \
(( ((y % 100) !=0) && ((y % 4)==0)) || ( (y % 400) == 0))

// 温度单位换算
#define cext_celsius_to_fahrenheit(c) (((float)(c) * 9) / 5 + 32)
#define cext_fahrenheit_to_celsius(f) (((float)(f) - 32) * 5 / 9)

// pa和mmHg的换算，1 mmHg = 133.322 Pa
#define cext_pa_to_mmhg(pa) ((float)(pa) / 133.322f)
#define cext_mmhg_to_pa(mmhg) ((float)(mmhg) * 133.322f)

// pa和atm的换算，1 atm = 101325 Pa
#define cext_pa_to_atm(pa) ((float)(pa) / 101325.0f)
#define cext_atm_to_pa(atm) ((float)(atm) * 101325.0f)

// 24小时制的小时转换成12小时制的小时，返回值是是否是下午
bool cext_cal_hour12(uint8_t hour, uint8_t * hour12);

// IIR 滤波
uint16_t cext_iir_uint16(uint16_t oldv, uint16_t newv, uint8_t coe);
float cext_iir_float(float oldv, float newv, uint8_t coe);

// 线性拟合
int32_t cext_linear_interpolate(int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t x);
float cext_linear_interpolate_float(float x1, float y1, float x2, float y2, float x);

/* 无符号时钟代数循环加 N 宏 */
#define cext_ring_add(v, n, minv, maxv) (minv + ((v - minv + n) % (maxv - minv + 1))) 

/* 无符号时钟代数循环减 N 宏 */
#define cext_ring_sub(v, n, minv, maxv) (minv + ((v - minv + ((maxv - minv + 1) - (n % (maxv - minv + 1)))) % (maxv - minv + 1)))

/* 无符号整数上下线 */
#define cext_limit(v, minv, maxv) (v < minv ? minv : (v > maxv ? maxv : v))

// 就地 URL 解码函数（纯静态，无内存申请）
void cext_url_decode_inplace(char *str);

/**
 * @brief 从 HTTP 表单 K-V 键值对流（Query String）中安全提取指定键的值并进行 URL 解码
 * 
 * @details 该函数采用全静态、零内存申请（Zero-malloc）的指针流式扫描方式。
 *          通过匹配 "key=" 准确定位参数起始点，并支持前缀安全隔离（防止 my_pass 匹配到 pass）。
 *          定位后自动截取数据至目标静态缓冲区，并内嵌就地（In-place）URL 解码算法，
 *          将表单转义字符（如 '+' 转为空格，'%XX' 转为标准 ASCII 字符）完美还原。
 *          同时包含严格的边界检查，防止恶意长输入导致本地静态缓冲区发生溢出。
 * 
 * @param[in]  query_str    包含所有表单键值对的原始 HTTP 文本数据流（如 "ssid=abc&pass=123"）
 * @param[in]  key          需要提取的目标键名（不带 '=' 符号，如 "ssid"、"pass"）
 * @param[out] dest_buf     用于存储提取并解码后的干净字符串的目标静态缓冲区指针
 * @param[in]  dest_max_len 目标缓冲区的最大安全可用容量（字节数，包含 '\0' 终止符位置）
 * 
 * @return bool 解析与提取结果状态：
 *         - true  : 成功在流中找到对应的键，完成数据截取、防溢出裁剪，且就地解码成功
 *         - false : 找不到目标键、键格式损毁（不带'='）、或者传入的关键参数指针为 NULL
 */
bool cext_extract_form_value(const char *query_str, const char *key, char *dest_buf, size_t dest_max_len);

#endif