idf_component_register(SRCS "beeper.c"
                    INCLUDE_DIRS "include"
                    REQUIRES "gpio_wrapper"
                    REQUIRES "esp_driver_ledc"
                    REQUIRES "esp_driver_gptimer"
                    REQUIRES "config"
                    REQUIRES "delay"
                    REQUIRES "logger")
