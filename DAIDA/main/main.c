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


void app_main(void)
{
    // 1. Inicialización de handles para el panel y la comunicación IO
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_handle_t panel_handle = NULL;
    panel_handle = lcd_st7789_init(&io_handle);

    // 2. Inicializar LVGL y el puerto LVGL
    lv_display_t *disp = lcd_lvgl_init(io_handle, panel_handle);
    
    // 3. Crear una etiqueta (label) y mostrar un mensaje en la pantalla
    if (lvgl_port_lock(0)) {
        lv_obj_t *scr = lv_display_get_screen_active(disp);

        lv_obj_t *label = lv_label_create(scr);
        lv_label_set_text(label, "¡LVGL Hello World!");
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

        lvgl_port_unlock();
    }

}