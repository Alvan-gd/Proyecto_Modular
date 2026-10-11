idf_component_register(SRCS "src/daida_compute.c"
                       PRIV_REQUIRES spi_flash freertos
                       INCLUDE_DIRS "include"
                       REQUIRES dsp drv_i2s_mic)