idf_component_register(SRCS "main.c"
                       INCLUDE_DIRS "."
                       REQUIRES dsp esp_timer drv_i2s_mic)