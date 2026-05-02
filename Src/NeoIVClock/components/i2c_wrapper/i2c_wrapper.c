#include "i2c_wrapper.h"
#include "logger.h"
#include "gpio_wrapper.h"

#define I2C_MASTER_TIMEOUT_MS 1000

static const char * TAG = "I2C";

static i2c_master_bus_handle_t i2c_bus_handle;

void i2c_wrapper_init(void)
{
  i2c_master_bus_config_t i2c_mst_config = {
      .clk_source = I2C_CLK_SRC_DEFAULT,
      .i2c_port   = 10,
      .scl_io_num = I2C_SCL_GPIO_PIN,
      .sda_io_num = I2C_SDA_GPIO_PIN,
      .glitch_ignore_cnt = 7,
  };  
  
  NEO_LOGI(TAG, "init");

  ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &i2c_bus_handle));
}

void i2c_wrapper_add_dev(uint16_t addr, i2c_wrapper_dev_handle_t * dev_handle)
{
  i2c_device_config_t dev_config = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = addr,
    .scl_speed_hz = 100000,
  };
  ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_bus_handle, &dev_config, &dev_handle->real_handle));
} 

void i2c_wrapper_write(
  i2c_wrapper_dev_handle_t * dev_handle, 
  uint16_t reg_addr, 
  uint8_t reg_addr_len, 
  const uint8_t * data, 
  size_t data_len)
{
  uint16_t reg_addr_buffer;

  i2c_master_transmit_multi_buffer_info_t i2c_buffer[2] = {};

  if(reg_addr_len == 1)
    reg_addr_buffer = reg_addr & 0xFF;
  else if(reg_addr_len == 2)
    reg_addr_buffer = reg_addr & 0xFFFF;
  else
    reg_addr_buffer = 0; // Invalid address length, default to 0


  i2c_buffer[0].write_buffer = (const uint8_t *)&reg_addr_buffer;
  i2c_buffer[0].buffer_size = reg_addr_len;
  i2c_buffer[1].write_buffer = data;
  i2c_buffer[1].buffer_size = data_len;

  ESP_ERROR_CHECK(i2c_master_multi_buffer_transmit(dev_handle->real_handle, i2c_buffer, 2, I2C_MASTER_TIMEOUT_MS));
}

void i2c_wrapper_read(
  i2c_wrapper_dev_handle_t * dev_handle, 
  uint16_t reg_addr, 
  uint8_t reg_addr_len, 
  uint8_t * data, 
  size_t data_len)
{
  ESP_ERROR_CHECK(i2c_master_transmit_receive(dev_handle->real_handle, (uint8_t *)&reg_addr, reg_addr_len, data, data_len, I2C_MASTER_TIMEOUT_MS));
}