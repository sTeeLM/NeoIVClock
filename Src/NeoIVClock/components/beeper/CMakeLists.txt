idf_component_register(SRCS "beeper.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES "gpio_wrapper"
                    PRIV_REQUIRES "esp_driver_ledc"
                    PRIV_REQUIRES "esp_driver_gptimer"
                    PRIV_REQUIRES "config"
                    PRIV_REQUIRES "logger")
