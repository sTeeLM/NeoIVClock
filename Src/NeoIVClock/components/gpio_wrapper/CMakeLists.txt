idf_component_register(SRCS "gpio_wrapper.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES "logger"
                    PRIV_REQUIRES "esp_driver_gpio"
                    )
