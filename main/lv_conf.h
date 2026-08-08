/**
 * LVGL 9.5 配置文件 (精简版, 适配 ESP32-S3 + 全中文字库)
 * 通过 -DLV_CONF_INCLUDE_SIMPLE 让 lvgl.h 以 "lv_conf.h" 形式包含本文件
 */
#if !defined(LV_CONF_H)
#define LV_CONF_H

#include <stdint.h>

/* 颜色深度 (16bit RGB565, 与 ILI9341 匹配) */
#define LV_COLOR_DEPTH 16

/* 内存: 使用自定义分配器(LVGL_MEM_CUSTOM), 由 sdkconfig 控制 */
#ifndef LV_MEM_CUSTOM
#define LV_MEM_CUSTOM 1
#endif
#ifndef LV_MEM_SIZE
#define LV_MEM_SIZE (1024 * 1024 * 3)   /* 3MB, 实际由 PSRAM 提供 */
#endif

/* 默认字体: 留给 LVGL 组件默认 (内置 lv_font_default); 中文由 ui_fonts.c 的 font_cn_16 覆盖 */

/* 启用需要的核心 widget (LVGL 9 命名: 前缀 LVGL_USE_, 原 LV_USE_ 在 v9 无效) */
#define LV_USE_LOG 0
#define LVGL_USE_OBJ 1
#define LVGL_USE_LABEL 1
#define LVGL_USE_BTN 1
#define LVGL_USE_LIST 1
#define LVGL_USE_TEXTAREA 1
#define LVGL_USE_BAR 1
#define LVGL_USE_TABVIEW 1
#define LVGL_USE_IMG 0
#define LVGL_USE_ARC 0
#define LVGL_USE_SLIDER 0
#define LVGL_USE_SWITCH 0
#define LVGL_USE_CHECKBOX 0
#define LVGL_USE_DROPDOWN 0
#define LVGL_USE_ROLLER 0
#define LVGL_USE_TABLE 0
#define LVGL_USE_CHART 0
#define LVGL_USE_CALENDAR 0
#define LVGL_USE_SPINNER 0
#define LVGL_USE_SPINBOX 0
#define LVGL_USE_KEYBOARD 0
#define LVGL_USE_MSGBOX 0

/* 文本/字体 */
#define LV_TXT_ENC LV_TXT_ENC_UTF8
#define LV_USE_USER_DATA 1
#define LV_FONT_FMT_TXT_LARGE 1

/* 显示刷新 */
#define LV_DISP_DEF_REFR_PERIOD 10      /* ms */
#define LV_INDEV_DEF_READ_PERIOD 10

/* tick: 由 ESP-IDF 定时器提供 (CONFIG_LVGL_TICK_CUSTOM) */
#define LV_TICK_CUSTOM 1
#define LV_TICK_CUSTOM_SYS_TIME_EXPR (lv_tick_get())

/* 内置字体 (回退用, 避免占位字体为全零结构体导致崩溃) */
#define LVGL_USE_FONT_MONTSERRAT_16 1
#define LVGL_USE_FONT_MONTSERRAT_22 1
#define LVGL_USE_FONT_MONTSERRAT_14 1

/* 杂项 */
#define LV_USE_ASSERT_NULL 0
#define LV_USE_ASSERT_MALLOC 0
#define LV_USE_ASSERT_STYLE 0
#define LV_USE_ASSERT_MEM_INTEGRITY 0
#define LV_USE_ASSERT_OBJ 0

#endif /* LV_CONF_H */
