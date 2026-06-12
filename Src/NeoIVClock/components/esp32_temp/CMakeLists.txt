idf_component_register(SRCS "esp32_temp.c"
                    INCLUDE_DIRS "include"
                    REQUIRES "logger"
                    REQUIRES "esp_driver_tsens")
