#include "bmp280.h"
#include "logger.h"
#include "i2c_wrapper.h"
#include "delay.h"
#include "cext.h"

#include <stdint.h>
#include <stdbool.h>
#include <math.h>

// Assuming I2C interface is available via system functions
// Include necessary headers for I2C, e.g., #include "i2c.h" if available

static const char * TAG = "BMP280";
static i2c_wrapper_dev_handle_t bmp280_i2c_dev_handle;

#define BMP280_I2C_ADDR 0x77  // Default I2C address, change if needed

// Registers
#define BMP280_REG_ID         0xD0
#define BMP280_REG_RESET      0xE0
#define BMP280_REG_STATUS     0xF3
#define BMP280_REG_CTRL_MEAS  0xF4
#define BMP280_REG_CONFIG     0xF5
#define BMP280_REG_PRESS_MSB  0xF7
#define BMP280_REG_PRESS_LSB  0xF8
#define BMP280_REG_PRESS_XLSB 0xF9
#define BMP280_REG_TEMP_MSB  0xFA
#define BMP280_REG_TEMP_LSB  0xFB
#define BMP280_REG_TEMP_XLSB 0xFC

// Calibration data registers
#define BMP280_REG_DIG_T1_LSB 0x88
#define BMP280_REG_DIG_T1_MSB 0x89
#define BMP280_REG_DIG_T2_LSB 0x8A
#define BMP280_REG_DIG_T2_MSB 0x8B
#define BMP280_REG_DIG_T3_LSB 0x8C
#define BMP280_REG_DIG_T3_MSB 0x8D
#define BMP280_REG_DIG_P1_LSB 0x8E
#define BMP280_REG_DIG_P1_MSB 0x8F
#define BMP280_REG_DIG_P2_LSB 0x90
#define BMP280_REG_DIG_P2_MSB 0x91
#define BMP280_REG_DIG_P3_LSB 0x92
#define BMP280_REG_DIG_P3_MSB 0x93
#define BMP280_REG_DIG_P4_LSB 0x94
#define BMP280_REG_DIG_P4_MSB 0x95
#define BMP280_REG_DIG_P5_LSB 0x96
#define BMP280_REG_DIG_P5_MSB 0x97
#define BMP280_REG_DIG_P6_LSB 0x98
#define BMP280_REG_DIG_P6_MSB 0x99
#define BMP280_REG_DIG_P7_LSB 0x9A
#define BMP280_REG_DIG_P7_MSB 0x9B
#define BMP280_REG_DIG_P8_LSB 0x9C
#define BMP280_REG_DIG_P8_MSB 0x9D
#define BMP280_REG_DIG_P9_LSB 0x9E
#define BMP280_REG_DIG_P9_MSB 0x9F

// Values
#define BMP280_ID 0x58
#define BMP280_RESET_VALUE 0xB6

// Oversampling and mode
#define BMP280_OSRS_P   BMP280_MODE_NORMAL  // x16
#define BMP280_OSRS_T   BMP280_MODE_NORMAL  // x16
#define BMP280_IIR_MODE BMP280_IIR_16 // IIR filter coefficient 16
#define BMP280_MODE     BMP280_MODE_NORMAL  // Normal mode


typedef struct _bmp280_calib_data_t {
    uint16_t dig_T1;
    int16_t dig_T2;
    int16_t dig_T3;
    uint16_t dig_P1;
    int16_t dig_P2;
    int16_t dig_P3;
    int16_t dig_P4;
    int16_t dig_P5;
    int16_t dig_P6;
    int16_t dig_P7;
    int16_t dig_P8;
    int16_t dig_P9;
} bmp280_calib_data_t ;

static bmp280_calib_data_t bmp_280_calib_data;
static int32_t bmp280_t_fine;

// Helper functions for I2C (assuming system provides i2c_write and i2c_read)
// You may need to adapt these to your I2C library

static void bmp280_write_reg(uint8_t reg, uint8_t value) 
{
    i2c_wrapper_write(&bmp280_i2c_dev_handle, reg, I2C_ADDR_MODE_8BIT, &value, 1);
}

static void bmp280_read_reg(uint8_t reg, uint8_t *data, size_t len) {
    i2c_wrapper_read(&bmp280_i2c_dev_handle, reg, I2C_ADDR_MODE_8BIT, data, len);
}

static void bmp280_read_calib_data(void) 
{
    uint8_t data[24];
    bmp280_read_reg(BMP280_REG_DIG_T1_LSB, data, 24);
    bmp_280_calib_data.dig_T1 = (data[1] << 8) | data[0];
    bmp_280_calib_data.dig_T2 = (data[3] << 8) | data[2];
    bmp_280_calib_data.dig_T3 = (data[5] << 8) | data[4];
    bmp_280_calib_data.dig_P1 = (data[7] << 8) | data[6];
    bmp_280_calib_data.dig_P2 = (data[9] << 8) | data[8];
    bmp_280_calib_data.dig_P3 = (data[11] << 8) | data[10];
    bmp_280_calib_data.dig_P4 = (data[13] << 8) | data[12];
    bmp_280_calib_data.dig_P5 = (data[15] << 8) | data[14];
    bmp_280_calib_data.dig_P6 = (data[17] << 8) | data[16];
    bmp_280_calib_data.dig_P7 = (data[19] << 8) | data[18];
    bmp_280_calib_data.dig_P8 = (data[21] << 8) | data[20];
    bmp_280_calib_data.dig_P9 = (data[23] << 8) | data[22];
}

// Compensation formulas from BMP280 datasheet
static int32_t bmp280_compensate_T_int32(int32_t adc_T)
{
    int32_t var1, var2, T;
    var1 = ((((adc_T >> 3) - ((int32_t)bmp_280_calib_data.dig_T1 << 1))) * ((int32_t)bmp_280_calib_data.dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)bmp_280_calib_data.dig_T1)) * ((adc_T >> 4) - ((int32_t)bmp_280_calib_data.dig_T1))) >> 12) * ((int32_t)bmp_280_calib_data.dig_T3)) >> 14;
    bmp280_t_fine = var1 + var2;
    T = (bmp280_t_fine * 5 + 128) >> 8;
    return T;
}

// Returns pressure in Pa, must be called after bmp280_compensate_T_int32 to get correct bmp280_t_fine
static uint32_t bmp280_compensate_P_int64(int32_t adc_P) 
{
    int64_t var1, var2, p;
    var1 = ((int64_t)bmp280_t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)bmp_280_calib_data.dig_P6;
    var2 = var2 + ((var1 * (int64_t)bmp_280_calib_data.dig_P5) << 17);
    var2 = var2 + (((int64_t)bmp_280_calib_data.dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)bmp_280_calib_data.dig_P3) >> 8) + ((var1 * (int64_t)bmp_280_calib_data.dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)bmp_280_calib_data.dig_P1) >> 33;
    if (var1 == 0) {
        return 0;  // avoid exception caused by division by zero
    }
    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)bmp_280_calib_data.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)bmp_280_calib_data.dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)bmp_280_calib_data.dig_P7) << 4);
    return (uint32_t)p;
}

bool bmp280_test_status(uint8_t mask)
{
    uint8_t status;
    bmp280_read_reg(BMP280_REG_STATUS, &status, 1);
    return (status & mask) != 0;
} 

void bmp280_set_standby_time(bmp280_standby_time_t t_sb) 
{
    uint8_t config;
    bmp280_read_reg(BMP280_REG_CONFIG, &config, 1);
    config &= ~0xE0;
    config |= (t_sb << 5);
    bmp280_write_reg(BMP280_REG_CONFIG, config);
}

void bmp280_reset(void) 
{
    bmp280_write_reg(BMP280_REG_RESET, BMP280_RESET_VALUE);
}

void bmp280_set_mode(bmp280_mode_t mode) 
{
    uint8_t ctrl_meas;
    bmp280_read_reg(BMP280_REG_CTRL_MEAS, &ctrl_meas, 1 );
    ctrl_meas &= ~0x3;
    ctrl_meas |= mode;
    bmp280_write_reg(BMP280_REG_CTRL_MEAS, ctrl_meas);
}

void bmp280_set_oversampling(bmp280_oversampling_t osrs_t, bmp280_oversampling_t osrs_p) 
{
    uint8_t ctrl_meas;
    
    if(osrs_t == BMP280_OSRS_SKIP || osrs_p == BMP280_OSRS_SKIP) {
        NEO_LOGE(TAG, "Oversampling skip mode not supported");
        return; // Skip mode not supported for oversampling
    }
    bmp280_read_reg(BMP280_REG_CTRL_MEAS, &ctrl_meas, 1);
    ctrl_meas &= ~0xFC;
    ctrl_meas |= (osrs_t << 5) | (osrs_p << 2);
    bmp280_write_reg(BMP280_REG_CTRL_MEAS, ctrl_meas);
}

void bmp280_set_rii_filter(bmp280_iir_coefficient_t iir_filter) 
{
    uint8_t config;
    bmp280_read_reg(BMP280_REG_CONFIG, &config, 1);
    config &= ~0x1C;
    config |= (iir_filter << 2);
    bmp280_write_reg(BMP280_REG_CONFIG, config);
}

void bmp280_init(void) 
{

    uint8_t id;

    NEO_LOGD(TAG, "bmp280_init");

    i2c_wrapper_add_dev(BMP280_I2C_ADDR, 100000, &bmp280_i2c_dev_handle);
    
    bmp280_read_reg(BMP280_REG_ID, &id, 1);
    if (id != BMP280_ID) {
        NEO_LOGD(TAG, "BMP280 not found or wrong ID");
        return;
    }
    bmp280_read_calib_data();

    // Reset
    bmp280_reset();
    // Wait for reset
    delay_ms(100);
    // Set config
    bmp280_write_reg(BMP280_REG_CONFIG, 0x00);  // Standby 0.5ms
    // Set mode and oversampling
    bmp280_set_mode(BMP280_MODE);
    bmp280_set_oversampling(BMP280_OSRS_T, BMP280_OSRS_P);
    bmp280_set_rii_filter(BMP280_IIR_MODE);

    // standby time 125ms
    bmp280_set_standby_time(BMP280_STANDBY_125_MS);
}

float bmp280_read_data(float * temperature) 
{
    uint8_t data[6];
    int32_t adc_P, adc_T, press, temp;
    float press_f, temp_f;

    bmp280_read_reg(BMP280_REG_PRESS_MSB, data, 6);
    adc_T = (data[3] << 12) | (data[4] << 4) | (data[5] >> 4);
    adc_P = (data[0] << 12) | (data[1] << 4) | (data[2] >> 4);
    temp = bmp280_compensate_T_int32(adc_T);
    press = bmp280_compensate_P_int64(adc_P);

    temp_f = (float)temp / 100.0f;  // Celsius
    press_f = (float)press / 256.0f;  // Pa

    NEO_LOGD(TAG, "bmp280_read_data %f Pa (%f mmHg  / %f atm) | %f Celsius (%f F)", 
        press_f, cext_pa_to_mmhg(press_f), cext_pa_to_atm(press_f), temp_f, cext_celsius_to_fahrenheit(temp_f));
    if(temperature) {
        *temperature = temp_f;
    }
    return press_f;  // Pa
}
