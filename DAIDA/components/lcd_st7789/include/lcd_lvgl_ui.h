#ifndef LCD_LVGL_UI_H
#define LCD_LVGL_UI_H

#include "esp_err.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lvgl_port.h"
#include "lvgl.h"
#include "lcd_st7789.h"
#include "esp_lcd_types.h"
#include "alarm.h"
#include "bell.h"
#include "danger.h"
#include "phone.h"
#include "voice.h"

// Defined color macros mapped to ST7789 panel logic
#define ST7789_COLOR_GREEN     lv_color_hex(0x0000ff) // Pure green
#define ST7789_COLOR_YELLOW    lv_color_hex(0xff00ee) // Red  + Green 
#define ST7789_COLOR_BLUE      lv_color_hex(0x00ff00) // Pure blue
#define ST7789_COLOR_RED       lv_color_hex(0xff0000) // Pure red

// Structure to define perimeter illumination steps
typedef struct {
    int32_t x1;
    int32_t y1;
    int32_t x2;
    int32_t y2;
} border_coords_t;

typedef enum {
    TOP = 0,
    TOP_RIGHT,
    RIGHT,
    BOTTOM_RIGHT,
    BOTTOM,
    BOTTOM_LEFT,
    LEFT,
    TOP_LEFT,
    BORDER_ZONE_MAX
} border_zone_t;


/**
 * @brief Initializes LVGL display interface.
 * 
 * @param io_handle Panel IO handle.
 * @param panel_handle Display panel handle.
 * @return lv_display_t* Pointer to created LVGL display instance.
 */
lv_display_t *lcd_lvgl_init(esp_lcd_panel_io_handle_t io_handle, esp_lcd_panel_handle_t panel_handle);

/**
 * @brief Initializes global persistent border illumination object.
 */
void illuminate_border_init(void);

/**
 * @brief Illuminates specific edge zone defined by coordinates and color.
 */
void illuminate_border_zone(border_zone_t zone, lv_color_t color);

/**
 * @brief Hides or turns off active border illumination.
 */
void illuminate_border_off(void);

/**
 * @brief Initializes the icon display.
 */
void init_icon(void);

/**
 * @brief Draws a 64x64 icon centered on the display
 * 
 * @param bitmap Pointer to the icon bitmap data.
 * @param color Color of the icon.
 */
void draw_icon(const uint8_t *bitmap, lv_color_t color);


#endif // LCD_LVGL_UI_H
