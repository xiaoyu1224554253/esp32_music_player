#pragma once

#include <Arduino.h>

/* ===== 硬件引脚 (LCDWiki 2.8" ESP32-S3 Display, freenove-esp32s3-display-2.8-lcd) ===== */

// ILI9341V SPI
#define PIN_LCD_CS      10
#define PIN_LCD_DC      46
#define PIN_LCD_SCK     12
#define PIN_LCD_MOSI    11
#define PIN_LCD_MISO    13
#define PIN_LCD_RST     (-1)
#define PIN_LCD_BL      45

// FT6336G I2C
#define PIN_TP_SDA      16
#define PIN_TP_SCL      15
#define PIN_TP_RST      18
#define PIN_TP_INT      17
#define TP_I2C_ADDR     0x38

// 音频 I2S
#define PIN_I2S_MCLK    4
#define PIN_I2S_BCLK    5
#define PIN_I2S_DOUT    6
#define PIN_I2S_LRCLK   7

// 背光、RGB、按键
#define PIN_RGB_LED     42
#define PIN_BOOT_BTN    0

/* ===== 屏幕参数 ===== */
#define SCREEN_WIDTH    320
#define SCREEN_HEIGHT   240
#define LCD_ROTATION    1   // 横屏: 0/1 视实际方向调整

/* ===== UI 尺寸 ===== */
#define UI_STATUS_H     20
#define UI_NAV_H        36
#define UI_CONTENT_Y    UI_STATUS_H
#define UI_CONTENT_H    (SCREEN_HEIGHT - UI_STATUS_H - UI_NAV_H)

/* ===== 颜色 (RGB565) ===== */
inline uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

#define C_BG            rgb(10, 8, 18)         // #0a0812 更深紫黑背景
#define C_SURFACE       rgb(18, 16, 28)        // #12101c 主卡片
#define C_SURFACE2      rgb(28, 24, 42)        // #1c182a 内嵌卡片/输入框
#define C_PRIMARY       rgb(130, 110, 255)     // #826eff 主紫色
#define C_ACCENT        rgb(200, 90, 255)      // #c85aff 粉紫强调
#define C_TEXT          rgb(255, 255, 255)     // 主文字
#define C_TEXT2         rgb(185, 180, 210)     // 次要文字
#define C_TEXT3         rgb(110, 105, 135)     // 更淡文字
#define C_DISABLED      rgb(70, 65, 95)
#define C_ONLINE        rgb(120, 150, 255)     // 网络来源标签
#define C_WHITE         0xFFFF
#define C_BLACK         0x0000
#define C_PINK          rgb(255, 120, 200)     // 封面渐变模拟
#define C_BLUE          rgb(80, 130, 255)

/* ===== 交互 ===== */
#define TAB_COUNT       4
