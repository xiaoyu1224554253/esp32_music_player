#pragma once
#include "lvgl.h"

/* 全中文字库字体声明 (由 gen_fonts.py 生成 ui_fonts.c; 回退时为指向内置默认字体的指针) */
extern const lv_font_t *font_cn_16;
extern const lv_font_t *font_cn_22;

void ui_init(void);
