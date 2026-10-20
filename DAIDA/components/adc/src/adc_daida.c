#include <stdlib.h>
#include "esp_log.h"
#include "adc_daida.h"

static const char *TAG = "ADC";

esp_err_t adc_init(adc_xiao_handle_t *out_handle)
{
    if (out_handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // 1. Allocate memory for the device handle
    adc_xiao_handle_t dev = (adc_xiao_handle_t)calloc(1, sizeof(adc_xiao_dev_t));
    if (dev == NULL) {
        ESP_LOGE(TAG, "Error allocating memory for ADC handle");
        return ESP_ERR_NO_MEM;
    }

    dev->channel = ADC_XIAO_CHANNEL;

    // 2. Initialize ADC oneshot unit (Unit 1) using the correct v6.2 config type name
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_XIAO_UNIT,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    
    esp_err_t ret = adc_oneshot_new_unit(&init_config, &dev->adc_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error creating ADC oneshot unit: %s", esp_err_to_name(ret));
        free(dev);
        return ret;
    }

    // 3. Configure the specific channel for D9 (GPIO 8) with 12dB attenuation (~3.1V range)
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };

    ret = adc_oneshot_config_channel(dev->adc_handle, dev->channel, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error configuring ADC channel: %s", esp_err_to_name(ret));
        adc_oneshot_del_unit(dev->adc_handle);
        free(dev);
        return ret;
    }

    *out_handle = dev;
    ESP_LOGI(TAG, "ADC initialized successfully on D9 (GPIO 8, Channel 7)");
    return ESP_OK;
}

esp_err_t adc_read(adc_xiao_handle_t dev, int *out_raw)
{
    if (dev == NULL || out_raw == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    return adc_oneshot_read(dev->adc_handle, dev->channel, out_raw);
}