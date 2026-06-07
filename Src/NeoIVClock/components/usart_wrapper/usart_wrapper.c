#include "usart_wrapper.h"
#include "logger.h"
#include "rom/uart.h"

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

int32_t usart_wrapper_read_frame(usart_wrapper_dev_handle_t * dev_handle, 
                                void * buffer, 
                                uint32_t buffer_size, 
                                const uint8_t * sig, 
                                uint32_t sig_len, 
                                uint8_t length_offset, 
                                uint8_t length_len, 
                                USART_LENGTH_FUNC length_fun, 
                                USART_CHECKSUM_FUNC chksum_fun)
{
    uint8_t *buf = (uint8_t *)buffer;
    uint32_t frame_len = 0;          // 当前已收集到的数据长度
    uint32_t length_field_end = (uint32_t)length_offset + length_len;
    int32_t remaining_payload_len, read_bytes;
    uint32_t total_expected_frame_len, need_bytes;
    // ==========================================
    // 1. 参数合法性检查
    // ==========================================
    if (!dev_handle || !buffer || buffer_size == 0 || !sig || sig_len == 0 || !length_fun) {
        // [异常: 非法参数传入] 
        NEO_LOGE(TAG, "usart_read_frame: invalid param");
        return USART_FRAME_ERR_INVALID_PARAM;
    }

    // ==========================================
    // 2. 流式解析状态机
    // ==========================================
    while (1) {
        // ------------------------------------------
        // 状态 A: 寻找并匹配完整的帧头
        // ------------------------------------------
        if (frame_len < sig_len) {
            uint8_t next_byte;
            int32_t read_bytes = usart_wrapper_read((usart_wrapper_dev_handle_t *)dev_handle, &next_byte, 1);
            if (read_bytes <= 0) {
                // [提示: 串口流读空] 
                return USART_FRAME_OK_EMPTY; 
            }

            if (next_byte == sig[frame_len]) {
                buf[frame_len++] = next_byte;
            } else {
                // 帧头不匹配，直接从头重新匹配
                NEO_LOGW(TAG, "usart_read_frame: drop garbage byte [%02x] before found frame", next_byte);
                frame_len = 0;
                if (next_byte == sig[0]) {
                    buf[frame_len++] = next_byte;
                }
            }
            continue; 
        }

        // ------------------------------------------
        // 状态 B: 帧头已锁定，继续收集直到覆盖长度字段
        // ------------------------------------------
        if (frame_len < length_field_end) {
            need_bytes = length_field_end - frame_len;
            
            if (length_field_end > buffer_size) {
                // [异常: 长度字段自身超限]
                NEO_LOGW(TAG, "usart_read_frame: buffer size %u too short for %u, %d bytes readed", buffer_size, length_field_end, frame_len);
                NEO_LOGW_HEX(TAG, buf, frame_len);
                frame_len = 0;
                return USART_FRAME_ERR_BUF_SHORT; 
            }

            read_bytes = usart_wrapper_read((usart_wrapper_dev_handle_t *)dev_handle, &buf[frame_len], need_bytes);
            if (read_bytes <= 0) {
                NEO_LOGW(TAG, "usart_read_frame: pre-mature frame %d bytes", frame_len);
                NEO_LOGW_HEX(TAG, buf, frame_len);
                return USART_FRAME_OK_EMPTY; 
            }
            frame_len += read_bytes;
            continue;
        }

        // ------------------------------------------
        // 状态 C: 解析长度字段，计算整帧所需大小
        // ------------------------------------------
        remaining_payload_len = length_fun(&buf[length_offset], length_len);
        if (remaining_payload_len < 0) {
            // [异常: 长度字段解析非法]
            NEO_LOGW(TAG, "usart_read_frame: invalid length field %d bytes", length_len);
            NEO_LOGW_HEX(TAG, &buf[length_offset], length_len);  
            frame_len = 0;
            return USART_FRAME_ERR_INVALID;
        }

        total_expected_frame_len = length_field_end + (uint32_t)remaining_payload_len;
        if (total_expected_frame_len > buffer_size) {
            // [异常: 接收缓冲区不够大]
            NEO_LOGW(TAG, "usart_read_frame: buffer size %u too short for %u, %d bytes readed", buffer_size, total_expected_frame_len, frame_len);
            NEO_LOGW_HEX(TAG, buf, frame_len);            
            frame_len = 0;
            return USART_FRAME_ERR_BUF_SHORT; 
        }

        // ------------------------------------------
        // 状态 D: 读取该帧剩余的所有消息体/校验和字节
        // ------------------------------------------
        if (frame_len < total_expected_frame_len) {
            need_bytes = total_expected_frame_len - frame_len;
            read_bytes = usart_wrapper_read((usart_wrapper_dev_handle_t *)dev_handle, &buf[frame_len], need_bytes);
            if (read_bytes <= 0) {
                return USART_FRAME_OK_EMPTY; 
            }
            frame_len += read_bytes;
            continue; 
        }

        // ------------------------------------------
        // 状态 E: 完整帧收集完毕，执行校验和函数
        // ------------------------------------------
        if (chksum_fun != NULL) {
            if (!chksum_fun(buf, total_expected_frame_len)) {
                // [异常: 校验和(Checksum)错误]
                NEO_LOGW(TAG, "usart_read_frame: invalid chksum");
                NEO_LOGW_HEX(TAG, buf, total_expected_frame_len);
                frame_len = 0;
                return USART_FRAME_ERR_INVALID;
            }
        }

        // ==========================================
        // 3. 完美通行，返回成功
        // ==========================================
        return (int32_t)total_expected_frame_len;
    }
}
