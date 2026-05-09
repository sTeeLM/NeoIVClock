idf_component_register(SRCS "bmp280.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES "i2c_wrapper"
                    PRIV_REQUIRES "delay"
                    PRIV_REQUIRES "cext"
                    PRIV_REQUIRES "logger")
