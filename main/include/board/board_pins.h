#pragma once

#include <Arduino.h>

/*
 * ES3C28P 板级引脚定义 (LCDWiki 2.8" ESP32-S3 Display, 带触摸版)
 * 主控: ESP32-S3 N16R8 (16MB Flash + 8MB PSRAM)
 * 屏幕: ILI9341V 320x240 4-Line SPI
 * 触摸: FT6336G I2C
 * 音频: I2S (ESP32-S3 内置)
 *
 * 移植自原 GC9A01 圆屏 PCB1(MCP23017) 项目。
 * 本板无 MCP23017 / RC522 NFC / EC06 编码器 / 实体按键(除 BOOT)。
 * 所有 UI 交互通过 FT6336 电容触摸完成。
 */

namespace board {

// ---- UI SPI (TFT) ----
constexpr int PIN_SPI_CLK  = 12;   // SCK
constexpr int PIN_SPI_MISO = 13;   // MISO
constexpr int PIN_SPI_MOSI = 11;   // MOSI
constexpr int PIN_TFT_CS   = 10;
constexpr int PIN_TFT_DC   = 46;
constexpr int PIN_TFT_RST  = -1;   // 屏幕 RESET 接 EN, 共用复位

// ---- 背光 / 功放 (直连 GPIO, 无 MCP23017) ----
constexpr int PIN_LCD_BL       = 45;  // 背光, 高电平点亮
constexpr int PIN_I2S_EN       = 1;   // 功放使能, 高电平开启

// ---- SD (SDIO 4-bit) ----
constexpr int PIN_SD_CLK  = 48;
constexpr int PIN_SD_CMD  = 47;
constexpr int PIN_SD_D0   = 38;
constexpr int PIN_SD_D1   = 40;
constexpr int PIN_SD_D2   = 39;
constexpr int PIN_SD_D3   = 41;

// SD 引脚裸宏（兼容 storage.cpp 原 PIN_SD_* 用法）
#define PIN_SD_CLK   board::PIN_SD_CLK
#define PIN_SD_CMD   board::PIN_SD_CMD
#define PIN_SD_D0    board::PIN_SD_D0
#define PIN_SD_D1    board::PIN_SD_D1
#define PIN_SD_D2    board::PIN_SD_D2
#define PIN_SD_D3    board::PIN_SD_D3

// 原项目用 1-bit SPI SD, 本板 SDIO 4-bit。保留 SPI SD 宏名映射到 SDIO 引脚，
// 但 SDIO 模式需要单独初始化(SDMMC)。 lib_sd.cpp 会按本板适配。
constexpr int PIN_SD_CS   = PIN_SD_D3; // 4-bit 模式下 CS=D3, 实际由 SDMMC 接管
constexpr int PIN_SPI_SD_SCK  = PIN_SD_CLK;
constexpr int PIN_SPI_SD_MISO = PIN_SD_D0;
constexpr int PIN_SPI_SD_MOSI = PIN_SD_CMD;

// ---- 触摸 FT6336G (I2C) ----
constexpr int PIN_TP_SDA = 16;
constexpr int PIN_TP_SCL = 15;
constexpr int PIN_TP_RST = 18;   // 低电平复位
constexpr int PIN_TP_INT = 17;   // 低电平触发
constexpr uint8_t TP_I2C_ADDR = 0x38;

// ---- I2S 音频 ----
constexpr int PIN_I2S_BCLK  = 5;
constexpr int PIN_I2S_DOUT  = 6;   // 数据输出到 DAC/功放
constexpr int PIN_I2S_LRCK  = 7;
constexpr int PIN_I2S_MCLK  = 4;

// ---- RGB 灯 ----
constexpr int PIN_WS2812 = 42;

// ---- 实体按键 (仅 BOOT) ----
constexpr int PIN_BOOT_BTN = 0;    // BOOT 键, 低电平按下

// ---- 电池 ADC ----
constexpr int PIN_BAT_ADC = 9;

// ---- 霍尔 (本板无, 禁用) ----
constexpr int PIN_HALL_OUT = -1;

// 以下为原 MCP23017 编码器/按键/蓝牙/NFC 引脚 —— 本板全部禁用(-1)。
// 交互改由 FT6336 触摸驱动层模拟这些"虚拟按键"事件。
constexpr int PIN_EC06_A = -1;
constexpr int PIN_EC06_B = -1;
constexpr int PIN_POWER_PLAY = PIN_BOOT_BTN; // BOOT 复用为播放/暂停

// 原 4 个物理按键全部禁用, 由触摸代替
constexpr int PIN_KEY_MODE  = -1;
constexpr int PIN_KEY_PLAY  = -1;
constexpr int PIN_KEY_PREV  = -1;
constexpr int PIN_KEY_NEXT  = -1;
constexpr int PIN_KEY_VOLDN = -1;
constexpr int PIN_KEY_VOLUP = -1;
constexpr int PIN_KEY_MCP_EC06_E = -1;
constexpr int PIN_KEY_DISABLED    = -1;
constexpr int PIN_KEY_MCP_BACK_MODE = -1;
constexpr int PIN_KEY_MCP_PREV_NFC  = -1;
constexpr int PIN_KEY_MCP_NEXT_LIST = -1;

// NFC 相关 (无硬件, 全部禁用)
constexpr int PIN_NFC_CS  = -1;
constexpr int PIN_NFC_IRQ = -1;
#define PIN_RC522_CS   board::PIN_NFC_CS   // 兼容 nfc.cpp 裸宏

// 蓝牙电源控制 (无, 禁用)
constexpr int PIN_POWER_CTRL = -1;
constexpr int PIN_POWER_HOLD = -1;

// 电磁铁 (无, 禁用)
constexpr int PIN_SOLENOID  = -1;

} // namespace board

// ============================================================
// 兼容旧代码的宏名（直接映射到本板引脚）
// ============================================================

// UI SPI
#define PIN_SPI_UI_SCK   board::PIN_SPI_CLK
#define PIN_SPI_UI_MISO  board::PIN_SPI_MISO
#define PIN_SPI_UI_MOSI  board::PIN_SPI_MOSI

#define PIN_TFT_CS       board::PIN_TFT_CS
#define PIN_TFT_DC       board::PIN_TFT_DC
#define PIN_TFT_RST      board::PIN_TFT_RST

// SD SPI (宏名保留, 实际 SDIO 模式)
#define PIN_SPI_SD_SCK   board::PIN_SPI_SD_SCK
#define PIN_SPI_SD_MISO  board::PIN_SPI_SD_MISO
#define PIN_SPI_SD_MOSI  board::PIN_SPI_SD_MOSI
#define PIN_SD_CS        board::PIN_SD_CS

// 编码器 / 按键
#define PIN_EC06_A       board::PIN_EC06_A
#define PIN_EC06_B       board::PIN_EC06_B
#define PIN_POWER_PLAY   board::PIN_POWER_PLAY

// I2S
#define PIN_I2S_BCLK     board::PIN_I2S_BCLK
#define PIN_I2S_DOUT     board::PIN_I2S_DOUT
#define PIN_I2S_LRCK     board::PIN_I2S_LRCK
#define PIN_I2S_MCLK     board::PIN_I2S_MCLK

// Other
#define PIN_WS2812       board::PIN_WS2812
#define PIN_POWER_CTRL   board::PIN_POWER_CTRL
#define PIN_BAT_ADC      board::PIN_BAT_ADC
#define PIN_HALL_OUT     board::PIN_HALL_OUT

// 屏幕物理尺寸
#define LCD_H_RES        320
#define LCD_V_RES        240

// 背光控制宏
#define LCD_BL_PIN       board::PIN_LCD_BL
#define LCD_BL_ON        1
