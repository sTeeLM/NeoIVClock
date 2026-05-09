idf_component_register(SRCS "motion_sensor.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES "gpio_wrapper"
                    PRIV_REQUIRES "config"
                    PRIV_REQUIRES "logger")
