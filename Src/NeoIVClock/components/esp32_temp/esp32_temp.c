#include "esp32_temp.h"
#include "logger.h"
#include "driver/temperature_sensor.h"

static const char * TAG = "ESP32_TEMP";

static temperature_sensor_handle_t temp_handle;

void esp32_temp_init(void)
{
  NEO_LOGI(TAG, "init");
  temperature_sensor_config_t temp_sensor_config = {
    .range_min = -10,
    .range_max = 80,
    .clk_src = TEMPERATURE_SENSOR_CLK_SRC_DEFAULT
  };
  ESP_ERROR_CHECK(temperature_sensor_install(&temp_sensor_config, &temp_handle));
  ESP_ERROR_CHECK(temperature_sensor_enable(temp_handle));
}

float esp32_temp_read_data(void)
{
  float tsens_out;
  ESP_ERROR_CHECK(temperature_sensor_get_celsius(temp_handle, &tsens_out));
  NEO_LOGD(TAG, "esp32_temp_read_data: %.2f C", tsens_out);
  return tsens_out;
}