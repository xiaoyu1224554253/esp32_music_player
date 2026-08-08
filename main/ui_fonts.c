/*
 * 中文字体封装层
 * 实际中文全字库由 CI 的 gen_fonts.py 用 lv_font_conv 生成覆盖本文件。
 * 若 CI 未生成 (无中文字体环境), 则回退到 LVGL 内置默认字体,
 * 通过拷贝 *LV_FONT_DEFAULT 构造合法 lv_font_t 对象,
 * 避免 font_cn_* 为全零结构体导致渲染时调用 NULL 函数指针崩溃。
 */
#include "lvgl.h"

/* 回退字体: 引用 LVGL 内置默认字体对象 lv_font_montserrat_14 (必已编译进库),
 * 拷贝为真实 lv_font_t 对象, 供 LV_FONT_DECLARE(font_cn_*) 声明使用。
 * 注: LVGL 9 的 LV_FONT_DEFAULT 是宏(指针), 不能用于文件作用域常量初始化,
 * 故直接引用具体内置字体对象。 */
extern const lv_font_t lv_font_montserrat_14;
const lv_font_t font_cn_16 = lv_font_montserrat_14;
const lv_font_t font_cn_22 = lv_font_montserrat_14;
