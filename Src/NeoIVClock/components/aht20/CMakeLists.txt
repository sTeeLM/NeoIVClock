idf_component_register(SRCS "aht20.c"
                    INCLUDE_DIRS "include"
                    REQUIRES "i2c_wrapper"
                    REQUIRES "delay"
                    REQUIRES "cext"
                    REQUIRES "logger")
