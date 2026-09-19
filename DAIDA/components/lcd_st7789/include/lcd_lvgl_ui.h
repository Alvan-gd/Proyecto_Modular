#ifndef LCD_LVGL_UI_H
#define LCD_LVGL_UI_H

#include "esp_err.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lvgl_port.h"
#include "lvgl.h"
#include "lcd_st7789.h"

lv_display_t *lcd_lvgl_init(esp_lcd_panel_io_handle_t io_handle, esp_lcd_panel_handle_t panel_handle);


#endif 
