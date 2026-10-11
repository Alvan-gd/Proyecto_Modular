#include "daida_compute.h"
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_heap_caps.h"
#include "esp_log.h"

#include "dsp_pipeline.h"
#include "drv_i2s_mic.h"

static const char *TAG = "DAIDA_COMPUTE"; // Tag usado para imprimir mensajes del log

#define POST_TRIGGER_BLOCKS 50


/* =============================================================================
    *      Prototipos de funciones de tareas FreeRTOS (Funciones privadas)
    * ============================================================================*/
void task_daida_dsp_extraction(void *pvParameters);
void task_daida_tinyml_classification(void *pvParameters);


/* =============================================================================
    *                      Definicion de Ring Buffer para TinyML
    * ============================================================================*/

__attribute__((aligned(16))) static int16_t daida_ring_buffer[DAIDA_TINY_ML_SAMPLE_SIZE]; // Buffer circular para almacenar muestras de audio para TinyML
static uint32_t daida_ring_buffer_head = 0; // Puntero de cabeza del buffer circular


/* =============================================================================
    *                      Definicion de colas de eventos entre tareas
    * ============================================================================*/

static QueueHandle_t core0_dsp_queue = NULL; // Cola para enviar eventos de DSP task a la tarea de TinyML
static QueueHandle_t s_core1_ui_queue = NULL; // Cola para enviar eventos de DSP task a la tarea de UI/Control


/* =============================================================================
    *                      Definicion de tareas FreeRTOS core 0
    * ============================================================================*/

/**
 * @brief Task that handles DSP processing on Core 0. It reads audio data from the I2S microphones, processes it through the DSP pipeline, 
 * and manages a circular buffer for TinyML processing. When a valid event is detected, it captures a specified number of post-trigger blocks
 *  and sends the angle to the TinyML task for further classification.
 *  @param pvParameters Pointer to task parameters (not used in this implementation).
 * @return void
 */
void task_daida_dsp_extraction(void *pvParameters) {
    
    // Buffers persistentes alojados en el Heap, no en la frágil pila de la tarea
    int32_t *dma_buffer = (int32_t *)heap_caps_malloc(DMA_FRAME_NUM * MIC_CHANNELS * sizeof(int32_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    int16_t *mic_mono = (int16_t *)heap_caps_malloc(DMA_FRAME_NUM * sizeof(int16_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);


    if (dma_buffer == NULL || mic_mono == NULL) {
        ESP_LOGE(TAG, "[FATAL] No hay memoria SRAM para la tarea DSP Core 0");
        vTaskDelete(NULL);
    }
  
    size_t bytes_read = 0;
    dsp_event_t event;
    float angle = 0.0f;

    bool capturing_post_trigger = false;
    uint8_t blocks_remaining = 0;


    while(1){
        if (drv_i2s_mic_read(dma_buffer, DMA_FRAME_NUM * MIC_CHANNELS * sizeof(int32_t), &bytes_read, 100) == ESP_OK) {
            if (dsp_pipeline_process(dma_buffer, &event, mic_mono) == ESP_OK) {
                // Copiar las muestras mono al buffer circular
                for (size_t i = 0; i < DMA_FRAME_NUM; i++) {
                    daida_ring_buffer[(daida_ring_buffer_head + i) & (DAIDA_TINY_ML_SAMPLE_SIZE - 1)] = mic_mono[i];
                }
                // Actualizar la cabeza del buffer circular
                daida_ring_buffer_head = (daida_ring_buffer_head + DMA_FRAME_NUM) & (DAIDA_TINY_ML_SAMPLE_SIZE - 1);

                if (!capturing_post_trigger) {
                    if (event.is_valid) {
                        angle = event.angle;
                        capturing_post_trigger = true;
                        blocks_remaining = POST_TRIGGER_BLOCKS;
                    }
                } 
                else {
                    blocks_remaining--;
                    if (blocks_remaining == 0) {
                        capturing_post_trigger = false;
                        xQueueOverwrite(core0_dsp_queue, &angle);
                    }
                }
            }
        }
        else {
            ESP_LOGE(TAG, "Error leyendo I2S. Reintentando...");
            vTaskDelay(pdMS_TO_TICKS(10)); // Cede el control al RTOS para no disparar el Watchdog
        }
    }

}


/* =============================================================================
    *             Inicialización de la tareas DSP / TinyML en Core 0
    * ============================================================================*/

/**
 * @brief Task that handles TinyML classification on Core 0. It waits for an angle event from the DSP task, 
 * takes a snapshot of the circular buffer, and processes the audio data through the TinyML model for classification.
 *  The results are then sent to the UI task on Core 1.
 *  @param pvParameters Pointer to task parameters (not used in this implementation).
 * @return void
 */
void task_daida_tinyml_classification(void *pvParameters) {

    float angle;

    int16_t *ring_buffer = (int16_t *)heap_caps_malloc(DAIDA_TINY_ML_SAMPLE_SIZE * sizeof(int16_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (ring_buffer == NULL) {
        ESP_LOGE(TAG, "[FATAL] No hay memoria SRAM para el buffer temporal de TinyML");
        vTaskDelete(NULL);
    }

    while(1){

        if (xQueueReceive(core0_dsp_queue, &angle, portMAX_DELAY) == pdTRUE) {
            // Hacer un snapshot de la cabeza del buffer circular para evitar inconsistencias durante la copia
            uint32_t head_snapshot = daida_ring_buffer_head;

            // Copiar el contenido del buffer circular a un buffer temporal para TinyML
            for (size_t i = 0; i < DAIDA_TINY_ML_SAMPLE_SIZE; i++) {
                ring_buffer[i] = daida_ring_buffer[(head_snapshot + i) & (DAIDA_TINY_ML_SAMPLE_SIZE - 1)]; // Aritmetica modular para respetar el ring buffer
            }

            // Aquí se llamaría a la función de inferencia de TinyML con el buffer temporal
            ESP_LOGI(TAG, "Procesando inferencia TinyML con angulo detectado %.2f", angle); // placeholder
            uint8_t audio_classification_result = 0; // placeholder para el resultado de la clasificación

            // Empaquetar y enviar al Core 1 (UI / Háptica)
            daida_compute_event_t ui_event = {
                .angle = angle,
                .audio_classification = audio_classification_result
            };
            xQueueOverwrite(s_core1_ui_queue, &ui_event);
        }
    }
}


/**
 * @brief Initialize the DAIDA compute module, including the DSP pipeline, I2S microphone channels, 
 * and FreeRTOS tasks for DSP processing and TinyML classification. This function sets up the necessary 
 * queues for inter-task communication and ensures that all components are ready for operation.
 *  @param core1_ui_queue Queue handle for sending events from the DSP task to the UI/Control task on Core 1.
 * @return esp_err_t Returns ESP_OK on successful initialization, or an error code if any component fails to initialize.
 */
esp_err_t daida_compute_init(QueueHandle_t core1_ui_queue) {

    core0_dsp_queue = xQueueCreate(1, sizeof(float)); // Crear la cola para enviar eventos de DSP a la tarea de TinyML
    if (core0_dsp_queue == NULL) {
        ESP_LOGE(TAG, "La cola de eventos de TDoA es nula");
        return ESP_ERR_NO_MEM;
    }

    if (core1_ui_queue == NULL) {
        ESP_LOGE(TAG, "La cola de eventos de UI es nula");
        return ESP_ERR_INVALID_ARG;
    }

    s_core1_ui_queue = core1_ui_queue; // Cola para enviar eventos de DSP a la tarea de UI/Control


    // Inicializar la cabeza del buffer circular
    daida_ring_buffer_head = 0;

    ESP_ERROR_CHECK(drv_i2s_mic_init()); // Inicializar los canales I2S para los micrófonos INMP441

    ESP_ERROR_CHECK(dsp_pipeline_init()); // Inicializar la tubería DSP, incluyendo TDOA


    // Crear la tarea DSP/TDoA en el Core 0
    xTaskCreatePinnedToCore(task_daida_dsp_extraction, "task_daida_dsp_extraction", 4096, NULL, 10, NULL, 0);

    // Crear la tarea TinyML en el Core 0
    xTaskCreatePinnedToCore(task_daida_tinyml_classification, "task_daida_tinyml_classification", 4096, NULL, 5, NULL, 0);

    ESP_LOGI("DAIDA_COMPUTE", "Inicialización completa y tareas DSP / TinyML creadas en Core 0");
    return ESP_OK;
}

