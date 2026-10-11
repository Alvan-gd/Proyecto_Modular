#include <stdio.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_err.h"

#include "daida_compute.h"

static const char *TAG = "MAIN_TEST_HARNESS";

/* Handle estático de la cola IPC entre Core 0 (Compute) y Core 1 (UI Mock) */
static QueueHandle_t s_ui_event_queue = NULL;

/**
 * @brief Función auxiliar para validar en el arnés el mapeo de 360° a 8 sectores
 *        que delegaste a la capa de UI (Sectores de 45° centrados).
 */
static uint8_t mock_ui_angle_to_sector(float angle) {
    /* Desfase de 22.5° para centrar el Sector 0 en 0° y máscara módulo 8 (& 7) */
    uint8_t sector = (uint8_t)((angle + 22.5f) / 45.0f) & 0x07;
    return sector;
}

/**
 * @brief Tarea consumidora falsa (Stub) anclada estrictamente al Core 1.
 *        Permanece bloqueada con 0% de uso de CPU hasta que el Core 0
 *        finaliza la captura post-trigger (800 ms) y la inferencia.
 */
static void task_mock_ui_core1(void *pvParameters) {
    daida_compute_event_t rx_event;

    ESP_LOGI(TAG, "[MOCK UI] Tarea iniciada correctamente en Core %d. Esperando eventos...", xPortGetCoreID());

    while (1) {
        /* Bloqueo infinito hasta que task_daida_tinyml_classification escriba en la cola */
        if (xQueueReceive(s_ui_event_queue, &rx_event, portMAX_DELAY) == pdTRUE) {
            uint8_t sector = mock_ui_angle_to_sector(rx_event.angle);

            ESP_LOGW(TAG, "==========================================");
            ESP_LOGW(TAG, "[IPC RX en Core %d] Evento Acústico Recibido", xPortGetCoreID());
            ESP_LOGW(TAG, " -> Ángulo (Ataque Inicial) : %.2f°", rx_event.angle);
            ESP_LOGW(TAG, " -> Sector UI Calculado     : %u (0-7)", sector);
            ESP_LOGW(TAG, " -> Clase TinyML (Dummy)    : %u", rx_event.audio_classification);
            ESP_LOGW(TAG, "==========================================");
        }
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "Iniciando Test Harness en Core %d...", xPortGetCoreID());

    /* 1. Crear la cola IPC de 1 elemento del tamaño de daida_compute_event_t */
    s_ui_event_queue = xQueueCreate(1, sizeof(daida_compute_event_t));
    if (s_ui_event_queue == NULL) {
        ESP_LOGE(TAG, "[FATAL] Fallo al alojar s_ui_event_queue en el Heap");
        return;
    }

    /* 2. Levantar el consumidor falso en el Core 1 ANTES de arrancar el productor */
    BaseType_t ret = xTaskCreatePinnedToCore(
        task_mock_ui_core1,
        "task_mock_ui_core1",
        4096,
        NULL,
        5,
        NULL,
        1 /* Anclada estrictamente al Core 1 */
    );

    if (ret != pdPASS) {
        ESP_LOGE(TAG, "[FATAL] No se pudo crear task_mock_ui_core1");
        return;
    }

    /* 3. Inicializar la rutina de cómputo en el Core 0 inyectándole la cola */
    ESP_ERROR_CHECK(daida_compute_init(s_ui_event_queue));

    ESP_LOGI(TAG, "Orquestación completada. app_main() liberando su stack.");
}