idf_component_register(SRCS "usart_wrapper.c"
                    INCLUDE_DIRS "include"
                    REQUIRES "esp_driver_uart"
                    REQUIRES "esp_driver_gpio"
                    REQUIRES "logger")
