#pragma once
#include "lvgl/lvgl.h"
#include "esp_err.h"

esp_err_t disp_driver_init(void);
void disp_driver_flush(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p);
esp_lcd_panel_handle_t disp_driver_get_panel(void);
