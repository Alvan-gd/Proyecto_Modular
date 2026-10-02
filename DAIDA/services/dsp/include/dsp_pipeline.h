#pragma once

#include "esp_err.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define DMA_FRAME_NUM 512
#define MIC_CHANNELS 4

#define DSP_ENERGY_THRESHOLD 800.0f // Umbral de energía para considerar un evento de sonido válido

// Definición de sectores en grados para la localización de la fuente de sonido
typedef enum {
    SECTOR_0 = 0,
    SECTOR_45 = 1,
    SECTOR_90 = 2,
    SECTOR_135 = 3,
    SECTOR_180 = 4,
    SECTOR_225 = 5,
    SECTOR_270 = 6,
    SECTOR_315 = 7,
    SECTOR_NONE = 8
} tdoa_sector_t;

typedef struct {
    bool is_valid; // Indica si el resultado es válido
    float angle; // Ángulo estimado de la fuente de sonido en grados
    tdoa_sector_t sector; // Sector correspondiente al ángulo estimado
} dsp_event_t;

esp_err_t dsp_pipeline_init(void);
esp_err_t dsp_pipeline_process(const int32_t *buffer, dsp_event_t *event_out, int16_t mic_mono_out[DMA_FRAME_NUM]);

