#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_heap_caps.h" // Requerido para la asignación de memoria segura

#include "drv_i2s_mic.h"
#include "dsp_pipeline.h"

static const char *TAG = "MAIN_VALIDATION";

#define COOLDOWN_MS 200
#define SYNTH_FREQ_HZ 1000.0f
#define PI 3.14159265f

// Cola para comunicar eventos del DSP (Core 1) al UI/Control (Core 0)
static QueueHandle_t dsp_event_queue;

/* =============================================================================
 * Fase 1: Autotest Sintético (Mock de Hardware I2S)
 * ============================================================================*/
void run_synthetic_autotest(void) {
    ESP_LOGI(TAG, "--- INICIANDO AUTOTEST SINTETICO ---");
    
    int32_t *synth_buffer = (int32_t *)heap_caps_malloc(DMA_FRAME_NUM * MIC_CHANNELS * sizeof(int32_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    int16_t *mono_out_dummy = (int16_t *)heap_caps_malloc(DMA_FRAME_NUM * sizeof(int16_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

    if (synth_buffer == NULL || mono_out_dummy == NULL) {
        ESP_LOGE(TAG, "[FATAL] No hay suficiente SRAM interna para el Autotest.");
        return;
    }

    dsp_event_t event;
    float sample_rate = 16000.0f;
    float phase_step = 2.0f * PI * SYNTH_FREQ_HZ / sample_rate;

    for (int i = 0; i < DMA_FRAME_NUM; i++) {
        // Amplitud base representativa de 16 bits (pico = 12000, RMS ≈ 8485)
        int16_t wave_base   = (int16_t)(12000.0f * sinf(i * phase_step));
        int16_t wave_delay1 = (int16_t)(12000.0f * sinf((i - 1) * phase_step));

        // Desplazamiento << 16 para alinear bits [31:16]
        synth_buffer[i * MIC_CHANNELS + 0] = (int32_t)wave_base << 16;    // X+ (Llega primero)
        synth_buffer[i * MIC_CHANNELS + 1] = (int32_t)wave_delay1 << 16;  // X- (Llega con retraso)
        synth_buffer[i * MIC_CHANNELS + 2] = (int32_t)wave_base << 16;    // Y+ (En fase)
        synth_buffer[i * MIC_CHANNELS + 3] = (int32_t)wave_base << 16;    // Y- (En fase)
    }

    esp_err_t err = dsp_pipeline_process(synth_buffer, &event, mono_out_dummy);
    ESP_LOGI(TAG, "[Debug Sintético] err: %s, is_valid: %d, angle: %.2f", esp_err_to_name(err), event.is_valid, event.angle);

    if (err == ESP_OK && event.is_valid) {
        ESP_LOGW(TAG, "[Sintetico] Prueba 0° -> Angulo calculado: %.2f°, Sector: %d", event.angle);
    } else {
        ESP_LOGE(TAG, "[Sintetico] Falla en la inyección de señal para 0°");
    }
    
    free(synth_buffer);
    free(mono_out_dummy);
    
    ESP_LOGI(TAG, "--- FIN AUTOTEST SINTETICO ---");
}

/* =============================================================================
 * Fase 2: Tareas FreeRTOS Multinúcleo
 * ============================================================================*/

void task_dsp_core1(void *pvParameters) {
    ESP_LOGI(TAG, "Tarea DSP fijada en Núcleo %d", xPortGetCoreID());

    // Buffers persistentes alojados en el Heap, no en la frágil pila de la tarea
    int32_t *dma_buffer = (int32_t *)heap_caps_malloc(DMA_FRAME_NUM * MIC_CHANNELS * sizeof(int32_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    int16_t *mic_mono = (int16_t *)heap_caps_malloc(DMA_FRAME_NUM * sizeof(int16_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

    if (dma_buffer == NULL || mic_mono == NULL) {
        ESP_LOGE(TAG, "[FATAL] No hay memoria SRAM para la tarea DSP Core 1");
        vTaskDelete(NULL);
    }

    size_t bytes_read = 0;
    dsp_event_t event;

    while (1) {
        if (drv_i2s_mic_read(dma_buffer, DMA_FRAME_NUM * MIC_CHANNELS * sizeof(int32_t), &bytes_read, 100) == ESP_OK) {
            if (dsp_pipeline_process(dma_buffer, &event, mic_mono) == ESP_OK) {
                if (event.is_valid) {
                    xQueueSend(dsp_event_queue, &event, 0);
                }
            }
        }
        else {
            ESP_LOGE(TAG, "Error leyendo I2S. Reintentando...");
            vTaskDelay(pdMS_TO_TICKS(10)); // Cede el control al RTOS para no disparar el Watchdog
        }
    }
}

void task_ui_core0(void *pvParameters) {
    ESP_LOGI(TAG, "Tarea UI/Control fijada en Núcleo %d", xPortGetCoreID());
    
    dsp_event_t received_event;
    int64_t last_event_time_us = 0;

    while (1) {
        if (xQueueReceive(dsp_event_queue, &received_event, portMAX_DELAY) == pdTRUE) {
            int64_t current_time_us = esp_timer_get_time();
            
            if ((current_time_us - last_event_time_us) > (COOLDOWN_MS * 1000)) {
                ESP_LOGI(TAG, ">>> SONIDO DETECTADO | Angulo: %05.1f°<<<", 
                         received_event.angle);
                
                last_event_time_us = current_time_us;
            }
        }
    }
}

void app_main(void) {
    // 0. Inicializar primitivas de concurrencia de forma global
    dsp_event_queue = xQueueCreate(10, sizeof(dsp_event_t));
    if (dsp_event_queue == NULL) {
        ESP_LOGE(TAG, "Fallo al crear la cola de eventos");
        return;
    }

    // 1. Inicializar matemática y estructuras
    ESP_ERROR_CHECK(dsp_pipeline_init());
    
    // 2. Ejecutar prueba matemática con hardware apagado
    run_synthetic_autotest();

    // 3. Inicializar Periféricos de Hardware
    ESP_ERROR_CHECK(drv_i2s_mic_init());

    // 4. Crear tareas. Con los buffers en el Heap, 8192 bytes de stack es muchísimo margen, incluso sobrado.
    xTaskCreatePinnedToCore(task_dsp_core1, "DSP_Task", 8192, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(task_ui_core0,  "UI_Task",  4096, NULL, 2, NULL, 0);
}