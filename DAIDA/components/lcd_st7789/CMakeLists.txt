idf_component_register(
    SRCS "lcd_st7789.c" "lcd_lvgl_ui.c"
    INCLUDE_DIRS "include" "icons"
    REQUIRES lvgl esp_lvgl_port esp_lcd driver
)