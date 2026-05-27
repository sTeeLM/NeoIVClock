idf_component_register(SRCS "timer.c"
                    INCLUDE_DIRS "include"
                    REQUIRES "player"
                    REQUIRES "config"
                    REQUIRES "task"
                    REQUIRES "iv18"
                    REQUIRES "logger")
