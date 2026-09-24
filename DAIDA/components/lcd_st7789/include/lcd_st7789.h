#ifndef LCD_ST7789_H
#define LCD_ST7789_H

// ESP-IDF native headers
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"

// Native pin definitions based on XIAO ESP32-S3 schematic
#define LCD_HOST            SPI2_HOST
#define PIN_NUM_DIN         9   // Pin D10 (SPI MOSI)
#define PIN_NUM_CS          8   // Pin D9
#define PIN_NUM_CLK         7   // Pin D8 (SPI Clock)
#define PIN_NUM_CD          44  // Pin D7 (Data/Command)

#define PIN_NUM_MISO        -1  // Not required for write-only mode
#define PIN_NUM_RST         -1  // Connected directly to 3.3V (no GPIO control needed)
#define PIN_NUM_BL          -1  // Connected directly to 3.3V (no GPIO control needed)

// Display resolution
#define LCD_H_RES           240
#define LCD_V_RES           280

/**
 * @brief Initializes the ST7789 LCD panel using the ESP-IDF esp_lcd component.
 * 
 * @param ret_io_handle Pointer to store the created panel IO handle.
 * @param ret_panel_handle Pointer to store the created panel handle.
 */
void lcd_st7789_init(esp_lcd_panel_io_handle_t *ret_io_handle, esp_lcd_panel_handle_t *ret_panel_handle);

#endif // LCD_ST7789_H