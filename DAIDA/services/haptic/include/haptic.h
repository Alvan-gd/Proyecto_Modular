#include "drv2605.h"

#define STRONG_CLICK 1
#define DOUBLE_CLICK 10
#define TRIPLE_CLICK 12
#define STRONG_BUZZ 14
#define ALERT_750 15
#define ALERT_1000 16


/**
 * @brief Initializes the haptic feedback system by setting up the DRV2605L device.
 * 
 * @param[out] out_handle Pointer to the handle of the initialized DRV2605L device
 * @return esp_err_t ESP_OK if successfully initialized, otherwise an error code
 */
esp_err_t haptic_init(drv2605_handle_t *out_handle);
/**
 * @brief Plays a haptic feedback pattern using the DRV2605L device.
 * 
 * @param[in] haptic_dev Handle to the DRV2605L device
 * @param[in] pattern Pointer to an array of waveform sequences to be played
 * @return esp_err_t ESP_OK if successfully played, otherwise an error code 
 */
esp_err_t haptic_play_pattern(drv2605_handle_t haptic_dev, const uint8_t *seq);

/** 
 * @brief The following functions invoque the haptic_play_pattern function with predefined patterns for different urgency levels.
 */
esp_err_t haptic_play_low_urgency(drv2605_handle_t haptic_dev);
esp_err_t haptic_play_medium_urgency(drv2605_handle_t haptic_dev);
esp_err_t haptic_play_high_urgency(drv2605_handle_t haptic_dev);
esp_err_t haptic_play_special_urgency(drv2605_handle_t haptic_dev);