#ifndef NEO_IV_CLOCK_USART_WRAPPER_H
#define NEO_IV_CLOCK_USART_WRAPPER_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"

void usart_wrapper_init(void);

typedef struct _usart_wrapper_dev_handle_t
{
    uart_port_t uart_num;
    uart_config_t uart_config;
} usart_wrapper_dev_handle_t;

void usart_wrapper_dev_add(
  usart_wrapper_dev_handle_t * dev_handle,
  uart_port_t uart_num, 
  int tx_io_num, 
  int rx_io_num, 
  const uart_config_t * uart_config);

ssize_t usart_wrapper_write(usart_wrapper_dev_handle_t * dev_handle, const uint8_t * data, size_t data_len);

bool usart_wrapper_flush(usart_wrapper_dev_handle_t * dev_handle);

ssize_t usart_wrapper_read(usart_wrapper_dev_handle_t * dev_handle, uint8_t * data, size_t data_len);


// ==========================================
// 异常与状态返回值宏定义
// ==========================================
#define USART_FRAME_OK_EMPTY            0   // 直到读空缓冲区也没有找到完整的帧
#define USART_FRAME_ERR_INVALID        -1   // 非法帧（校验和错误，数据长度错误）
#define USART_FRAME_ERR_BUF_SHORT      -2   // 传入的 buffer 不够大
#define USART_FRAME_ERR_INVALID_PARAM  -3   // 传入了非法参数（如函数指针或缓冲区指针为NULL）

// 句柄与回调函数指针定义
typedef int32_t (*USART_LENGTH_FUNC)(const uint8_t * length_buffer, uint8_t length);
typedef bool (*USART_CHECKSUM_FUNC)(const void * buffer, uint32_t buffer_length);

/**
 * @brief 从串口流中读取并解析出一个完整的数据帧（通用流式数据帧解析函数）
 * 
 * @details 该函数采用逐字节状态机机制。锁定帧头后，通过用户传入的 length_fun 回调解析长度字段。
 *          若中途遇到数据长度非法、缓冲区不足或 chksum_fun 校验和失败，函数将直接重置接收状态并返回错误。
 * 
 * @param[in]  dev_handle    串口设备包装句柄，将作为参数传入底层的 usart_wrapper_read
 * @param[out] buffer        用于存储并返回提取出的整帧数据的缓冲区
 * @param[in]  buffer_size   用户提供的返回缓冲区的大小（字节数）
 * @param[in]  sig           期望匹配的帧头特征字节数组（魔术字）
 * @param[in]  sig_len       帧头特征字节数组的长度
 * @param[in]  length_offset 长度字段相对于整帧起始位置（即帧头首字节）的偏移量
 * @param[in]  length_len    长度字段本身所占用的字节数
 * @param[in]  length_fun    长度解析回调函数。传入长度字段缓冲区，需返回后续还需读取的字节数。
 *                           若字段非法需返回负数。
 * @param[in]  chksum_fun    校验和检查回调函数。传入整帧缓冲区和整帧总长度，返回布尔值。
 *                           可传入 NULL 表示不执行校验和检查。
 * 
 * @return int32_t 解析结果状态或长度：
 *         - 正数                      : 成功读取到一帧完整且通过校验的数据，返回值为该帧的总长度（字节）
 *         - USART_FRAME_OK_EMPTY     (0) : 串口底层流已读空，但目前仍未凑齐一个完整的有效帧
 *         - USART_FRAME_ERR_INVALID  (-1): 读取到了一帧，但其为非法帧（数据长度非法或校验和错误）
 *         - USART_FRAME_ERR_BUF_SHORT(-2): 帧长度或长度字段位置超出了传入的 buffer_size 限制
 *         - USART_FRAME_ERR_INVALID_PARAM (-3): 关键输入参数传入了空指针或缓冲区大小为0
 */
int32_t usart_read_frame(usart_wrapper_dev_handle_t * dev_handle, 
                                void * buffer, 
                                uint32_t buffer_size, 
                                const uint8_t * sig, 
                                uint32_t sig_len, 
                                uint8_t length_offset, 
                                uint8_t length_len, 
                                USART_LENGTH_FUNC length_fun, 
                                USART_CHECKSUM_FUNC chksum_fun);

#endif // NEO_IV_CLOCK_USART_WRAPPER_H