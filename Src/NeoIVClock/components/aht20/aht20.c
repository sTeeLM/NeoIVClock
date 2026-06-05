#include "aht20.h"
#include "delay.h"
#include "i2c_wrapper.h"
#include "logger.h"
#include "cext.h"

#include <string.h>

static const char *TAG = "AHT20";

#define AHT20_I2C_ADDR 0x38

#define AHT20_CMD_CALIBRATION 0xBE // 初始化（校准）命令
#define AHT20_CMD_CALIBRATION_ARG0 0x08
#define AHT20_CMD_CALIBRATION_ARG1 0x00

/**
 * 传感器在采集时需要时间,主机发出测量指令（0xAC）后，延时75毫秒以上再读取转换后的数据并判断返回的状态位是否正常。
 * 若状态比特位[Bit7]为0代表数据可正常读取，为1时传感器为忙状态，主机需要等待数据处理完成。
 **/
#define AHT20_CMD_TRIGGER 0xAC // 触发测量命令
#define AHT20_CMD_TRIGGER_ARG0 0x33
#define AHT20_CMD_TRIGGER_ARG1 0x00

// 用于在无需关闭和再次打开电源的情况下，重新启动传感器系统，软复位所需时间不超过20
// 毫秒
#define AHT20_CMD_RESET 0xBA // 软复位命令

#define AHT20_CMD_STATUS 0x71 // 获取状态命令

/**
 * STATUS 命令回复：
 * 1. 初始化后触发测量之前，STATUS 只回复 1B 状态值；
 * 2. 触发测量之后，STATUS 回复6B： 1B 状态值 + 2B 湿度 + 4b湿度 + 4b温度 + 2B
 *温度 RH = Srh / 2^20 * 100% T  = St  / 2^20 * 200 - 50
 **/
#define AHT20_STATUS_BUSY_SHIFT 7 // bit[7] Busy indication
#define AHT20_STATUS_BUSY_MASK (0x1 << AHT20_STATUS_BUSY_SHIFT)
#define AHT20_STATUS_BUSY(status)                                              \
  ((status & AHT20_STATUS_BUSY_MASK) >> AHT20_STATUS_BUSY_SHIFT)

#define AHT20_STATUS_MODE_SHIFT 5 // bit[6:5] Mode Status
#define AHT20_STATUS_MODE_MASK (0x3 << AHT20_STATUS_MODE_SHIFT)
#define AHT20_STATUS_MODE(status)                                              \
  ((status & AHT20_STATUS_MODE_MASK) >> AHT20_STATUS_MODE_SHIFT)

// bit[4] Reserved
#define AHT20_STATUS_CALI_SHIFT 3 // bit[3] CAL Enable
#define AHT20_STATUS_CALI_MASK (0x1 << AHT20_STATUS_CALI_SHIFT)
#define AHT20_STATUS_CALI(status)                                              \
  ((status & AHT20_STATUS_CALI_MASK) >> AHT20_STATUS_CALI_SHIFT)
// bit[2:0] Reserved

#define AHT20_STATUS_RESPONSE_MAX 7

#define AHT20_RESOLUTION (1 << 20) // 2^20

#define AHT20_MAX_RETRY 10

#define AHT20_STARTUP_TIME_MS 40

#define AHT20_MEASURE_TIME_MS 75

static i2c_wrapper_dev_handle_t aht20_i2c_dev_handle;

// 发送软复位命令
static bool aht20_reset_cmd(void) 
{
  const uint8_t cmd[] = {AHT20_CMD_RESET};
  return i2c_wrapper_raw_write(&aht20_i2c_dev_handle, cmd, 1);
}

// 发送获取状态命令
static bool aht20_status_cmd(void) 
{
  uint8_t cmd[] = {AHT20_CMD_STATUS};
  return i2c_wrapper_raw_write(&aht20_i2c_dev_handle, cmd, 1);
}

// 发送初始化校准命令
static bool aht20_calibrate_cmd(void) 
{
  uint8_t cmd[] = {AHT20_CMD_CALIBRATION, AHT20_CMD_CALIBRATION_ARG0,
                   AHT20_CMD_CALIBRATION_ARG1};
  return i2c_wrapper_raw_write(&aht20_i2c_dev_handle, cmd, 3);
}

// 读取温湿度值之前， 首先要看状态字的校准使能位Bit[3]是否为
// 1(通过发送0x71可以获取一个字节的状态字)，
// 如果不为1，要发送0xBE命令(初始化)，此命令参数有两个字节，
// 第一个字节为0x08，第二个字节为0x00。
static bool aht20_calibrate(void) 
{
  bool retval = false;
  uint8_t buffer[AHT20_STATUS_RESPONSE_MAX] = {};

  if (!aht20_status_cmd()) {
    return retval;
  }

  if (!i2c_wrapper_raw_read(&aht20_i2c_dev_handle, buffer, sizeof(buffer))) {
    return retval;
  }

  if (AHT20_STATUS_BUSY(buffer[0]) || !AHT20_STATUS_CALI(buffer[0])) {
    if (!aht20_reset_cmd()) {
      return retval;
    }

    delay_ms(AHT20_STARTUP_TIME_MS);
    retval = aht20_calibrate_cmd();
    delay_ms(AHT20_STARTUP_TIME_MS);
    return retval;
  }

  return true;
}

// 发送 触发测量 命令，开始测量
static bool aht20_start_measure(void) 
{
  uint8_t cmd[] = {AHT20_CMD_TRIGGER, AHT20_CMD_TRIGGER_ARG0,
                   AHT20_CMD_TRIGGER_ARG1};
  return i2c_wrapper_raw_write(&aht20_i2c_dev_handle, cmd, 3);
}

static bool aht20_get_measure_result(float * temp, float * mol) 
{
  uint8_t buffer[AHT20_STATUS_RESPONSE_MAX] = {};
  uint8_t retry_cnt = 0;
  uint32_t mol_raw;
  int32_t temp_raw;

  do {
    if (!i2c_wrapper_raw_read(&aht20_i2c_dev_handle, buffer, sizeof(buffer))) {
      return false;
    }
    if(AHT20_STATUS_BUSY(buffer[0])) {
      delay_ms(AHT20_MEASURE_TIME_MS);
    }
    retry_cnt ++;
  } while(AHT20_STATUS_BUSY(buffer[0]) && retry_cnt < AHT20_MAX_RETRY);

  if(AHT20_STATUS_BUSY(buffer[0])) {
    NEO_LOGW(TAG, "aht20_get_measure_result: not response!");
    return false;
  }

  if(mol) {
    mol_raw = buffer[1];
    mol_raw = (mol_raw << 8) | buffer[2];
    mol_raw = (mol_raw << 4) | ((buffer[3] & 0xF0) >> 4);
    * mol = mol_raw / (float)AHT20_RESOLUTION * 100;
  }

  if(temp) {
    temp_raw = buffer[3] & 0x0F;
    temp_raw = (temp_raw << 8) | buffer[4];
    temp_raw = (temp_raw << 8) | buffer[5];
    *temp = temp_raw / (float)AHT20_RESOLUTION * 200 - 50;
  }

  return true;
}

void aht20_init(void) 
{
  NEO_LOGI(TAG, "init");
  i2c_wrapper_add_dev(AHT20_I2C_ADDR, 100000, &aht20_i2c_dev_handle);
  delay_ms(100);
  // 1.上电后要等待不少于100ms，读取温湿度值之前，通过发送0x71获取一个字节的状态字，如果状态字和0x18相与后不等于0x18，初始化0x1B、0x1C、0x1E寄存器，
  if(!aht20_calibrate()) {
    NEO_LOGE(TAG, "aht20_calibrate: failed!");
  }
}

float aht20_read_data(float * mol) 
{
  float res_temp = 0.0f, res_mol=0.0f;
  if(aht20_start_measure()) {
    delay_ms(AHT20_MEASURE_TIME_MS);
    if(aht20_get_measure_result(&res_temp, &res_mol)) {
      if(mol) {
        *mol = res_mol;
      }
      NEO_LOGD(TAG, "aht20_read_data  %f Celsius (%f F) | mol %f%%", 
        res_temp, cext_celsius_to_fahrenheit(res_temp), res_mol);

      return res_temp;
    }
  }
  return 0.0f;
}