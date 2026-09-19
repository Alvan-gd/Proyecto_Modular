#ifndef ST7789_H
#define ST7789_H

// Headers nativas de ESP-IDF
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"


// Definición nativa de pines basada en el esquema del XIAO ESP32S3
#define LCD_HOST               SPI2_HOST
#define PIN_NUM_DIN            9   // Pin D10 (SPI MOSI)
#define PIN_NUM_CS             8   // Pin D9
#define PIN_NUM_CLK            7   // Pin D8 (SPI Clock)
#define PIN_NUM_CD             44   // Pin D7 (Data/Command)

#define PIN_NUM_MISO           -1  // No requerido para solo escritura
#define PIN_NUM_RST            -1  // Controlado con conexión a 3.3V (no se requiere pin de control)
#define PIN_NUM_BL             -1  // Controlado con conexión a 3.3V (no se requiere pin de control)

// Dimensiones 
#define LCD_H_RES              240
#define LCD_V_RES              280

// Prototipado de funciones st7789
esp_lcd_panel_handle_t lcd_st7789_init(esp_lcd_panel_io_handle_t *ret_io_handle);


#endif