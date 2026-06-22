idf_component_register(SRCS "nvs_wrapper.c"
                    INCLUDE_DIRS "include"
                    REQUIRES "nvs_flash" 
                    REQUIRES "logger")