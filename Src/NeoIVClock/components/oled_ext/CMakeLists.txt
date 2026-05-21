idf_component_register(SRCS "oled_ext.c" "oled_ext_font.c"
                    INCLUDE_DIRS "include"
                    REQUIRES "oled"
                    REQUIRES "cext"
                    REQUIRES "logger")
