idf_component_register(SRCS "src/drv_i2s_mic.c"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_driver_i2s esp_driver_gpio)