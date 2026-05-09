#ifndef NEO_IV_CLOCK_BMP280_H
#define NEO_IV_CLOCK_BMP280_H

#include <stdint.h>
#include <stdbool.h>

typedef enum _bmp280_mode_t
{
    BMP280_MODE_SLEEP  = 0x00,
    BMP280_MODE_FORCED = 0x01,
    BMP280_MODE_NORMAL = 0x03
} bmp280_mode_t;

typedef enum _bmp280_oversampling_t
{
    BMP280_OSRS_SKIP = 0x00,
    BMP280_OSRS_1    = 0x01,
    BMP280_OSRS_2    = 0x02,
    BMP280_OSRS_4    = 0x03,
    BMP280_OSRS_8    = 0x04,
    BMP280_OSRS_16   = 0x05
} bmp280_oversampling_t;

typedef enum _bmp280_iir_coefficient_t
{
    BMP280_IIR_OFF = 0x00,
    BMP280_IIR_2   = 0x01,
    BMP280_IIR_4   = 0x02,
    BMP280_IIR_8   = 0x03,
    BMP280_IIR_16  = 0x04
} bmp280_iir_coefficient_t;

typedef enum _bmp280_standby_time_t
{
    BMP280_STANDBY_0_5_MS = 0x00,
    BMP280_STANDBY_62_5_MS = 0x01,
    BMP280_STANDBY_125_MS = 0x02,
    BMP280_STANDBY_250_MS = 0x03,
    BMP280_STANDBY_500_MS = 0x04,
    BMP280_STANDBY_1000_MS = 0x05,
    BMP280_STANDBY_2000_MS = 0x06,
    BMP280_STANDBY_4000_MS = 0x07
} bmp280_standby_time_t;

void bmp280_init(void);
float bmp280_read_data(float * temperature);
void bmp280_set_mode(bmp280_mode_t mode);
void bmp280_set_oversampling(bmp280_oversampling_t osrs_t, bmp280_oversampling_t osrs_p);
void bmp280_set_rii_filter(bmp280_iir_coefficient_t iir_filter);
void bmp280_reset(void);
void bmp280_set_standby_time(bmp280_standby_time_t t_sb);

#define BMP280_STATUS_MEASURING 0x08
#define BMP280_STATUS_IM_UPDATE 0x01
bool bmp280_test_status(uint8_t mask);

#endif // NEO_IV_CLOCK_BMP280_H 
