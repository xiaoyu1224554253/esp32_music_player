#pragma once

#include <LovyanGFX.hpp>
#include "config.h"

class LGFX : public lgfx::LGFX_Device {
    lgfx::Panel_ILI9341 _panel_instance;
    lgfx::Bus_SPI _bus_instance;
    lgfx::Touch_FT5x06 _touch_instance;
public:
    LGFX(void) {
        {
            auto cfg = _bus_instance.config();
            cfg.spi_host = SPI2_HOST;
            cfg.spi_mode = 0;
            cfg.freq_write = 8000000;
            cfg.freq_read  = 6000000;
            cfg.pin_sclk = PIN_LCD_SCK;
            cfg.pin_mosi = PIN_LCD_MOSI;
            cfg.pin_miso = PIN_LCD_MISO;
            cfg.pin_dc   = PIN_LCD_DC;
            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }
        {
            auto cfg = _panel_instance.config();
            cfg.pin_cs   = PIN_LCD_CS;
            cfg.pin_rst  = PIN_LCD_RST;
            cfg.pin_busy = -1;
            cfg.panel_width  = 240;
            cfg.panel_height = 320;
            cfg.offset_x = 0;
            cfg.offset_y = 0;
            cfg.offset_rotation = 0;
            cfg.dummy_read_pixel = 8;
            cfg.dummy_read_bits  = 1;
            cfg.readable = true;
            cfg.invert = true;
            cfg.rgb_order = false;          // BGR
            cfg.dlen_16bit = false;
            cfg.bus_shared = true;
            _panel_instance.config(cfg);
        }
        {
            // FT6336 触摸（FT5x06 兼容），由 LovyanGFX 自动按 rotation 校准坐标
            auto cfg = _touch_instance.config();
            cfg.x_min = 0;
            cfg.x_max = 319;
            cfg.y_min = 0;
            cfg.y_max = 239;
            cfg.pin_int = PIN_TP_INT;
            cfg.bus_shared = true;
            cfg.offset_rotation = 0;
            // I2C 地址 0x38，使用默认 Wire (i2c_port=0)
            cfg.i2c_port = 0;
            cfg.i2c_addr = TP_I2C_ADDR;
            cfg.pin_sda = PIN_TP_SDA;
            cfg.pin_scl = PIN_TP_SCL;
            cfg.freq = 100000;
            _touch_instance.config(cfg);
            // 触摸改由 main.cpp 自行裸读 FT6336（见 ft6336_read），不交给 LGFX 自动校准
            // _panel_instance.setTouch(&_touch_instance);
        }
        setPanel(&_panel_instance);
    }
};
