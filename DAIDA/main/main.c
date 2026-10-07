#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"

// Hardware Drivers & Libraries
#include "lcd_st7789.h"
#include "lcd_lvgl_ui.h"
#include "drv2605.h"
#include "driver/i2c_master.h"

#include "haptic.h"
#include "screen.h"

static const char *TAG = "MAIN";

void app_main(void)
{
    drv2605_dev_t *haptic_dev = NULL;
    haptic_init(&haptic_dev);
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_handle_t panel_handle = NULL;
    lv_display_t* disp = NULL;
    screen_init(&io_handle, &panel_handle, &disp);
    draw_indication(SECTOR_0,alarm_bitmap,HIGH_URGENCY_UI);
    
}