#ifndef BATTERY_H
#define BATTERY_H

#include "esp_err.h"
#include <stdint.h>
#include "adc_daida.h"

/**
 * @brief Inicializa el servicio de batería y su componente ADC subyacente.
 * @return esp_err_t ESP_OK si es exitoso.
 */
esp_err_t battery_service_init(void);

/**
 * @brief Lee el nivel actual de la batería en porcentaje (0-100%).
 * 
 * @param[out] battery_pct Puntero para almacenar el porcentaje calculado.
 * @return esp_err_t ESP_OK si la lectura fue exitosa.
 */
esp_err_t battery_get_level(uint16_t *battery_pct);

#endif // BATTERY_H