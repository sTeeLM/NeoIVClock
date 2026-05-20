idf_component_register(SRCS "tpm300.c"
                    INCLUDE_DIRS "include"
                    REQUIRES "i2c_wrapper"
                    REQUIRES "logger")
