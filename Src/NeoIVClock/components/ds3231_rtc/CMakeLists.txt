idf_component_register(SRCS "ds3231_rtc.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES "logger"
                    PRIV_REQUIRES "i2c_wrapper")
