idf_component_register(
    SRCS "src/lcd_st7789.c" "src/lcd_lvgl_ui.c"
    INCLUDE_DIRS "include" "icons" 
    REQUIRES lvgl esp_lvgl_port esp_lcd driver
)