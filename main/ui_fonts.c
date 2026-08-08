/*
 * 中文字体封装层
 * 实际中文全字库由 CI 的 gen_fonts.py 用 lv_font_conv 生成覆盖本文件。
 * 若 CI 未生成 (无中文字体环境), 则回退到 LVGL 内置默认字体,
 * 通过拷贝 *LV_FONT_DEFAULT 构造合法 lv_font_t 对象,
 * 避免 font_cn_* 为全零结构体导致渲染时调用 NULL 函数指针崩溃。
 */
#include "lvgl.h"

/* 回退字体: 直接指向 LVGL 内置默认字体指针 LV_FONT_DEFAULT (地址常量, 合法初始化)。
 * 中文全字库应由 CI 的 gen_fonts.py 生成覆盖本文件 (届时 font_cn_* 为真实 lv_font_t 对象)。
 * 此处用指针别名, 避免内置字体对象无法做文件作用域常量拷贝的问题。 */
#include "lvgl.h"
const lv_font_t *font_cn_16 = LV_FONT_DEFAULT;
const lv_font_t *font_cn_22 = LV_FONT_DEFAULT;
