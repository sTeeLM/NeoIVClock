idf_component_register(SRCS "oled.c" "oled_font.c" 
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES "i2c_wrapper"
                    PRIV_REQUIRES "logger")
