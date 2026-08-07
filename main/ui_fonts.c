/*
 * 全中文字库占位文件
 * 实际字体由 build 流程中的 gen_fonts.py 用 lv_font_conv 生成并覆盖本文件。
 * 此处提供最小占位字体, 保证仓库可直接编译出骨架 (无中文渲染)。
 * 完整中文需 CI 生成: python gen_fonts.py
 */
#include "lvgl/lvgl.h"

/* 占位: 仅含 ASCII, 中文显示会 fallback 为方框。CI 生成后覆盖。 */
LV_FONT_DECLARE(lv_font_montserrat_16);
const lv_font_t font_cn_16 = lv_font_montserrat_16;
const lv_font_t font_cn_22 = lv_font_montserrat_16;
