#include <stdio.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_err.h"

#include "daida_compute.h"

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