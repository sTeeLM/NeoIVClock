idf_component_register(SRCS "ec11.c"
                    INCLUDE_DIRS "include"
                    REQUIRES "gpio_wrapper"
                    REQUIRES "task"
                    REQUIRES "clock"
                    REQUIRES "sm"
                    REQUIRES "beeper"
                    REQUIRES "logger")
