/*
 * 中文字体封装层
 * 实际中文全字库由 CI 的 gen_fonts.py 用 lv_font_conv 生成覆盖本文件。
 * 若 CI 未生成 (无中文字体环境), 则回退到 LVGL 内置合法字体,
 * 避免 font_cn_* 为全零结构体导致渲染时调用 NULL 函数指针崩溃。
 */
#include "lvgl.h"

#ifndef font_cn_16
/* 回退: 引用 LVGL 内置默认字体 (LV_FONT_DEFAULT 必定已编译进库, 避免空壳崩溃)。
 * 注: 项目 lv_conf.h 的 LVGL_USE_* 宏不影响 LVGL 库自身编译(库用 Kconfig 配置),
 * 因此这里用保证存在的 LV_FONT_DEFAULT, 而非可能未编译的 montserrat_16/22。 */
#define font_cn_16 (*LV_FONT_DEFAULT)
#define font_cn_22 (*LV_FONT_DEFAULT)
#endif
