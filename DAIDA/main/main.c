#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "lcd_st7789.h"
#include "lcd_lvgl_ui.h"
#include "drv2605.h"
#include "driver/i2c_master.h"
#include "esp_log.h"

static const char *TAG = "MAIN";

void app_main(void)
{
    drv2605_handle_t haptic_dev;

    // 1. Initialize the DRV2605L device and configure the I2C bus
    esp_err_t ret = drv2605_init(I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO, &haptic_dev);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error initializing DRV2605L");
        return;
    }

    // 2. Select the motor type (ERM) for the DRV2605L device
    drv2605_select_motor(haptic_dev, DRV2605_MOTOR_ERM);

    // 3. Use internal trigger mode for the DRV2605L device
    drv2605_write_reg(haptic_dev, DRV2605_REG_MODE, DRV2605_MODE_INTTRIG);

    while (1) {
        // --- Pattern 1: Strong Click (Effect #1) ---
        ESP_LOGI(TAG, "Executing Pattern 1: Strong Click (Effect 1)");
        drv2605_set_effect(haptic_dev, 1);
        drv2605_go(haptic_dev);
        drv2605_wait_idle(haptic_dev, 1000);

        vTaskDelay(pdMS_TO_TICKS(2000)); // Wait 2 seconds between patterns

        // --- Pattern 2: Double Click (Effect #12) ---
        ESP_LOGI(TAG, "Executing Pattern 2: Double Click (Effect 12)");
        drv2605_set_effect(haptic_dev, 12);
        drv2605_go(haptic_dev);
        drv2605_wait_idle(haptic_dev, 1000);

        vTaskDelay(pdMS_TO_TICKS(2000));

        // --- Pattern 3: Increasing Ramp (Effect #47) ---
        ESP_LOGI(TAG, "Executing Pattern 3: Increasing Ramp 0-100%% (Effect 47)");
        drv2605_set_effect(haptic_dev, 47);
        drv2605_go(haptic_dev);
        drv2605_wait_idle(haptic_dev, 2000);

        vTaskDelay(pdMS_TO_TICKS(4000)); // Wait longer before repeating the entire cycle
    }
}

