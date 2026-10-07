#include <stdlib.h>
#include <string.h>
#include "esp_log.h"
#include "drv2605.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "DRV2605L";

esp_err_t drv2605_write_reg(drv2605_handle_t dev, uint8_t reg, uint8_t val)
{
    uint8_t buffer[2] = {reg, val};
    return i2c_master_transmit(dev->i2c_dev, buffer, sizeof(buffer), -1);
}

esp_err_t drv2605_read_reg(drv2605_handle_t dev, uint8_t reg, uint8_t *out_val)
{
    return i2c_master_transmit_receive(dev->i2c_dev, &reg, 1, out_val, 1, -1);
}

esp_err_t drv2605_init(gpio_num_t sda_pin, gpio_num_t scl_pin, drv2605_handle_t *out_handle)
{
    if (out_handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // 1. Asignar memory for the device handle
    drv2605_handle_t dev = (drv2605_handle_t)calloc(1, sizeof(drv2605_dev_t));
    if (dev == NULL) {
        ESP_LOGE(TAG, "Error assigning memory for handle DRV2605L");
        return ESP_ERR_NO_MEM;
    }

    // 2. Configure and create the I2C master bus
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = sda_pin,
        .scl_io_num = scl_pin,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = false, // Disable internal pull-ups, use external pull-ups
    };

    i2c_master_bus_handle_t bus_handle;
    esp_err_t ret = i2c_new_master_bus(&bus_config, &bus_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error creating the I2C master bus: %s", esp_err_to_name(ret));
        free(dev);
        return ret;
    }

    // 3. Configure and add the DRV2605L device to the I2C bus
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = DRV2605_ADDR,
        .scl_speed_hz = 400000,
    };

    ret = i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev->i2c_dev);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error adding the device to the I2C bus: %s", esp_err_to_name(ret));
        i2c_del_master_bus(bus_handle);
        free(dev);
        return ret;
    }

    // 4. Verify communication with the DRV2605L by reading the STATUS register
    uint8_t status_reg = 0;
    ret = drv2605_read_reg(dev, DRV2605_REG_STATUS, &status_reg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Unable to communicate with DRV2605L at address 0x%02X", DRV2605_ADDR);
        i2c_master_bus_rm_device(dev->i2c_dev);
        i2c_del_master_bus(bus_handle);
        free(dev);
        return ret;
    }

    uint8_t chip_id = (status_reg >> 5) & 0x07;
    ESP_LOGI(TAG, "Module detected. STATUS: 0x%02X (Device ID: 0x%02X)", status_reg, chip_id);

    // 1. FORCE the device into Standby mode to allow configuration
    drv2605_write_reg(dev, DRV2605_REG_MODE, DRV2605_MODE_STANDBY);
    drv2605_write_reg(dev, DRV2605_REG_FEEDBACK, 0x36);  
    drv2605_write_reg(dev, DRV2605_REG_CONTROL3, 0x20); // Configure Control3 register for Open-Loop operation
    drv2605_write_reg(dev, DRV2605_REG_RATEDV, 0x90);   // Rated Voltage
    drv2605_write_reg(dev, DRV2605_REG_CLAMPV, 0xA4);   // Overdrive Clamp Voltage

    *out_handle = dev;
    ESP_LOGI(TAG, "I2C bus and DRV2605L initialized correctly. Mode: Open-Loop, Rated Voltage: 0x90, Clamp Voltage: 0xA4");
    return ESP_OK;

    *out_handle = dev;
    ESP_LOGI(TAG, "I2C bus and DRV2605L initialized correctly.");
    return ESP_OK;
}

esp_err_t drv2605_select_motor(drv2605_handle_t dev, drv2605_motor_type_t type)
{
    if (dev == NULL) return ESP_ERR_INVALID_ARG;

    uint8_t feedback_reg = 0;
    esp_err_t ret = drv2605_read_reg(dev, DRV2605_REG_FEEDBACK, &feedback_reg);
    if (ret != ESP_OK) return ret;

    if (type == DRV2605_MOTOR_LRA) {
        feedback_reg |= (1 << 7);
        drv2605_write_reg(dev, DRV2605_REG_FEEDBACK, feedback_reg);
        drv2605_write_reg(dev, DRV2605_REG_LIBRARY, 0x06);
        ESP_LOGI(TAG, "Configured for motor LRA");
    } else {
        feedback_reg &= ~(1 << 7);
        drv2605_write_reg(dev, DRV2605_REG_FEEDBACK, feedback_reg);
        drv2605_write_reg(dev, DRV2605_REG_LIBRARY, 0x01);
        ESP_LOGI(TAG, "Configured for motor ERM");
    }

    return ESP_OK;
}

esp_err_t drv2605_set_sequence(drv2605_handle_t dev, const uint8_t *seq, uint8_t len)
{
    if (dev == NULL || seq == NULL || len == 0) return ESP_ERR_INVALID_ARG;
    if (len > 8) len = 8;

    uint8_t buffer[9];
    buffer[0] = DRV2605_REG_WAVESEQUENCE1;
    for (int i = 0; i < len; i++) {
        buffer[i + 1] = seq[i];
    }
    if (len < 8) {
        buffer[len + 1] = 0x00;
        len++;
    }

    return i2c_master_transmit(dev->i2c_dev, buffer, len + 1, -1);
}

esp_err_t drv2605_set_effect(drv2605_handle_t dev, uint8_t effect_id)
{
    uint8_t seq[2] = {effect_id, 0x00};
    return drv2605_set_sequence(dev, seq, 1);
}

esp_err_t drv2605_go(drv2605_handle_t dev)
{
    if (dev == NULL) return ESP_ERR_INVALID_ARG;
    return drv2605_write_reg(dev, DRV2605_REG_GO, 0x01);
}

esp_err_t drv2605_wait_idle(drv2605_handle_t dev, uint32_t timeout_ms)
{
    if (dev == NULL) return ESP_ERR_INVALID_ARG;

    uint8_t go_bit = 1;
    uint32_t elapsed = 0;
    const uint32_t poll_interval_ms = 10;

    while (go_bit != 0) {
        esp_err_t ret = drv2605_read_reg(dev, DRV2605_REG_GO, &go_bit);
        if (ret != ESP_OK) return ret;

        go_bit &= 0x01;
        if (go_bit == 0) break;

        vTaskDelay(pdMS_TO_TICKS(poll_interval_ms));
        elapsed += poll_interval_ms;

        if (elapsed >= timeout_ms) {
            ESP_LOGW(TAG, "Timeout waiting for DRV2605L to finish effect playback");
            return ESP_ERR_TIMEOUT;
        }
    }
    return ESP_OK;
}
