#ifndef DAIDA_COMPUTE_H
#define DAIDA_COMPUTE_H

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_err.h"

#include "daida_tinyml.h"

#define DAIDA_TINY_ML_SAMPLE_SIZE 16384 // numero de muestras que procesara el TinyML 32Kb

typedef struct {
    float angle; // Ángulo de evento DSP
    daida_audio_class_t audio_classification; // struct resultado de la clasificación de audio (a definirse en servicio tinyML)
} daida_compute_event_t;

esp_err_t daida_compute_init(QueueHandle_t core1_ui_queue);

#endif