#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "lcd_st7789.h"
#include "esp_lvgl_port.h"


void app_main(void)
{
    // Inicialización de handles para el panel y la comunicación IO
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_handle_t panel_handle = NULL;
    // Inicialización del panel ST7789 y obtenemos su handle y el handle de IO
    panel_handle = lcd_st7789_init(&io_handle);

    // 2. Inicializar el motor LVGL v9
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));

    // 3. Vincular el panel ST7789 a LVGL
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = 240 * 40, // Buffer de píxeles (40 líneas)
        .double_buffer = true,
        .hres =  LCD_H_RES,             // Ancho de pantalla
        .vres =  LCD_V_RES,             // Alto de pantalla
        .monochrome = false,
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        },
        .flags = {
            .buff_dma = true,    // Usar RAM accesible por DMA
        }
    };

    lv_display_t *disp = lvgl_port_add_disp(&disp_cfg);

    // 4. Crear una interfaz sencilla 
    if (lvgl_port_lock(0)) {
        lv_obj_t *scr = lv_display_get_screen_active(disp);

        lv_obj_t *label = lv_label_create(scr);
        lv_label_set_text(label, "¡LVGL Hello World!");
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

        lvgl_port_unlock();
    }

}