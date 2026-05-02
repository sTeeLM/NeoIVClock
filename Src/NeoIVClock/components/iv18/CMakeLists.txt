idf_component_register(SRCS "iv18.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES "logger"
                    PRIV_REQUIRES "gpio_wrapper"
                    PRIV_REQUIRES "esp_driver_ledc"
                    PRIV_REQUIRES "delay")
