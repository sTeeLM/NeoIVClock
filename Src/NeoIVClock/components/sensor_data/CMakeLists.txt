idf_component_register(SRCS "sensor_data.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES "bmp280"
                    PRIV_REQUIRES "pms5003st"
                    PRIV_REQUIRES "tpm300"
                    PRIV_REQUIRES "logger")
