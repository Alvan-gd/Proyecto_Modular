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

#define I2C_MASTER_SDA_IO   5   
#define I2C_MASTER_SCL_IO   6   
#define I2C_MASTER_NUM      I2C_NUM_0

static const char *TAG = "MAIN";

void app_main(void)
{
    // 1. Configurar e inicializar el bus maestro I2C
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_MASTER_NUM,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = false,
    };

    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handle));

    // 2. Inicializar biblioteca (deja el chip en Standby de manera segura)
    drv2605_handle_t haptic_dev;
    esp_err_t ret = drv2605_init(bus_handle, &haptic_dev);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Fallo crítico al inicializar el driver DRV2605L. Abortando.");
        return;
    }

    // 3. Seleccionar tipo de motor (ERM) mientras el chip está en Standby
    ret = drv2605_select_motor(haptic_dev, DRV2605_MOTOR_ERM);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error al configurar el tipo de motor ERM. Abortando.");
        return;
    }

    // 4. Diagnóstico previo antes de despertar al puente H
    uint8_t status_val = 0;
    drv2605_read_reg(haptic_dev, DRV2605_REG_STATUS, &status_val);
    ESP_LOGI(TAG, "--- DIAGNÓSTICO INICIAL (STANDBY) ---");
    ESP_LOGI(TAG, "Registro STATUS (0x00): 0x%02X", status_val);

    // 5. Despertar el chip y pasarlo a Modo Real-Time Playback (RTP = 0x05)
    ESP_LOGI(TAG, "Cambiando a modo Real-Time Playback (RTP)...");
    drv2605_write_reg(haptic_dev, DRV2605_REG_MODE, DRV2605_MODE_REALTIME);

    // Pequeño delay de asentamiento del oscilador interno
    vTaskDelay(pdMS_TO_TICKS(10));

    // 6. Iniciar la reproducción enviando amplitud al registro RTP (0x02)
    ESP_LOGI(TAG, "Enviando amplitud de prueba al motor (0x7F)...");
    drv2605_write_reg(haptic_dev, 0x02, 0x7F);

    // Mantener activo durante 2 segundos
    vTaskDelay(pdMS_TO_TICKS(20000));

    // 7. Apagar la salida escribiendo 0x00
    drv2605_write_reg(haptic_dev, 0x02, 0x00);

    // 8. Diagnóstico posterior para verificar si saltó la protección OC
    drv2605_read_reg(haptic_dev, DRV2605_REG_STATUS, &status_val);
    ESP_LOGI(TAG, "--- DIAGNÓSTICO POST-EJECUCIÓN ---");
    ESP_LOGI(TAG, "Registro STATUS (0x00): 0x%02X", status_val);

    if (status_val & (1 << 0)) {
        ESP_LOGE(TAG, "-> OVER_CURRENT: El puente H detectó sobrecorriente dinámica.");
    } else {
        ESP_LOGI(TAG, "-> ÉXITO: Ejecución completada sin errores de sobrecorriente.");
    }

    // Regresar a Standby por seguridad al finalizar
    drv2605_write_reg(haptic_dev, DRV2605_REG_MODE, DRV2605_MODE_STANDBY);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
