#ifndef NEO_IV_CLOCK_I2C_WRAPPER_H
#define NEO_IV_CLOCK_I2C_WRAPPER_H

#include "driver/i2c_master.h"

typedef struct _i2c_wrapper_dev_handle_t
{
    i2c_master_dev_handle_t real_handle;
} i2c_wrapper_dev_handle_t;

void i2c_wrapper_init(void);

void i2c_wrapper_add_dev(
    uint16_t addr, 
    i2c_wrapper_dev_handle_t * dev_handle);

void i2c_wrapper_write(
  i2c_wrapper_dev_handle_t * dev_handle, 
  uint16_t reg_addr, 
  uint8_t  reg_addr_len, 
  const uint8_t * data, 
  size_t data_len);

void i2c_wrapper_read(
  i2c_wrapper_dev_handle_t * dev_handle, 
  uint16_t reg_addr, 
  uint8_t  reg_addr_len, 
  uint8_t * data, 
  size_t data_len);

#endif // NEO_IV_CLOCK_I2C_WRAPPER_H
