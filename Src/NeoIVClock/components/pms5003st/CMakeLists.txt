idf_component_register(SRCS "pms5003st.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES "usart_wrapper"
                    PRIV_REQUIRES "gpio_wrapper"
                    PRIV_REQUIRES "delay"
                    PRIV_REQUIRES "logger")
