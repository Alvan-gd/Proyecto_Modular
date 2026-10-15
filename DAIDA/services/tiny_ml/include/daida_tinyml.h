#ifndef DAIDA_TINYML_H
#define DAIDA_TINYML_H

#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief Diccionario de clases acústicas generadas por el modelo actual.
 *        Mapeado alfabéticamente según la exportación de Edge Impulse.
 */
typedef enum {
    DAIDA_CLASS_ALERTA_SISMICA = 0,
    DAIDA_CLASS_BACKGROUND_NOISE = 1,
    DAIDA_CLASS_CLAXON_AUTO = 2,
    DAIDA_CLASS_DISPAROS = 3,
    DAIDA_CLASS_DOG_BARK = 4,
    DAIDA_CLASS_NOMBRE_USUARIO = 5,
    DAIDA_CLASS_TIMBRE_CASA = 6,
    DAIDA_CLASS_UNKNOWN = 255
} daida_audio_class_t;


/**
 * @brief Inicializa configuraciones del modelo de Edge Impulse.
 */
esp_err_t daida_tinyml_init(void);


/**
 * @brief Ejecuta la inferencia sobre un buffer de muestras de audio.
 * 
 * @param[in]  audio_buffer   Puntero al inicio del buffer de audio (int16_t).
 * @param[in]  buffer_length  Debe coincidir con la ventana del modelo (16000).
 * @param[out] out_class_id   Retorna el índice de la clase con mayor probabilidad.
 * @param[out] out_confidence Retorna el valor de confianza [0.0 - 1.0].
 * 
 * @return ESP_OK si la clasificación fue exitosa.
 */
esp_err_t daida_tinyml_classify(int16_t *audio_buffer, size_t buffer_length, daida_audio_class_t *out_class_id, float *out_confidence);

#ifdef __cplusplus
}
#endif

#endif // DAIDA_TINYML_H