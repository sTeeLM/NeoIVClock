idf_component_register(SRCS "ds3231_rtc.c"
                    INCLUDE_DIRS "include"
                    REQUIRES "logger"
                    REQUIRES "i2c_wrapper"
                    REQUIRES "cext")
