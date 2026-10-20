#include <stdint.h>
#include "esp_err.h"
#include "hal/adc_types.h"
#include "esp_adc/adc_oneshot.h"


// Pin definition for XIAO ESP32S3 D9 (GPIO 8 = ADC1 Channel 7)
#define ADC_XIAO_PIN        8
#define ADC_XIAO_CHANNEL    ADC_CHANNEL_7
#define ADC_XIAO_UNIT       ADC_UNIT_1

// Handle structure for the XIAO ADC device
typedef struct {
    adc_oneshot_unit_handle_t adc_handle;
    adc_channel_t channel;
} adc_xiao_dev_t;

// Handle type for the XIAO ADC device
typedef adc_xiao_dev_t* adc_xiao_handle_t;


/**
 * @brief Initializes the ADC unit and configures the D9 channel for the XIAO ESP32S3.
 * 
 * @param[out] out_handle Pointer to the handle of the initialized ADC device
 * @return esp_err_t ESP_OK if successfully initialized, otherwise an error code
 */
esp_err_t adc_init(adc_xiao_handle_t *out_handle);

/**
 * @brief Performs a single One-Shot reading from the D9 ADC pin on demand.
 * 
 * @param[in] dev Handle to the ADC device
 * @param[out] out_raw Pointer to store the raw ADC reading (0 - 4095)
 * @return esp_err_t ESP_OK if successfully read, otherwise an error code
 */
esp_err_t adc_read(adc_xiao_handle_t dev, int *out_raw);

