#pragma once

#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"


/* Parámetros operativos base */
#define I2S_SAMPLE_RATE_HZ   16000
#define I2S_DMA_FRAME_NUM    512   /* Número de muestras por interrupción DMA */


esp_err_t drv_i2s_mic_init(void);
esp_err_t drv_i2s_mic_read(int32_t *buffer_out, size_t buffer_size, size_t *bytes_read, uint32_t timeout_ms);


