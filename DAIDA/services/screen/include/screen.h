#include "lcd_st7789.h"
#include "lcd_lvgl_ui.h"
#include "esp_err.h"

typedef enum {
    LOW_URGENCY_UI,
    MEDIUM_URGENCY_UI,
    HIGH_URGENCY_UI,
    SPECIAL_URGENCY_UI,
    URGENCY_MAX_UI
}ui_urgency_t;

typedef struct {
    const uint8_t* icon_bitmap;
    lv_color_t     urgency_color; 
}daida_alert_t;

/**
 * @brief Inicializa el hardware LCD, LVGL y prepara los elementos de interfaz (ícono y bordes).
 */
esp_err_t screen_init(esp_lcd_panel_io_handle_t *ret_io_handle, esp_lcd_panel_handle_t *ret_panel_handle, lv_display_t **ret_disp);

border_zone_t angle_to_sector(uint16_t angle);
lv_color_t get_urgency_color(ui_urgency_t urgency);

esp_err_t draw_indication(border_zone_t direction ,const uint8_t *bitmap, ui_urgency_t urgency);