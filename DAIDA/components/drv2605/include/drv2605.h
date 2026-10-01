#ifndef DRV2605_H
#define DRV2605_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/i2c_master.h"

#define DRV2605_ADDR            0x5A

#define DRV2605_REG_STATUS      0x00
#define DRV2605_REG_MODE        0x01
#define DRV2605_REG_LIBRARY     0x03
#define DRV2605_REG_WAVESEQUENCE1 0x04
#define DRV2605_REG_GO          0x0C
#define DRV2605_REG_RATEDV      0x16
#define DRV2605_REG_CLAMPV      0x17
#define DRV2605_REG_FEEDBACK    0x1A
#define DRV2605_REG_CONTROL3    0x1D

#define DRV2605_MODE_INTTRIG    0x00
#define DRV2605_MODE_REALTIME   0x05
#define DRV2605_MODE_STANDBY    0x40

typedef enum {
    DRV2605_MOTOR_ERM = 0,
    DRV2605_MOTOR_LRA = 1
} drv2605_motor_type_t;

typedef struct {
    i2c_master_dev_handle_t i2c_dev;
} drv2605_dev_t;

typedef drv2605_dev_t* drv2605_handle_t;

esp_err_t drv2605_init(i2c_master_bus_handle_t bus_handle, drv2605_handle_t *out_handle);
esp_err_t drv2605_select_motor(drv2605_handle_t dev, drv2605_motor_type_t type);
esp_err_t drv2605_set_sequence(drv2605_handle_t dev, const uint8_t *seq, uint8_t len);
esp_err_t drv2605_set_effect(drv2605_handle_t dev, uint8_t effect_id);
esp_err_t drv2605_go(drv2605_handle_t dev);
esp_err_t drv2605_wait_idle(drv2605_handle_t dev, uint32_t timeout_ms);
esp_err_t drv2605_write_reg(drv2605_handle_t dev, uint8_t reg, uint8_t val);
esp_err_t drv2605_read_reg(drv2605_handle_t dev, uint8_t reg, uint8_t *out_val);

#endif // DRV2605_H
