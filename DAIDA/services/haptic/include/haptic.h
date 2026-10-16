#ifndef HAPTIC_H
#define HAPTIC_H

#include <stddef.h>
#include "drv2605.h"

#define STRONG_CLICK 1
#define DOUBLE_CLICK 10
#define TRIPLE_CLICK 12
#define STRONG_BUZZ  14
#define ALERT_750    15
#define ALERT_1000   16

/**
 * @brief Initializes the haptic system configuring the DRV2605L in ERM mode with internal trigger.
 * 
 * @param[out] out_handle Pointer to the DRV2605L inicialized.
 * @return esp_err_t ESP_OK if correct.
 */
esp_err_t haptic_init(drv2605_handle_t *out_handle);

/**
 * @brief Plays an arbitrary haptic pattern specifying the length of the array that contains it, maximum 7.
 * 
 * @param[in] haptic_dev Handle DRV2605L.
 * @param[in] pattern Wave sequence array.
 * @param[in] len Number of elements in pattern.
 * @return esp_err_t ESP_OK .
 */
esp_err_t haptic_play_pattern(drv2605_handle_t haptic_dev, const uint8_t *pattern, size_t len);

/**
 * @brief Stops haptic pattern being played
 * 
 * @param[in] haptic_dev Handle DRV2605L.
 * @return esp_err_t ESP_OK 
 */
esp_err_t haptic_stop(drv2605_handle_t haptic_dev);

/** 
 * @brief Predefined functions by urgency level. This are the only functions other than init that should be used in higher layers.
 */
esp_err_t haptic_play_low_urgency(drv2605_handle_t haptic_dev);
esp_err_t haptic_play_medium_urgency(drv2605_handle_t haptic_dev);
esp_err_t haptic_play_high_urgency(drv2605_handle_t haptic_dev);
esp_err_t haptic_play_special_urgency(drv2605_handle_t haptic_dev);

#endif // HAPTIC_H