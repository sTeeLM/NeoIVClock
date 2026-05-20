idf_component_register(SRCS "sensor_data.c"
                    INCLUDE_DIRS "include"
                    REQUIRES "bmp280"
                    REQUIRES "pms5003st"
                    REQUIRES "tpm300"
                    REQUIRES "cext"
                    REQUIRES "logger")
