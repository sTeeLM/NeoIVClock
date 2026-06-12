#include "ens160.h"
#include "logger.h"
#include "i2c_wrapper.h"
#include "delay.h"

#include <string.h>

static const char *TAG = "ENS160";
static i2c_wrapper_dev_handle_t ens160_i2c_handle;

#define ENS160_I2C_ADDR 0x53

static void ens160_set_mode(ens160_mode_t mode)
{
  uint8_t data = mode;
  i2c_wrapper_write(&ens160_i2c_handle, 0x10, I2C_ADDR_MODE_8BIT, &data, 1);
}

void ens160_set_config(const ens160_config_t * config)
{
  uint8_t data = 0;
  data |= (config->int_polarity_positive ? 0x40 : 0x00);
  data |= (config->int_drive_push_pull ? 0x20 : 0x00);
  data |= (config->int_gpr ? 0x08 : 0x00);
  data |= (config->int_data ? 0x02 : 0x00);
  data |= (config->int_enable ? 0x1 : 0x00);

  i2c_wrapper_write(&ens160_i2c_handle, 0x11, I2C_ADDR_MODE_8BIT, &data, 1);
}

void ens160_init(void)
{
  ens160_config_t cfg = {
    .int_polarity_positive = true,
    .int_drive_push_pull = true,
    .int_gpr = false,
    .int_data = false,
    .int_enable = false
  };
  uint8_t fw_major, fw_minor, fw_release;
  NEO_LOGI(TAG, "init");
  i2c_wrapper_add_dev(ENS160_I2C_ADDR, 400000, &ens160_i2c_handle);

  ens160_set_mode(ENS160_MODE_IDLE);

  ens160_get_fw_version(&fw_major, &fw_minor, &fw_release);
  NEO_LOGI(TAG, "FW version: %d.%d.%d", fw_major, fw_minor, fw_release);

  ens160_set_mode(ENS160_MODE_STANDARD);

  ens160_set_config(&cfg);
}

void ens160_clr_gpt(void)
{
  uint8_t cmd = 0xCC;
  i2c_wrapper_write(&ens160_i2c_handle, 0x12, I2C_ADDR_MODE_8BIT, &cmd, 1);
}

uint8_t ens160_get_status(void)
{
  uint8_t status = 0;
  i2c_wrapper_read(&ens160_i2c_handle, 0x20, I2C_ADDR_MODE_8BIT, &status, 1);
  return status;
}

void ens160_get_fw_version(uint8_t * major, uint8_t * minor, uint8_t * release)
{
  uint8_t cmd = 0x0E;
  uint8_t status = 0;
  uint8_t data[3] = {0};
  i2c_wrapper_write(&ens160_i2c_handle, 0x12, I2C_ADDR_MODE_8BIT, &cmd, 1);
  do{
    delay_ms(10);
    status = ens160_get_status();
  } while((status & ENS160_STATUS_NEWGPR_BIT) == 0); // 等待GPR寄存器准备好  
  i2c_wrapper_read(&ens160_i2c_handle, 0x4C, I2C_ADDR_MODE_8BIT, data, 3);
  *major = data[0];
  *minor = data[1];
  *release = data[2];
}

void ens160_compensate(float temp, float humidity)
{
  uint16_t temp_kelvin_64 = (uint16_t)((temp + 273.15f) * 64.0f);
  uint8_t temp_hi = (temp_kelvin_64 >> 8) & 0xFF;
  uint8_t temp_lo = temp_kelvin_64 & 0xFF;
  uint16_t humidity_512 = (uint16_t)(humidity * 512.0f);
  uint8_t humidity_hi = (humidity_512 >> 8) & 0xFF;
  uint8_t humidity_lo = humidity_512 & 0xFF;

  uint8_t data[4] = {temp_lo, temp_hi, humidity_lo, humidity_hi};

  i2c_wrapper_write(&ens160_i2c_handle, 0x13, I2C_ADDR_MODE_8BIT, data, 4);

  NEO_LOGD(TAG, "compensate temp %.2fC humidity %.2f%%", temp, humidity);
}

bool ens160_read_data(ens160_data_t * data)
{
  uint8_t buffer[2];
  memset(data, 0, sizeof(ens160_data_t));

  // DATA_AQI (Address 0x21)
  i2c_wrapper_read(&ens160_i2c_handle, 0x21, I2C_ADDR_MODE_8BIT, buffer, 1);
  data->iaq = buffer[0] & 0x7; 

  // DATA_TVOC (Address 0x22)
  i2c_wrapper_read(&ens160_i2c_handle, 0x22, I2C_ADDR_MODE_8BIT, buffer, 2);
  data->tvoc = (float)(buffer[0] | (buffer[1] << 8));

  // DATA_ECO2 (Address 0x24)
  i2c_wrapper_read(&ens160_i2c_handle, 0x24, I2C_ADDR_MODE_8BIT, buffer, 2);
  data->eco2 = (float)(buffer[0] | (buffer[1] << 8));

  NEO_LOGD(TAG, "read data iaq %d tvoc %.2f ppb eco2 %.2f ppm", data->iaq, data->tvoc, data->eco2);

  return true;
}


