#include "lcd_st7789.h"

static const char *TAG = "LCD_ST7789";

esp_err_t lcd_st7789_init(esp_lcd_panel_io_handle_t *ret_io_handle, esp_lcd_panel_handle_t *ret_panel_handle) {
    esp_err_t ret = ESP_OK;

    // 1. Validate input parameters
    if (ret_io_handle == NULL || ret_panel_handle == NULL) {
        ESP_LOGE(TAG, "Invalid arguments: NULL pointer passed");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Initializing SPI bus...");
    
    // SPI bus configuration
    spi_bus_config_t buscfg = {
        .sclk_io_num = PIN_NUM_CLK,
        .mosi_io_num = PIN_NUM_DIN,
        .miso_io_num = PIN_NUM_MISO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_H_RES * 40 * sizeof(uint16_t),
    };

    // Initialize SPI bus
    ret = spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO);
    ESP_RETURN_ON_ERROR(ret, TAG, "Failed to initialize SPI bus");

    ESP_LOGI(TAG, "Installing panel IO...");

    // ST7789 panel IO configuration
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = PIN_NUM_CD,
        .cs_gpio_num = PIN_NUM_CS,
        .pclk_hz = 24 * 1000 * 1000, 
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };

    ret = esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle);
    ESP_RETURN_ON_ERROR(ret, TAG, "Failed to create panel IO");

    ESP_LOGI(TAG, "Installing ST7789 driver...");

    // ST7789 device configuration
    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = PIN_NUM_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .bits_per_pixel = 16,
    };

    ret = esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle);
    ESP_RETURN_ON_ERROR(ret, TAG, "Failed to create ST7789 panel driver");

    // Reset and initialize display panel
    ESP_LOGI(TAG, "Resetting and initializing display panel...");

    ret = esp_lcd_panel_reset(panel_handle);
    ESP_RETURN_ON_ERROR(ret, TAG, "Failed to reset panel");

    ret = esp_lcd_panel_init(panel_handle);
    ESP_RETURN_ON_ERROR(ret, TAG, "Failed to initialize panel");

    ret = esp_lcd_panel_set_gap(panel_handle, 0, 20);
    ESP_RETURN_ON_ERROR(ret, TAG, "Failed to set panel gap");

    ret = esp_lcd_panel_invert_color(panel_handle, true);
    ESP_RETURN_ON_ERROR(ret, TAG, "Failed to invert color");

    ret = esp_lcd_panel_disp_on_off(panel_handle, true);
    ESP_RETURN_ON_ERROR(ret, TAG, "Failed to turn on display");

    // Assign handles to the pointers passed by the user
    *ret_io_handle = io_handle;
    *ret_panel_handle = panel_handle;

    ESP_LOGI(TAG, "ST7789 LCD initialized successfully");
    return ESP_OK;
}
