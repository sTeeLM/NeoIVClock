idf_component_register(SRCS "ens160.c"
                    INCLUDE_DIRS "include"
                    REQUIRES "i2c_wrapper"
                    REQUIRES "delay"
                    REQUIRES "logger")
