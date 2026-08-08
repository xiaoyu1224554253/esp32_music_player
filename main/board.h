/*
 * ES3C28P 板级引脚定义 (LCDWiki 2.8" ESP32-S3 Display, 带触摸版)
 * 主控: ESP32-S3 N16R8 (16MB Flash + 8MB PSRAM)
 * 屏幕: ILI9341V 4-Line SPI
 * 触摸: FT6336G I2C
 * 音频: I2S (ESP32-S3 内置)
 */
#pragma once
#include "driver/gpio.h"

/* ===== 屏幕 ILI9341V (SPI) ===== */
#define PIN_LCD_CS      GPIO_NUM_10
#define PIN_LCD_DC      GPIO_NUM_46
#define PIN_LCD_SCK     GPIO_NUM_12
#define PIN_LCD_MOSI    GPIO_NUM_11
#define PIN_LCD_MISO    GPIO_NUM_13
#define PIN_LCD_RST     GPIO_NUM_NC   /* 屏幕 RESET 接 EN, 共用复位, 不单独控制 */
#define PIN_LCD_BL      GPIO_NUM_45   /* 背光, 高电平点亮 */
#define PIN_LCD_BL_ON   1

#define LCD_H_RES       320
#define LCD_V_RES       240
#define LCD_SPI_CLOCK_HZ 20000000    /* 参考 freenove 2.8 配置, 保守稳定 */
#define LCD_SPI_HOST    SPI2_HOST

/* ===== 触摸 FT6336G (I2C) ===== */
#define PIN_TP_SDA      GPIO_NUM_16
#define PIN_TP_SCL      GPIO_NUM_15
#define PIN_TP_RST      GPIO_NUM_18   /* 低电平复位 */
#define PIN_TP_INT      GPIO_NUM_17   /* 低电平触发 */
#define TP_I2C_HOST     I2C_NUM_0
#define TP_I2C_ADDR     0x38          /* FT6336 默认地址 (页面未给, 按默认) */
#define TP_I2C_CLOCK_HZ 400000

/* ===== RGB 灯 ===== */
#define PIN_RGB_LED     GPIO_NUM_42   /* 单线 RGB */

/* ===== 按键 ===== */
#define PIN_BOOT_BTN    GPIO_NUM_0    /* BOOT 键 */

/* ===== 音频 I2S ===== */
#define PIN_I2S_EN      GPIO_NUM_1    /* 功放使能 */
#define PIN_I2S_MCLK    GPIO_NUM_4
#define PIN_I2S_BCLK    GPIO_NUM_5
#define PIN_I2S_DOUT    GPIO_NUM_6    /* 数据输出到 DAC/功放 */
#define PIN_I2S_LRCLK   GPIO_NUM_7
#define PIN_I2S_DIN     GPIO_NUM_8    /* 输入(麦克风, 预留) */

/* ===== MicroSD (SDIO) ===== */
#define PIN_SD_CLK      GPIO_NUM_48
#define PIN_SD_CMD      GPIO_NUM_47
#define PIN_SD_D0       GPIO_NUM_38
#define PIN_SD_D1       GPIO_NUM_40
#define PIN_SD_D2       GPIO_NUM_39
#define PIN_SD_D3       GPIO_NUM_41

/* ===== 串口 ===== */
#define PIN_UART_TX     GPIO_NUM_43
#define PIN_UART_RX     GPIO_NUM_44

/* ===== 电池检测 (ADC) ===== */
#define PIN_BAT_ADC     GPIO_NUM_9
