#pragma once

#include "esp_err.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define DMA_FRAME_NUM 256
#define MIC_CHANNELS 4

#define DSP_ENERGY_THRESHOLD 800.0f // Umbral de energía para considerar un evento de sonido válido

typedef struct {
    bool is_valid; // Indica si el resultado es válido
    float angle; // Ángulo estimado de la fuente de sonido en grados
} dsp_event_t;

esp_err_t dsp_pipeline_init(void);
esp_err_t dsp_pipeline_process(const int32_t *buffer, dsp_event_t *event_out, int16_t mic_mono_out[DMA_FRAME_NUM]);

