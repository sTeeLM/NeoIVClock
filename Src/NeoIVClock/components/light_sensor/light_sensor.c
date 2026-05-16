#include "light_sensor.h"
#include "logger.h"


#include "esp_adc/adc_oneshot.h"

static const char * TAG = "LIGHT_SENSOR";

static adc_oneshot_unit_handle_t adc_handle;

void light_sensor_init(void)
{
    
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id  = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };    
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };

    NEO_LOGI(TAG, "init");

    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle)); 
    // ADC_CHANNEL_0 = GPIO1
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle,  ADC_CHANNEL_0, &config));   
}

int32_t light_sensor_read_data()
{
    int data;
    if(adc_oneshot_read(adc_handle, ADC_CHANNEL_0, &data) == ESP_OK) {
        NEO_LOGD(TAG, "light_sensor_read_data %d", data);
        return data;
    } else {
        NEO_LOGW(TAG, "adc_oneshot_read failed");
        return -1;
    }
}