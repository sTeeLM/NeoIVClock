idf_component_register(SRCS "i2c_wrapper.c"
                    INCLUDE_DIRS "include"
                    REQUIRES "logger"
                    REQUIRES "gpio_wrapper"
                    REQUIRES "esp_driver_i2c")
