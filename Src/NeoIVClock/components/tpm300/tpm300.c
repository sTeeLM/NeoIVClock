#include "tpm300.h"
#include "logger.h"
#include "i2c_wrapper.h"

static const char * TAG = "TPM300";

#define TMP300_I2C_ADDR 0x3B

static i2c_wrapper_dev_handle_t tpm300_i2c_dev_handle;

void tpm300_init(void)
{
    NEO_LOGI(TAG, "tmp300_init");
    i2c_wrapper_add_dev(TMP300_I2C_ADDR, 10000, &tpm300_i2c_dev_handle);
}

float tpm300_read_data(void)
{
    uint8_t data[2];
    float ret;
    if(i2c_wrapper_raw_read(&tpm300_i2c_dev_handle, data, 2)) {
        ret = (data[0] * 256 + data[1]) / 100.0f;
        NEO_LOGD(TAG, "tpm300_read_data %.4f mg/m3", ret);
    } else {
        ret = -1.0f;
        NEO_LOGW(TAG, "tpm300_read_data failed", ret);
        i2c_wrapper_bus_reset();
    }
    return ret;
}