idf_component_register(SRCS "oled_ext.c" "oled_ext_icon.c"
                    INCLUDE_DIRS "include"
                    REQUIRES "oled"
                    REQUIRES "cext"
                    REQUIRES "mini_font"
                    REQUIRES "logger")
