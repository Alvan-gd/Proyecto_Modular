#ifndef DRV2605_H
#define DRV2605_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/i2c_master.h"

// Pin definition based on the default I2C pins in the XIAO ESP32S3
#define I2C_MASTER_SDA_IO   5   
#define I2C_MASTER_SCL_IO   6   
#define I2C_MASTER_NUM      I2C_NUM_0

// Default I2C address for the DRV2605L device
#define DRV2605_ADDR            0x5A

// Internal registers in the DRV2605L device
#define DRV2605_REG_STATUS      0x00
#define DRV2605_REG_MODE        0x01
#define DRV2605_REG_LIBRARY     0x03
#define DRV2605_REG_WAVESEQUENCE1 0x04
#define DRV2605_REG_GO          0x0C
#define DRV2605_REG_RATEDV      0x16
#define DRV2605_REG_CLAMPV      0x17
#define DRV2605_REG_FEEDBACK    0x1A
#define DRV2605_REG_CONTROL3    0x1D

// Operating modes for the DRV2605L device
#define DRV2605_MODE_INTTRIG    0x00
#define DRV2605_MODE_REALTIME   0x05
#define DRV2605_MODE_STANDBY    0x40

// Motor types supported by the DRV2605L device
typedef enum {
    DRV2605_MOTOR_ERM = 0,
    DRV2605_MOTOR_LRA = 1
} drv2605_motor_type_t;

// Handle structure for the DRV2605L device
typedef struct {
    i2c_master_dev_handle_t i2c_dev;
} drv2605_dev_t;

// Handle type for the DRV2605L device
typedef drv2605_dev_t* drv2605_handle_t;

/**
 * @brief Initialezes the DRV2605L device and configures the I2C bus.
 *
 * @param[in] sda_pin GPIO assigned to SDA line
 * @param[in] scl_pin GPIO assigned to SCL line
 * @param[out] out_handle Pointer to the handle of the initialized DRV2605L device
 * @return esp_err_t ESP_OK if succesfully initialized, otherwise an error code
 */
esp_err_t drv2605_init(gpio_num_t sda_pin, gpio_num_t scl_pin, drv2605_handle_t *out_handle);
/**
 * @brief Selects the motor type (ERM or LRA) for the DRV2605L device.
 * 
 * @param[in] dev Handle to the DRV2605L device
 * @param[in] type Motor type to select (DRV2605_MOTOR_ERM or DRV2605_MOTOR_LRA)
 * @return esp_err_t ESP_OK if succesfully configured, otherwise an error code
 */
esp_err_t drv2605_select_motor(drv2605_handle_t dev, drv2605_motor_type_t type);
/** 
 * @brief Sets the waveform sequence for the DRV2605L device.
 * 
 * @param[in] dev Handle to the DRV2605L device
 * @param[in] seq Pointer to an array containing the waveform sequence
 * @param[in] len Length of the waveform sequence (maximum 8)
 * @return esp_err_t ESP_OK if succesfully configured, otherwise an error code
 */
esp_err_t drv2605_set_sequence(drv2605_handle_t dev, const uint8_t *seq, uint8_t len);
/**
 * @brief Sets a single effect to be played by the DRV2605L device.
 * 
 * @param[in] dev Handle to the DRV2605L device
 * @param[in] effect_id ID of the effect to be played (0-123)
 * @return esp_err_t ESP_OK if succesfully configured, otherwise an error code
 */
esp_err_t drv2605_set_effect(drv2605_handle_t dev, uint8_t effect_id);
/**
 * @brief Starts the playback of the configured effect on the DRV2605L device.
 * 
 * @param[in] dev Handle to the DRV2605L device
 * @return esp_err_t ESP_OK if succesfully started, otherwise an error code
 */
esp_err_t drv2605_go(drv2605_handle_t dev);
/**
 * @brief Waits for the DRV2605L device to finish playing the current effect.
 * 
 * @param[in] dev Handle to the DRV2605L device
 * @param[in] timeout_ms Timeout value in milliseconds
 * @return esp_err_t ESP_OK if succesfully completed, otherwise an error code
 */
esp_err_t drv2605_wait_idle(drv2605_handle_t dev, uint32_t timeout_ms);
/**
 * @brief Writes a value to a register in the DRV2605L device.
 * 
 * @param[in] dev Handle to the DRV2605L device
 * @param[in] reg Register address to write to
 * @param[in] val Value to write to the register
 * @return esp_err_t ESP_OK if succesfully written, otherwise an error code
 */
esp_err_t drv2605_write_reg(drv2605_handle_t dev, uint8_t reg, uint8_t val);
/**
 * @brief Reads a value from a register in the DRV2605L device.
 * 
 * @param[in] dev Handle to the DRV2605L device
 * @param[in] reg Register address to read from
 * @param[out] out_val Pointer to the variable where the read value will be stored
 * @return esp_err_t ESP_OK if succesfully read, otherwise an error code
 */
esp_err_t drv2605_read_reg(drv2605_handle_t dev, uint8_t reg, uint8_t *out_val);

#endif // DRV2605_H
