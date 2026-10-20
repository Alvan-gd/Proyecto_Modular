#include <stdio.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_err.h"

#include "daida_compute.h"
#include "daida_ui.h"

#define DAIDA_IPC_QUEUE_DEPTH       1
#define DAIDA_BOOT_RETRY_DELAY_MS   2000

static const char *TAG = "DAIDA_MAIN";

/**
 * @brief Manejador de fallos críticos durante el arranque.
 *        Limpia recursos del Heap y fuerza un reinicio controlado del SoC
 *        para evitar que el dispositivo quede en estado "zombie".
 */
static void daida_boot_fail_handler(QueueHandle_t queue_to_clean) {
    if (queue_to_clean != NULL) {
        vQueueDelete(queue_to_clean);
    }
    ESP_LOGE(TAG, "Sistema abortado. Reiniciando SoC en %d ms...", DAIDA_BOOT_RETRY_DELAY_MS);
    vTaskDelay(pdMS_TO_TICKS(DAIDA_BOOT_RETRY_DELAY_MS));
    esp_restart();
}

void app_main(void) {
    ESP_LOGI(TAG, "===========================================");
    ESP_LOGI(TAG, "   INICIANDO SISTEMA EMBEBIDO DAIDA        ");
    ESP_LOGI(TAG, "===========================================");

    // 1. Instanciación del bus IPC en el Heap (puntero alojado temporalmente en el Stack local)
    QueueHandle_t compute_to_ui_queue = xQueueCreate(DAIDA_IPC_QUEUE_DEPTH, sizeof(daida_compute_event_t));
    if (compute_to_ui_queue == NULL) {
        ESP_LOGE(TAG, "Fallo crítico: Sin memoria en Heap para compute_to_ui_queue");
        daida_boot_fail_handler(NULL);
    }

    // 2. Inicializar subsistema consumidor (Core 1: Pantalla, DRV2605L y tarea UI)
    esp_err_t err = daida_ui_init(compute_to_ui_queue);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error al inicializar daida_ui (Core 1): %s", esp_err_to_name(err));
        daida_boot_fail_handler(compute_to_ui_queue);
    }
    ESP_LOGI(TAG, "Core 1 inicializado. Interfaz de usuario en ejecución...");

    // 3. Inicializar subsistema productor (Core 0: I2S DMA, TDoA y TinyML)
    err = daida_compute_init(compute_to_ui_queue);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error al inicializar daida_compute (Core 0): %s", esp_err_to_name(err));
        daida_boot_fail_handler(compute_to_ui_queue);
    }
    ESP_LOGI(TAG, "Core 0 inicializado. Procesamiento DSP/TinyML en ejecución...");

    // 4. Destruir main_task y devolver su Stack (~3.5 KB) y variables locales a la SRAM
    ESP_LOGI(TAG, "Arranque exitoso. Liberando recursos de main_task...");
    vTaskDelete(NULL);
}