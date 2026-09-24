# LCD ST7789 Component
This file describes the structure and proper use of the contents on the 'lcd_st7789.c' and 'lcd_lvgl_ui.c' files.
The functions and characteristics in this component are designed exclusively to cover the needs and specificactions of DAIDA, so the implementation is not meant to be a general purpose library for the display.

## Components inside /lcd_stt7789
### lcd_st7789
This component's only purpose is to generate the initial configuration of the physical display and manage it's low-level characteristics:
- SPI Configuration
- Display Initialization
- Display configuration
- General display hardware related functions (e.g. the sleep mode)
All these features are managed through the esp_lcd native library in the esp-idf environment.
### lcd_lvgl_ui
This component is meant to manage the user interface (UI) related functions, such as the graphics for the display, using the integrated LVGL software. 
LVGL (Light and Versatile Graphic Library) is an open source library meant to create and manage lightweight, fast GUIs, ideal for embedded systems.