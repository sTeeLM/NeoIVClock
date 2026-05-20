idf_component_register(SRCS "oled.c" "oled_font.c" 
                    INCLUDE_DIRS "include"
                    REQUIRES "i2c_wrapper"
                    REQUIRES "logger")
