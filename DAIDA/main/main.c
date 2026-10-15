#include <stdio.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_err.h"

#include "daida_compute.h"
#include "daida_ui.h"


static const char *TAG = "MAIN_CORE1_TEST";


void app_main(void) {
    ESP_LOGI(TAG, "=== Iniciando prueba aislada del Core 1 (UI / Háptico) ===");

    // 1. Inicializar pantalla, motor háptico, cola y tarea del Core 1
    esp_err_t ret = daida_ui_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error al inicializar daida_ui: %s", esp_err_to_name(ret));
        return;
    }

    QueueHandle_t ui_queue = daida_ui_get_queue_handle();
    if (ui_queue == NULL) {
        ESP_LOGE(TAG, "La cola de la UI no fue creada correctamente.");
        return;
    }

    ESP_LOGI(TAG, "Core 1 inicializado. Enviando eventos de prueba...");

    // 2. Secuencia de eventos de prueba simulando la salida del Core 0
    daida_compute_event_t test_events[] = {
        {.angle = 0.0f,   .audio_classification = 0}, // Alerta sísmica (Norte)
        {.angle = 45.0f,  .audio_classification = 1}, // Claxon (Noreste)
        {.angle = 90.0f,  .audio_classification = 2}, // Timbre (Este)
        {.angle = 180.0f, .audio_classification = 3}, // Nombre de usuario (Sur)
        {.angle = 270.0f, .audio_classification = 4}, // Teléfono (Oeste)
    };

    size_t num_events = sizeof(test_events) / sizeof(test_events[0]);
    size_t idx = 0;

    // 3. Bucle para alimentar la cola de la UI periódicamente
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(3000)); // Enviar un evento cada 3 segundos

        daida_compute_event_t event = test_events[idx];

        ESP_LOGI(TAG, "Simulando disparo -> Ángulo: %.1f°, Sonido: %u", 
                 event.angle, event.audio_classification);

        if (xQueueSend(ui_queue, &event, pdMS_TO_TICKS(100)) != pdTRUE) {
            ESP_LOGW(TAG, "La cola de UI está llena, no se pudo enviar el evento");
        }

        idx = (idx + 1) % num_events;
    }
}