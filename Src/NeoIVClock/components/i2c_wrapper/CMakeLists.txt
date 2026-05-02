idf_component_register(SRCS "i2c_wrapper.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES "logger"
                    PRIV_REQUIRES "gpio_wrapper"
                    REQUIRES "esp_driver_i2c")
