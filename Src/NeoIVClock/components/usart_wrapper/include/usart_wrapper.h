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

#endif // NEO_IV_CLOCK_USART_WRAPPER_H