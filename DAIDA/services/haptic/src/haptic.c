#include "haptic.h"
#include "esp_log.h"
#include "esp_err.h"

static const char *TAG = "HAPTIC";

static const uint8_t LOW_URGENCY_PATTERN[] = {STRONG_CLICK, ALERT_750, STRONG_CLICK};
static const uint8_t MEDIUM_URGENCY_PATTERN[] = {DOUBLE_CLICK, ALERT_750, DOUBLE_CLICK, ALERT_750};
static const uint8_t HIGH_URGENCY_PATTERN[] = {ALERT_1000, ALERT_1000, ALERT_1000};
static const uint8_t SPECIAL_URGENCY_PATTERN[] = {ALERT_1000, STRONG_BUZZ, STRONG_BUZZ};

esp_err_t haptic_init(drv2605_handle_t *out_handle)
{
    if (out_handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    // 1. Initialize the DRV2605L device and configure the I2C bus
    esp_err_t ret = drv2605_init(I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO, out_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error initializing DRV2605L");
        return ret; 
    }
    // 2. Select the motor type (ERM) for the DRV2605L device
    ret = drv2605_select_motor(*out_handle, DRV2605_MOTOR_ERM);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error selecting motor type");
        return ret;
    }
    // 3. Use internal trigger mode for the DRV2605L device
    ret = drv2605_write_reg(*out_handle, DRV2605_REG_MODE, DRV2605_MODE_INTTRIG);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error setting internal trigger mode");
        return ret;
    }
    ESP_LOGI(TAG, "Haptic feedback system initialized successfully.");
    return ESP_OK;
}

esp_err_t haptic_play_pattern(drv2605_handle_t haptic_dev, const uint8_t *pattern)
{
    if (haptic_dev == NULL || pattern == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    // 1. Set the waveform sequence for the DRV2605L device
    esp_err_t ret = drv2605_set_sequence(haptic_dev, pattern, sizeof(pattern));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error setting waveform sequence");
        return ret;
    }
    // 2. Trigger the haptic feedback
    ret = drv2605_go(haptic_dev);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error triggering haptic feedback");
        return ret;
    }
    ESP_LOGI(TAG, "Haptic feedback pattern played successfully.");
    return ESP_OK;
}

esp_err_t haptic_play_low_urgency(drv2605_handle_t haptic_dev)
{
    return haptic_play_pattern(haptic_dev, LOW_URGENCY_PATTERN);
}

esp_err_t haptic_play_medium_urgency(drv2605_handle_t haptic_dev)
{
    return haptic_play_pattern(haptic_dev, MEDIUM_URGENCY_PATTERN);
}

esp_err_t haptic_play_high_urgency(drv2605_handle_t haptic_dev)
{
    return haptic_play_pattern(haptic_dev, HIGH_URGENCY_PATTERN);
}

esp_err_t haptic_play_special_urgency(drv2605_handle_t haptic_dev)
{
    return haptic_play_pattern(haptic_dev, SPECIAL_URGENCY_PATTERN);
}