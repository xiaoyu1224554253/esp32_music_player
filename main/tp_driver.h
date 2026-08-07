#pragma once
#include "lvgl/lvgl.h"
#include "esp_err.h"

esp_err_t tp_driver_init(void);
void tp_driver_read(lv_indev_drv_t *drv, lv_indev_data_t *data);
