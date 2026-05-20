idf_component_register(SRCS "rom.c"
                    INCLUDE_DIRS "include"
                    REQUIRES "i2c_wrapper"
                    REQUIRES "delay"
                    REQUIRES "logger")
