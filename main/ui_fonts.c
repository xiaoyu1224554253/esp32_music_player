/*
 * 全中文字库占位文件
 * 实际字体由 build 流程中的 gen_fonts.py 用 lv_font_conv 生成并覆盖本文件。
 * 此处提供最小占位字体, 保证仓库可直接编译出骨架 (无中文渲染)。
 * 完整中文需 CI 生成: python gen_fonts.py
 */
#include "lvgl/lvgl.h"

/* 占位字体: 合法的最小 lv_font_t 对象。
 * CI 中的 gen_fonts.py 会生成完整中文字库并整体覆盖本文件。
 * 若 CI 未生成, 此处保证编译通过 (中文显示为默认字形)。 */
const lv_font_t font_cn_16 = {0};
const lv_font_t font_cn_22 = {0};
