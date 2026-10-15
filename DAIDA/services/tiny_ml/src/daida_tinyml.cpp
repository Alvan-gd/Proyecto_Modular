#include "daida_tinyml.h"
#include "esp_log.h"

// Cabeceras de Edge Impulse
#include "edge-impulse-sdk/classifier/ei_run_classifier.h"

static const char *TAG = "DAIDA_TINYML";

// Puntero estático para que el callback de Edge Impulse sepa de dónde leer
static int16_t *s_current_audio_buffer = nullptr;

/**
 * @brief Callback exigido por Edge Impulse para extraer el audio bajo demanda.
 * Convierte el audio crudo int16_t del micrófono INMP441 a float.
 */
static int microphone_audio_signal_get_data(size_t offset, size_t length, float *out_ptr) {
    if (s_current_audio_buffer == nullptr) {
        return -1;
    }
    for (size_t i = 0; i < length; i++) {
        out_ptr[i] = (float)s_current_audio_buffer[offset + i];
    }
    return 0; // 0 significa éxito para Edge Impulse
}

esp_err_t daida_tinyml_init(void) {
    ESP_LOGI(TAG, "Edge Impulse Model: %s", EI_CLASSIFIER_PROJECT_NAME);
    ESP_LOGI(TAG, "Window size: %d ms", EI_CLASSIFIER_RAW_SAMPLE_COUNT / (EI_CLASSIFIER_FREQUENCY / 1000));
    return ESP_OK;
}

esp_err_t daida_tinyml_classify(int16_t *audio_buffer, size_t buffer_length, daida_audio_class_t *out_class_id, float *out_confidence) {
    if (buffer_length != EI_CLASSIFIER_RAW_SAMPLE_COUNT) {
        ESP_LOGE(TAG, "Tamano de buffer incorrecto. Esperado %d, recibido %zu", EI_CLASSIFIER_RAW_SAMPLE_COUNT, buffer_length);
        return ESP_ERR_INVALID_ARG;
    }

    // Configurar el puntero y la estructura de señal de Edge Impulse
    s_current_audio_buffer = audio_buffer;
    signal_t signal;
    signal.total_length = EI_CLASSIFIER_RAW_SAMPLE_COUNT;
    signal.get_data = &microphone_audio_signal_get_data;

    // Ejecutar la inferencia
    ei_impulse_result_t result = { 0 };
    EI_IMPULSE_ERROR res = run_classifier(&signal, &result, false);
    
    if (res != EI_IMPULSE_OK) {
        ESP_LOGE(TAG, "Fallo run_classifier (%d)", res);
        return ESP_FAIL;
    }

    // Buscar la clase ganadora
    float max_confidence = 0.0f;
    uint8_t best_class_id = 0;

    for (uint16_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        if (result.classification[i].value > max_confidence) {
            max_confidence = result.classification[i].value;
            best_class_id = i;
        }
    }

    // Retornar los resultados al hilo de FreeRTOS
    *out_class_id = (daida_audio_class_t)best_class_id;
    *out_confidence = max_confidence;

    return ESP_OK;
}