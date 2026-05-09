#include "usart_wrapper.h"
#include "logger.h"

static const char * TAG = "USART";

#define USART_TIMEOUT_MS 1000

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
  ESP_ERROR_CHECK(uart_driver_install(uart_num, 1024 * 2, 0, 0, NULL, 0));
}

void usart_wrapper_write(usart_wrapper_dev_handle_t * dev_handle, const uint8_t * data, size_t data_len)
{
  ESP_ERROR_CHECK(uart_write_bytes(dev_handle->uart_num, data, data_len));
}

void usart_wrapper_flush(usart_wrapper_dev_handle_t * dev_handle)
{
  ESP_ERROR_CHECK(uart_flush(dev_handle->uart_num));
}

ssize_t usart_wrapper_read(usart_wrapper_dev_handle_t * dev_handle, uint8_t * data, size_t data_len)
{
  return uart_read_bytes(dev_handle->uart_num, data, data_len, USART_TIMEOUT_MS/portTICK_PERIOD_MS);
}