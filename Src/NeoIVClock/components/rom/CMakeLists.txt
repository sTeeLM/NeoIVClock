idf_component_register(SRCS "rom.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES "i2c_wrapper"
                    PRIV_REQUIRES "delay"
                    PRIV_REQUIRES "logger")
