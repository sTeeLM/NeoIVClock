idf_component_register(SRCS "alarm.c"
                    INCLUDE_DIRS "include"
                    REQUIRES "config"
                    REQUIRES "task"
                    REQUIRES "clock"
                    REQUIRES "logger")
