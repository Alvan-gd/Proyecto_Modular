#ifndef DAIDA_ROUTINES_H
#define DAIDA_ROUTINES_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "screen.h"
#include "haptic.h"
#include "battery.h"

typedef struct {
    const uint8_t *bitmap;
    ui_urgency_t urgency_level;
    uint8_t sound_type;
    char label_text[];
}ui_event_t;

typedef struct {
    border_zone_t sector;
    ui_event_t event;
}dada_ui_feedback_t;

/**
 * @brief Inicializa los servicios del Core 1 (Pantalla + Háptico) y crea la cola inter-core.
 */
esp_err_t daida_ui_init(QueueHandle_t daida_intercore_queue);

/**
 * @brief Tarea principal de FreeRTOS asignada al Core 1 para procesar la retroalimentación.
 */
void daida_ui_task(void *pvParameters);

QueueHandle_t daida_ui_get_queue_handle(void);

static void battery_monitor_task(void *pvParameters);

#endif // DAIDA_ROUTINES_H
