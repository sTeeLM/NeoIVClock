idf_component_register(SRCS "dpf_player.c"
                    INCLUDE_DIRS "include"
                    REQUIRES "usart_wrapper"
                    REQUIRES "gpio_wrapper"
                    REQUIRES "delay"
                    REQUIRES "logger")
