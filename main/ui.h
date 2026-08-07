#pragma once
#include "lvgl/lvgl.h"

/* 全中文字库字体声明 (由 gen_fonts.py 生成 ui_fonts.c) */
LV_FONT_DECLARE(font_cn_16);
LV_FONT_DECLARE(font_cn_22);

void ui_init(void);
