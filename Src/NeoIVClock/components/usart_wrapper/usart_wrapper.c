#include "usart_wrapper.h"
#include "logger.h"

static const char * TAG = "USART";

#define USART_TIMEOUT_MS 100

void usart_wrapper_init(void)
{
  NEO_LOGI(TAG, "init");
}

void usart_wrapper_dev_add(
  usart_wrapper_dev_handle_t * dev_handle,
  uart_port_t uart_num, 
  int tx_io_num, 
  int rx_io_num, 
  const uart_config_t * uart_config)
{
  dev_handle->uart_num = uart_num;
  dev_handle->uart_config = *uart_config;
  ESP_ERROR_CHECK(uart_param_config(uart_num, uart_config));
  ESP_ERROR_CHECK(uart_set_pin(uart_num, tx_io_num, rx_io_num, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
  ESP_ERROR_CHECK(uart_driver_install(uart_num, 1024 * 4, 0, 0, NULL, 0));
}

ssize_t usart_wrapper_write(usart_wrapper_dev_handle_t * dev_handle, const uint8_t * data, size_t data_len)
{
  return uart_write_bytes(dev_handle->uart_num, data, data_len);
}

bool usart_wrapper_flush(usart_wrapper_dev_handle_t * dev_handle)
{
  return uart_flush(dev_handle->uart_num) == ESP_OK;
}

ssize_t usart_wrapper_read(usart_wrapper_dev_handle_t * dev_handle, uint8_t * data, size_t data_len)
{
  return uart_read_bytes(dev_handle->uart_num, data, data_len, USART_TIMEOUT_MS/portTICK_PERIOD_MS);
}

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* 抽象帧状态机枚举 */
typedef enum _usart_wrapper_rx_generic_state_t{
    USART_WRAPPER_STATE_SYNC_SIGNATURE, // 匹配特征头阶段
    USART_WRAPPER_STATE_READ_BODY       // 读取剩余身体数据阶段
} usart_wrapper_rx_generic_state_t;

/**
 * @brief 支持任意特征头的流式数据帧读取并排空缓冲区
 * 
 * @param res            接收成功后填充的目标结构体/内存指针
 * @param res_total_len  目标数据帧的总字节数（即 sizeof(pms5003st_res_msg_t)）
 * @param sig            指向用于匹配的任意长度特征头数组指针
 * @param sig_len        特征头的长度
 * @return bool          如果成功解析出至少一条完整消息返回 true，否则返回 false
 */
bool usart_wrapper_read_with_sigature(
  usart_wrapper_dev_handle_t * dev_handle, 
  void * res, uint32_t res_total_len, const uint8_t * sig, uint32_t sig_len) 
{
    /* ==================== 1. 所有变量声明严格放置在最前端 ==================== */
    usart_wrapper_rx_generic_state_t state;
    uint8_t single_byte;
    uint32_t sig_match_cnt;
    uint32_t body_bytes_read;
    uint32_t total_body_len;
    uint8_t *res_raw;
    bool got_message;
    ssize_t read_len;
    uint8_t trash_bin[16];

    /* ==================== 2. 安全边界检查 ==================== */
    if (res == NULL || sig == NULL) {
        return false;
    }
    // 特征头长度不能大于等于总帧长，且不能为空
    if (sig_len == 0 || sig_len >= res_total_len) {
        return false;
    }

    /* ==================== 3. 变量初始化赋值 ==================== */
    state = USART_WRAPPER_STATE_SYNC_SIGNATURE;
    single_byte = 0;
    sig_match_cnt = 0;
    body_bytes_read = 0;
    got_message = false;
    
    // 剩余身体长度 = 总帧长 - 特征头长度
    total_body_len = res_total_len - sig_len;
    res_raw = (uint8_t *)res;

    /* ==================== 4. 核心动态流式状态机 ==================== */
    while (!got_message) {
        // 从串口尝试读取 1 个字节
        read_len = usart_wrapper_read(dev_handle, &single_byte, 1);
        
        // 缓冲区读空则退出当前消息查找
        if (read_len <= 0) {
            break;
        }

        switch (state) {
            case USART_WRAPPER_STATE_SYNC_SIGNATURE:
                // 检查当前字节是否匹配特征头中对应索引的预期字节
                if (single_byte == sig[sig_match_cnt]) {
                    // 暂时将匹配到的特征头字节直接写入结果缓冲区中
                    res_raw[sig_match_cnt] = single_byte;
                    sig_match_cnt++;
                    
                    // 当匹配数量等于指定的 signature 长度时，代表特征头同步完成
                    if (sig_match_cnt >= sig_len) {
                        body_bytes_read = 0;
                        state = USART_WRAPPER_STATE_READ_BODY; // 切换到身体读取状态
                    }
                } else {
                    // 【关键防死锁设计】：如果中途匹配失败，回退机制
                    if (sig_match_cnt > 0) {
                        /* 
                         * 极端情况处理：当前字节不匹配 sig[sig_match_cnt]，但它有可能正好是 
                         * 整个 signature 的第 0 个字节（例如头是 0x42 0x42 0x4D，当前收到 0x42 0x42 0x42）。
                         * 如果是，我们不能直接清零，而是应该将其算作新的第 0 字节重新开始。
                         */
                        if (single_byte == sig[0]) {
                            res_raw[0] = single_byte;
                            sig_match_cnt = 1;
                        } else {
                            sig_match_cnt = 0; // 彻底不匹配，计数器清零重新寻找头
                        }
                    }
                }
                break;

            case USART_WRAPPER_STATE_READ_BODY:
                // 从紧跟在 signature 结束后的内存地址开始按字节顺序填充身体
                res_raw[sig_len + body_bytes_read] = single_byte;
                body_bytes_read++;

                if (body_bytes_read >= total_body_len) {
                    got_message = true; // 身体接收完毕，集齐完整一帧
                }
                break;

            default:
                state = USART_WRAPPER_STATE_SYNC_SIGNATURE;
                break;
        }
    }

    if(got_message) {
      /* ==================== 5. 强制排空接收缓冲区中的干扰残留 ==================== */
      while ((read_len = usart_wrapper_read(dev_handle, trash_bin, sizeof(trash_bin))) > 0) {
          // 通过循环直至返回 0，确保硬件缓冲区彻底被冲刷干净
          NEO_LOGW(TAG, "clear trash bin %d bytes", read_len);
          NEO_LOGW_HEX(TAG, trash_bin, read_len);        
      }
    }

    return got_message;
}
