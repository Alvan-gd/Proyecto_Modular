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

static const char *TAG = "main";

void app_main(void)
{
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_handle_t panel_handle = NULL;

    // 1. Initialize ST7789 Display Driver
    lcd_st7789_init(&io_handle, &panel_handle);

    // 2. Initialize LVGL
    lv_display_t *disp = lcd_lvgl_init(io_handle, panel_handle);
    if (disp == NULL) {
        return;   
    }
    // Set black background on main screen
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, 0);

    // 3. Initialize border illumination object
    illuminate_border_init();
    init_icon();

    border_zone_t zone = TOP;
    lv_color_t COLORS[] = {
        ST7789_COLOR_GREEN,
        ST7789_COLOR_YELLOW,
        ST7789_COLOR_RED,
        ST7789_COLOR_BLUE,
    };


    size_t num_colors = sizeof(COLORS) / sizeof(COLORS[0]);
    while (1) {
        for (uint8_t color_index = 0; color_index < num_colors; color_index++) {
            zone = TOP;
            
            do {
                if (lvgl_port_lock(0)) {
                    illuminate_border_zone(zone, COLORS[color_index]);
                     switch (zone) {
                        case TOP:
                            draw_icon(alarm_bitmap, COLORS[color_index]);
                            break;
                        case TOP_RIGHT:
                            draw_icon(voice_bitmap, COLORS[color_index]);
                            break;
                        case RIGHT:
                            draw_icon(bell_bitmap, COLORS[color_index]);
                            break;
                        case BOTTOM:
                            draw_icon(danger_bitmap, COLORS[color_index]);
                            break;
                        case LEFT:
                            draw_icon(phone_bitmap, COLORS[color_index]);
                            break;
                        default:
                            break;
                    }
                    lvgl_port_unlock();
                }
                vTaskDelay(pdMS_TO_TICKS(1000));
                zone = (zone + 1) % BORDER_ZONE_MAX;
            } while (zone != TOP); 
        }
    }

}

