#include "battery.h"
#include "adc_daida.h"
#include "esp_log.h"

static const char *TAG = "BATTERY_SERVICE";
static adc_xiao_handle_t s_adc_handle = NULL;

esp_err_t battery_service_init(void) {
    esp_err_t err = adc_init(&s_adc_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize ADC for battery service: %s", esp_err_to_name(err));
        return err;
    }
    ESP_LOGI(TAG, "Battery service initialized successfully");
    return ESP_OK;
}

esp_err_t battery_get_level(uint16_t *battery_pct) {
    if (s_adc_handle == NULL || battery_pct == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    int raw_value = 0;
    esp_err_t err = adc_read(s_adc_handle, &raw_value);
    if (err != ESP_OK) {
        return err;
    }

    // Convertir lectura cruda a milivoltios reales de la batería
    // V_adc (mV) = (raw * 3300) / 4095
    // V_bat (mV) = V_adc * 2
    uint32_t v_bat_mv = ((uint32_t)raw_value * 3300 * 2) / 4095;

    // Definir rangos típicos de Batería LiPo / Li-Ion
    const uint32_t V_MIN_MV = 3300; // 3.3V (0%)
    const uint32_t V_MAX_MV = 4200; // 4.2V (100%)

    if (v_bat_mv <= V_MIN_MV) {
        *battery_pct = 0;
    } else if (v_bat_mv >= V_MAX_MV) {
        *battery_pct = 100;
    } else {
        *battery_pct = (uint16_t)(((v_bat_mv - V_MIN_MV) * 100) / (V_MAX_MV - V_MIN_MV));
    }

    return ESP_OK;
}