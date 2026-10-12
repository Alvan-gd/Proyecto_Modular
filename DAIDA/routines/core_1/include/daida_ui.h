#ifndef DAIDA_ROUTINES_H
#define DAIDA_ROUTINES_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "screen.h"
#include "haptic.h"
/*
// Tipos de eventos acústicos detectados por la IA
typedef enum {
    DAIDA_SOUND_SISMIC_ALERT,
    DAIDA_SOUND_CAR_HORN,
    DAIDA_SOUND_USERS_NAME,
    DAIDA_SOUND_DOG_BARK,
    DAIDA_SOUND_GUNSHOTS,
    DAIDA_SOUND_RINGBELL,
    DAIDA_SOUND_MAX
} daida_sound_type_t;

// Estructura de mensaje enviada desde el Core 0 (IA + TDOA)
typedef struct {
    uint16_t           angle;       // Ángulo detectado (0 - 359°)
    daida_sound_type_t sound_type;  // Tipo de sonido clasificado
} daida_event_msg_t;
 */

/**
 * @brief Inicializa los servicios del Core 1 (Pantalla + Háptico) y crea la cola inter-core.
 */
esp_err_t daida_ui_init(void);

/**
 * @brief Tarea principal de FreeRTOS asignada al Core 1 para procesar la retroalimentación.
 */
void daida_ui_task(void *pvParameters);


#endif // DAIDA_ROUTINES_H
