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
    uint32_t clk_speed_hz,
    i2c_wrapper_dev_handle_t * dev_handle);

typedef enum _i2c_wrapper_addr_mode_t
{
  I2C_ADDR_MODE_8BIT = 0,
  I2C_ADDR_MODE_16BIT = 1
} i2c_wrapper_addr_mode_t;

void i2c_wrapper_write(
  i2c_wrapper_dev_handle_t * dev_handle, 
  uint16_t reg_addr, 
  i2c_wrapper_addr_mode_t reg_addr_mode,
  const uint8_t * data, 
  size_t data_len);

void i2c_wrapper_read(
  i2c_wrapper_dev_handle_t * dev_handle, 
  uint16_t reg_addr, 
  i2c_wrapper_addr_mode_t reg_addr_mode,
  uint8_t * data, 
  size_t data_len);

bool i2c_wrapper_raw_read(
  i2c_wrapper_dev_handle_t * dev_handle, 
  uint8_t * data, 
  size_t data_len);

void i2c_wrapper_bus_reset(void);

#endif // NEO_IV_CLOCK_I2C_WRAPPER_H
