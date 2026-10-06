#pragma once

#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"

/* Configuraciones Físicas y de Señal */
#define TDOA_FFT_N          256
#define TDOA_COMPLEX_SIZE   (TDOA_FFT_N * 2)
#define I2S_STRIDE          4
#define SAMPLE_RATE_HZ      16000.0f
#define SOUND_SPEED_M_S     343.0f
#define MIC_DISTANCE_M      0.035f  /* 3.5 cm */

#define TDOA_CH_X_POS   0
#define TDOA_CH_X_NEG   1
#define TDOA_CH_Y_POS   2
#define TDOA_CH_Y_NEG   3



/* ###################################################################################
                                    FUNCIONES
    ################################################################################### */

esp_err_t tdoa_init(void);
esp_err_t tdoa_process(const int32_t *buffer, float *angle_out);



