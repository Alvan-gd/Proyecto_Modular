#include "screen.h"
#include "esp_lvgl_port.h" // Necesario para lvgl_port_lock/unlock
#include "adc_daida.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// Utility functions
border_zone_t angle_to_sector(float angle) {
    /* Desfase de 22.5° para centrar el Sector 0 en 0° y máscara módulo 8 (& 7) */
    uint8_t sector = (uint8_t)((angle + 22.5f) / 45.0f) & 0x07;
    return sector;                                    // TOP_LEFT
}

lv_color_t get_urgency_color(ui_urgency_t urgency) {
    if (urgency == LOW_URGENCY_UI)    return ST7789_COLOR_GREEN;
    if (urgency == MEDIUM_URGENCY_UI) return ST7789_COLOR_YELLOW;
    if (urgency == HIGH_URGENCY_UI)   return ST7789_COLOR_RED;
    return ST7789_COLOR_BLUE;
}

// Service Functions
esp_err_t screen_init(
    esp_lcd_panel_io_handle_t *ret_io_handle, 
    esp_lcd_panel_handle_t *ret_panel_handle, 
    lv_display_t **ret_disp) 
{
    if (ret_io_handle == NULL || ret_panel_handle == NULL || ret_disp == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // Inicializar el LCD físico y el puerto LVGL
    lcd_st7789_init(ret_io_handle, ret_panel_handle); 
    lcd_lvgl_init(*ret_io_handle, *ret_panel_handle, ret_disp);

    // Adquirir el lock de LVGL para manipular objetos de interfaz
    if (lvgl_port_lock(0)) {
        lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x000000), 0);
        lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, 0);

        illuminate_border_init();
        init_icon();
        init_battery_label();
        init_icon_label();
        init_battery_icon();
        lvgl_port_unlock();
    }

    return ESP_OK;
}

esp_err_t draw_indication(
    border_zone_t direction,
    const uint8_t *bitmap,
    ui_urgency_t urgency,
    char text[]) {
    if (direction >= BORDER_ZONE_MAX || bitmap == NULL || urgency >= URGENCY_MAX_UI) {
        return ESP_ERR_INVALID_ARG;
    }
    lv_color_t color = get_urgency_color(urgency);
    if (lvgl_port_lock(0)) {
        illuminate_border_zone(direction, color);
        draw_icon(bitmap, color);
        write_icon_label(text,color);
        lvgl_port_unlock();
        return ESP_OK;
    }

    return ESP_ERR_TIMEOUT;
}

esp_err_t screen_clear(void) 
{
    if (lvgl_port_lock(0)) {
        illuminate_border_off();
        clear_icon();
        clear_icon_label();
        lvgl_port_unlock();
        return ESP_OK;
    }
    return ESP_ERR_TIMEOUT;
}

esp_err_t screen_update_battery(uint16_t battery_level) {
    if (lvgl_port_lock(0)) {
        write_battery_level(battery_level);
        draw_battery_icon(battery_level);
        lvgl_port_unlock();
        return ESP_OK;
    }
    return ESP_ERR_TIMEOUT;
}

esp_err_t draw_daida_splash_screen(void) {
    if (lvgl_port_lock(0)) {
        draw_splash_screen(); 
        lvgl_port_unlock();
        vTaskDelay(pdMS_TO_TICKS(5000));
        return ESP_OK;
    }
    return ESP_ERR_TIMEOUT;
}