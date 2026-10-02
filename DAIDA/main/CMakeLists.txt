idf_component_register(SRCS "main.c"
                       INCLUDE_DIRS "."
                       REQUIRES drv_i2s_mic dsp esp_timer)