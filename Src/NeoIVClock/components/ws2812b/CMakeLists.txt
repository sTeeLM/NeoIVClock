idf_component_register(SRCS "ws2812b.c"
                    INCLUDE_DIRS "include"
                    REQUIRES "led_strip"
                    REQUIRES "gpio_wrapper"
                    REQUIRES "delay"
                    REQUIRES "logger")