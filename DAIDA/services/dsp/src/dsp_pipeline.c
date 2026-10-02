#include "dsp_pipeline.h"
#include "tdoa.h"

#include <math.h>
#include "esp_log.h"

static const char *TAG = "DSP_PIPELINE"; // Tag usado para imprimir mensajes del log

static bool is_initialized = false;


/**
 * @brief Initialize the DSP pipeline, including the TDOA processing module.
 * 
 * This function initializes the DSP pipeline and prepares it for processing audio data. It must be called before any calls to dsp_pipeline_process().
 * @return esp_err_t Returns ESP_OK on successful initialization, or an error code if initialization fails.
 */
esp_err_t dsp_pipeline_init(void) {
    if (is_initialized) {
        ESP_LOGI(TAG, "DSP Pipeline ya ha sido inicializado");
        return ESP_OK;
    }

    esp_err_t err = tdoa_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error al inicializar TDOA: %s", esp_err_to_name(err));
        return err;
    }

    is_initialized = true;
    ESP_LOGI(TAG, "DSP Pipeline inicializado correctamente");
    return ESP_OK;
}


/**
 * @brief Process audio data through the DSP pipeline, including TDOA processing.
 * 
 * This function processes the input audio buffer, calculates the energy of the signal, and performs TDOA processing to estimate the angle and sector of the sound source. It also converts the multi-channel audio data to mono for further processing.
 * @param buffer Pointer to the input buffer containing interleaved microphone samples.
 * @param event_out Pointer to a dsp_event_t structure where the results of the TDOA processing will be stored.
 * @param mic_mono_out Array where the mono audio data will be stored after processing. The size of this array must be DMA_FRAME_NUM.
 * 
 * @return esp_err_t Returns ESP_OK on success, ESP_ERR_INVALID_STATE if the DSP pipeline is not initialized, ESP_ERR_INVALID_ARG if input pointers are NULL, or other error codes from TDOA processing.
 */
esp_err_t dsp_pipeline_process(const int32_t *buffer, dsp_event_t *event_out, int16_t mic_mono_out[DMA_FRAME_NUM]) {

    // // DUMP DE DEPURACIÓN: Inspección de los primeros 4 frames de audio crudo
    // static int dump_count = 0;
    // if (dump_count < 3) {
    //     ESP_LOGW("RAW_DUMP", "--- MUESTRAS CRUDAS (CH0, CH1, CH2, CH3) ---");
    //     for (int f = 0; f < 4; f++) {
    //         ESP_LOGI("RAW_DUMP", "F%d: [CH0: 0x%08X (%d)] [CH1: 0x%08X] [CH2: 0x%08X] [CH3: 0x%08X]",
    //                  f,
    //                  (unsigned int)buffer[f * MIC_CHANNELS + 0],
    //                  (int)(buffer[f * MIC_CHANNELS + 0] >> 16),
    //                  (unsigned int)buffer[f * MIC_CHANNELS + 1],
    //                  (unsigned int)buffer[f * MIC_CHANNELS + 2],
    //                  (unsigned int)buffer[f * MIC_CHANNELS + 3]);
    //     }
    //     dump_count++;
    // }

    esp_err_t err;

    /* =============================================================================
     *             1: Inicialización y validación de parámetros
     * ============================================================================*/

    if (!is_initialized) {
        ESP_LOGE(TAG, "DSP Pipeline no ha sido inicializado");
        return ESP_ERR_INVALID_STATE;
    }

    if (buffer == NULL || event_out == NULL || mic_mono_out == NULL) {
        ESP_LOGE(TAG, "Punteros de buffer o salida nulos");
        return ESP_ERR_INVALID_ARG;
    }

    // Inicializar la estructura de evento de DSP
    event_out->is_valid = false;
    event_out->angle = 0.0f;
    event_out->sector = SECTOR_NONE;

    /* =============================================================================
     *             2: Cálculo de la energía de la señal y conversión a mono
     * ============================================================================*/
    float sum_sq = 0.0f, sum = 0.0f;

    for (int i = 0; i< DMA_FRAME_NUM; i++) {
        int16_t sample_i16 = (int16_t)(buffer[i * MIC_CHANNELS] >> 16); // Convertir la muestra de 24 bits a 16 bits para el micrófono 1 (canal 0)
        mic_mono_out[i] = sample_i16; // Almacenar la muestra convertida en el buffer de salida mono

        float s = (float)sample_i16; // Convertir la muestra a float para el cálculo de energía

        sum_sq += s * s; // Acumulación de la energía de la señal, eliminacion de negativos Vpeak
        sum += s; // Acumulación de la señal para calcular la media
    }

    float mean = sum / (float)DMA_FRAME_NUM; // Calcular la media de la señal para el cálculo de varianza
    float mean_sq = sum_sq / (float)DMA_FRAME_NUM; // Calcular la media de los cuadrados de la señal para el cálculo de RMS
    float variance = mean_sq - (mean * mean); // Calcular la varianza de la señal

    if (variance < 0.0f) {
        variance = 0.0f; /* Protección por redondeo numérico de punto flotante */
    }

    float rms = sqrtf(variance); // Calculo de RMS para señales discretas no periodicas, se calcula sobre la varianza para elimiinar el offset DC de la señal

    //ESP_LOGI("DEBUG_ENERGIA", "Media: %.1f | Var: %.1f | RMS: %.2f", mean, variance, rms);
    if (rms < DSP_ENERGY_THRESHOLD) {
        return ESP_OK; // No hay suficiente energía para procesar
    }

    err = tdoa_process(buffer, &event_out->angle);

    if (err == ESP_ERR_NOT_FOUND) {
        return ESP_OK; // No se encontró una correlación clara, pero no es un error crítico
    }

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error al procesar TDOA: %s", esp_err_to_name(err));
        return err;
    }

    event_out->is_valid = true;
    event_out->sector = (tdoa_sector_t)((int)((event_out->angle + 22.5f) / 45.0f) % 8);

    return ESP_OK;
}
