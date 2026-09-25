#include "lcd_lvgl_ui.h"
#include "esp_log.h"
#include "lcd_st7789.h" 
#include "lvgl.h"

#define ICON_X       88
#define ICON_Y       108
#define ICON_WIDTH   64
#define ICON_HEIGHT  64
#define ICON_SIZE    (ICON_WIDTH * ICON_HEIGHT / 8)

static const char *TAG = "lcd_lvgl_ui";
static lv_obj_t *glow_obj = NULL;
static lv_obj_t *black_square = NULL;
static lv_obj_t *icon_img = NULL;
static lv_image_dsc_t icon_dsc;


// Lookup table matching border_zone_t enum
static const border_coords_t BORDER_ZONES_COORDS[] = {
    {  0,   0, 239,   0}, // TOP 
    {160,   0, 239,  80}, // TOP_RIGHT 
    {239,   0, 239, 279}, // RIGHT 
    {160, 199, 239, 279}, // BOTTOM_RIGHT 
    {  0, 279, 239, 279}, // BOTTOM 
    {  0, 199,  80, 279}, // BOTTOM_LEFT 
    {  0,   0,   0, 279}, // LEFT 
    {  0,   0,  80,  80}, // TOP_LEFT 
};

lv_display_t *lcd_lvgl_init(esp_lcd_panel_io_handle_t io_handle, esp_lcd_panel_handle_t panel_handle)
{
    if (io_handle == NULL || panel_handle == NULL) {
        return NULL;
    }

    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    esp_err_t err = lvgl_port_init(&lvgl_cfg);
    if (err != ESP_OK) {
        return NULL;
    }

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = LCD_H_RES * 40, // Pixel buffer size (40 lines)
        .double_buffer = true,
        .hres = LCD_H_RES,
        .vres = LCD_V_RES,
        .monochrome = false,
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        },
        .flags = {
            .buff_dma = true, // DMA-accessible RAM
        }
    };

    lv_display_t *disp = lvgl_port_add_disp(&disp_cfg);
    if (disp == NULL) {
        return NULL;
    }

    return disp;
}

void illuminate_border_init(void)
{
    if (glow_obj == NULL) {
        // Attach to top layer to remain on top of any screen content
        glow_obj = lv_obj_create(lv_layer_top());
        black_square = lv_obj_create(lv_layer_top());
        
        // Clear default styles
        lv_obj_remove_style_all(glow_obj);
        lv_obj_clear_flag(glow_obj, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_clear_flag(black_square, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_radius(glow_obj, 10, 0);

        lv_obj_set_style_bg_opa(glow_obj, LV_OPA_100, 0);
        lv_obj_set_style_pad_all(black_square, 0, 0);
        lv_obj_set_style_border_width(black_square, 0, 0);
        lv_obj_set_style_bg_color(black_square, lv_color_hex(0x000000), 0);
        lv_obj_set_style_bg_opa(black_square, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(black_square, 0, 0);

        // Created hidden until called with coordinates
        lv_obj_add_flag(glow_obj, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(black_square, LV_OBJ_FLAG_HIDDEN);
        // Make the square black 
        lv_obj_set_style_bg_color(black_square, lv_color_hex(0x000000), 0);
    }
}

void illuminate_border_zone(border_zone_t zone, lv_color_t color)
{
    // Validate zone index
    if (zone < 0 || zone >= BORDER_ZONE_MAX) {
        return;
    }

    // Ensure object is initialized before use
    if (glow_obj == NULL) {
        illuminate_border_init();
    }

    // Retrieve coordinates from lookup table based on enum
    int32_t x1 = BORDER_ZONES_COORDS[zone].x1;
    int32_t y1 = BORDER_ZONES_COORDS[zone].y1;
    int32_t x2 = BORDER_ZONES_COORDS[zone].x2;
    int32_t y2 = BORDER_ZONES_COORDS[zone].y2;

    int32_t x = (x1 < x2) ? x1 : x2;
    int32_t y = (y1 < y2) ? y1 : y2;
    int32_t raw_w = (x2 > x1) ? (x2 - x1) : (x1 - x2);
    int32_t raw_h = (y2 > y1) ? (y2 - y1) : (y1 - y2);

    // Thickness of the edge bar
    const int32_t THICKNESS = 20;

    int32_t final_x = x;
    int32_t final_y = y;
    int32_t final_w = raw_w;
    int32_t final_h = raw_h;

    // Detect if this is a flat horizontal line (Top / Bottom)
    if (raw_h == 0) {
        final_h = THICKNESS;
        if (y >= LCD_V_RES - THICKNESS) {
            final_y = LCD_V_RES - THICKNESS; // Bottom edge alignment
        }
        lv_obj_add_flag(black_square, LV_OBJ_FLAG_HIDDEN); // Hide black square for horizontal lines
    }
    // Detect if this is a flat vertical line (Left / Right)
    else if (raw_w == 0) {
        final_w = THICKNESS;
        if (x >= LCD_H_RES - THICKNESS) {
            final_x = LCD_H_RES - THICKNESS; // Right edge alignment
        }
        lv_obj_add_flag(black_square, LV_OBJ_FLAG_HIDDEN); // Hide black square for vertical lines
    }
    // CORNER CASE (Both width and height > 0)
    else {
        final_w = raw_w;
        final_h = raw_h;
        final_x = x;
        final_y = y;

        // Black mask caracteristics 
        int32_t sq_x = 0;
        int32_t sq_y = 0;
        int32_t sq_w = raw_w - THICKNESS;
        int32_t sq_h = raw_h - THICKNESS;

        // Determine which corner we are in to position the black square correctly
        bool is_left = (x < (LCD_H_RES / 2));
        bool is_top  = (y < (LCD_V_RES / 2));

        if (is_top && is_left) {
            // TOP_LEFT
            sq_x = x + THICKNESS;
            sq_y = y + THICKNESS;
        } 
        else if (is_top && !is_left) {
            // TOP_RIGHT
            sq_x = x;
            sq_y = y + THICKNESS;
        } 
        else if (!is_top && is_left) {
            // BOTTOM_LEFT
            sq_x = x + THICKNESS;
            sq_y = y;
        } 
        else {
            // BOTTOM_RIGHT
            sq_x = x;
            sq_y = y;
        }

        lv_obj_set_pos(black_square, sq_x, sq_y);
        lv_obj_set_size(black_square, sq_w, sq_h);

        lv_obj_move_foreground(black_square);
        lv_obj_clear_flag(black_square, LV_OBJ_FLAG_HIDDEN);
    }

    // Update positioning and style for main glow object
    lv_obj_set_pos(glow_obj, final_x, final_y);
    lv_obj_set_size(glow_obj, final_w, final_h);
    
    // Set colors
    lv_obj_set_style_bg_color(glow_obj, color, 0);
    lv_obj_set_style_shadow_color(glow_obj, color, 0);

    // Make visible
    lv_obj_clear_flag(glow_obj, LV_OBJ_FLAG_HIDDEN);
}

void illuminate_border_off(void)
{
    if (glow_obj != NULL) {
        lv_obj_add_flag(glow_obj, LV_OBJ_FLAG_HIDDEN);
    }
}

void init_icon(void)
{
    icon_img = lv_image_create(lv_screen_active());

    lv_obj_set_pos(icon_img, ICON_X, ICON_Y);

    icon_dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
    icon_dsc.header.cf = LV_COLOR_FORMAT_A1;
    icon_dsc.header.flags = 0;
    icon_dsc.header.w = ICON_WIDTH;
    icon_dsc.header.h = ICON_HEIGHT;
    icon_dsc.header.stride = ICON_WIDTH / 8;

    icon_dsc.data_size = ICON_SIZE;
    icon_dsc.data = NULL;
}

void draw_icon(const uint8_t *bitmap, lv_color_t color)
{
    if (icon_img == NULL || bitmap == NULL) {
        return;
    }

    icon_dsc.data = bitmap;

    lv_image_set_src(icon_img, &icon_dsc);

    lv_obj_set_style_image_recolor(icon_img, color, 0);
    lv_obj_set_style_image_recolor_opa(icon_img, LV_OPA_COVER, 0);
}