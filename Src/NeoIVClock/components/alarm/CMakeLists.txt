idf_component_register(SRCS "alarm.c"
                    INCLUDE_DIRS "include"
                    REQUIRES "config"
                    REQUIRES "task"
                    REQUIRES "sm"
                    REQUIRES "clock"
                    REQUIRES "player"
                    REQUIRES "logger")
