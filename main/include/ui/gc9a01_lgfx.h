#pragma once

#include <LovyanGFX.hpp>
#include <Wire.h>
#include <driver/i2c.h>
#include "board/board_pins.h"

/*
 * ES3C28P 显示驱动: ILI9341V 320x240 (4-Line SPI)
 * 移植自原 GC9A01 圆屏配置 (AppStateV2.5.0)。
 */

#define SCR_W 320
#define SCR_H 240

class LGFX_ES3C28P : public LGFX_Device {
    lgfx::Panel_ILI9341 _panel_instance;
    lgfx::Bus_SPI _bus_instance;
    lgfx::Light_PWM _light_instance;
    lgfx::Touch_FT5x06 _touch_instance;

public:
    LGFX_ES3C28P(void) {
        auto cfg = _bus_instance.config();
        cfg.spi_host = SPI2_HOST;
        cfg.spi_mode = 0;
        cfg.freq_write = 40000000;
        cfg.freq_read  = 16000000;
        cfg.spi_3wire  = true;
        cfg.pin_sclk = PIN_SPI_UI_SCK;
        cfg.pin_mosi = PIN_SPI_UI_MOSI;
        cfg.pin_miso = PIN_SPI_UI_MISO;
        cfg.pin_dc   = PIN_TFT_DC;
        _bus_instance.config(cfg);
        _panel_instance.setBus(&_bus_instance);

        auto pnl = _panel_instance.config();
        pnl.pin_cs   = PIN_TFT_CS;
        pnl.pin_rst  = PIN_TFT_RST;
        pnl.pin_busy = -1;
        pnl.panel_width  = 240;   // ILI9341 原生逻辑分辨率 240x320 (竖)
        pnl.panel_height = 320;
        pnl.offset_x = 0;
        pnl.offset_y = 0;
        pnl.offset_rotation = 0;
        pnl.dummy_read_pixel = 8;
        pnl.bus_shared = true;
        pnl.rgb_order = false;
        pnl.invert = true;
        _panel_instance.config(pnl);

        _panel_instance.setRotation(1);   // ES3C28P ILI9341: 横屏 320x240
        _panel_instance.setBrightness(255);
        setPanel(&_panel_instance);

        // ---- FT6336G 触摸 ----
        auto tp = _touch_instance.config();
        tp.i2c_port   = I2C_NUM_1;
        tp.i2c_addr   = board::TP_I2C_ADDR;
        tp.pin_sda    = board::PIN_TP_SDA;
        tp.pin_scl    = board::PIN_TP_SCL;
        tp.pin_int    = board::PIN_TP_INT;
        tp.pin_rst    = board::PIN_TP_RST;
        tp.x_min      = 0;
        tp.x_max      = SCR_W - 1;
        tp.y_min      = 0;
        tp.y_max      = SCR_H - 1;
        tp.bus_shared = true;
        tp.freq       = 400000;
        _touch_instance.config(tp);
        _panel_instance.setTouch(&_touch_instance);
    }
};

// 全局显示对象 (LGFX_ES3C28P 实例)，在 ui.cpp 中定义，其它 TU 通过 extern 引用。
// 屏幕与触摸共用同一实例，避免多实例导致触摸读不到屏幕状态。
extern LGFX tft;

// 原项目别名：LGFX = LGFX_ES3C28P
typedef LGFX_ES3C28P LGFX;
