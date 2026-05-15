idf_component_register(
                    SRCS "sm.c" 
                    SRCS "state_machines/sm_clock.c"
                    SRCS "state_machines/sm_net_access.c" 
                    SRCS "state_machines/sm_sensor.c" 
                    SRCS "state_machines/sm_set_alarm.c"
                    SRCS "state_machines/sm_set_date.c"
                    SRCS "state_machines/sm_set_param.c"
                    SRCS "state_machines/sm_set_time.c"
                    SRCS "state_machines/sm_stop_watch.c"
                    SRCS "state_machines/sm_timer.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES "task"
                    PRIV_REQUIRES "logger")
