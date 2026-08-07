#pragma once
#include "lvgl.h"
#include "esp_err.h"
#include "esp_lcd_panel_dev.h"

esp_err_t disp_driver_init(void);
void disp_driver_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *color_p);
esp_lcd_panel_handle_t disp_driver_get_panel(void);
