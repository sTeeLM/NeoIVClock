idf_component_register(SRCS "light_sensor.c"
                    INCLUDE_DIRS "include"
                    REQUIRES "esp_adc"
                    REQUIRES "cext"
                    REQUIRES "logger")
