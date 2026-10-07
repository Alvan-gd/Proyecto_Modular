#include "screen.h"


// Utility funtions
border_zone_t angle_to_sector(uint16_t angle) {
    angle = angle % 360;
    if (angle >= 338 || angle < 23)   return SECTOR_0;    // TOP
    if (angle >= 23  && angle < 68)   return SECTOR_45;   // TOP_RIGHT
    if (angle >= 68  && angle < 113)  return SECTOR_90;   // RIGHT
    if (angle >= 113 && angle < 158)  return SECTOR_135;  // BOTTOM_RIGHT
    if (angle >= 158 && angle < 203)  return SECTOR_180;  // BOTTOM
    if (angle >= 203 && angle < 248)  return SECTOR_225;  // BOTTOM_LEFT
    if (angle >= 248 && angle < 293)  return SECTOR_270;  // LEFT
    return SECTOR_315;                                    // TOP_LEFT
}

lv_color_t get_urgency_color(ui_urgency_t urgency){
    if(urgency == LOW_URGENCY_UI)    return ST7789_COLOR_GREEN;
    if(urgency == MEDIUM_URGENCY_UI) return ST7789_COLOR_YELLOW;
    if(urgency == HIGH_URGENCY_UI)   return ST7789_COLOR_RED;
    return ST7789_COLOR_BLUE;
}

// Functions
esp_err_t screen_init(
    esp_lcd_panel_io_handle_t *ret_io_handle, 
    esp_lcd_panel_handle_t *ret_panel_handle, 
    lv_display_t **ret_disp) {

    if(ret_io_handle == NULL || ret_panel_handle == NULL || ret_disp == NULL){
        return ESP_ERR_INVALID_ARG;
    }

    // Initialize the physical display and lvgl display
    lcd_st7789_init(ret_io_handle,ret_panel_handle); 
    lcd_lvgl_init(*ret_io_handle, *ret_panel_handle, ret_disp);

    // Set black background on main screen
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, 0);

    // Initialize border illumination object
    illuminate_border_init();
    init_icon();
    return ESP_OK;
}


esp_err_t draw_indication(
    border_zone_t direction,
    const uint8_t *bitmap,
    ui_urgency_t urgency){

        if(direction >= BORDER_ZONE_MAX || bitmap == NULL || urgency >= URGENCY_MAX_UI){
            return ESP_ERR_INVALID_ARG;
        }
        lv_color_t color = get_urgency_color(urgency);
        illuminate_border_zone(direction,color);
        draw_icon(bitmap,color);
        return ESP_OK;     
}




