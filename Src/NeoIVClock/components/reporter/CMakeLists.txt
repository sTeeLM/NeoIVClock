idf_component_register(SRCS "reporter.c"
                    INCLUDE_DIRS "include"
                    REQUIRES "config"
                    REQUIRES "sensor_data"
                    REQUIRES "nm"
                    REQUIRES "cjson_wrapper"
                    REQUIRES "logger")
