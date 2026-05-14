idf_component_register(SRCS "tpm300.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES "i2c_wrapper"
                    PRIV_REQUIRES "logger")
