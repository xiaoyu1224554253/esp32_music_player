#include <Arduino.h>
#include <LovyanGFX.hpp>
#include "config.h"
#include "lgfx_config.h"
#include "ui.h"
#include "player.h"
#include "audio_i2s.h"

static LGFX lcd;
static UI ui;

void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println("ESP32-S3 Music Player (Arduino) starting...");

    pinMode(PIN_LCD_BL, OUTPUT);
    digitalWrite(PIN_LCD_BL, LOW);

    lcd.init();
    lcd.setBrightness(128);
    lcd.setRotation(LCD_ROTATION);
    lcd.fillScreen(C_BG);

    // 音频 I2S 初始化（失败不影响 UI）
    if (audio_i2s_init(PIN_I2S_BCLK, PIN_I2S_LRCLK, PIN_I2S_DOUT, PIN_I2S_MCLK, 44100)) {
        Serial.println("I2S audio init ok");
    } else {
        Serial.println("I2S audio init failed (ignore)");
    }

    // LovyanGFX 内置触摸（FT5x06）自动按 rotation 校准坐标

    ui.begin(&lcd);
    ui.render();

    digitalWrite(PIN_LCD_BL, HIGH);
    Serial.printf("LCD init ok, %dx%d\n", lcd.width(), lcd.height());
}

void loop() {
    static bool was_pressed = false;

    uint16_t tx = 0, ty = 0;
    bool pressed = lcd.getTouch(&tx, &ty);
    if (pressed) {
        // LovyanGFX 内置 FT5x06 已按 rotation 校准，但 FT6336 原始坐标轴与显示相反，
        // 先做轴翻转，再交给 UI（若仍整体偏移，调 offset_rotation）
        uint16_t w = lcd.width(), h = lcd.height();
        Serial.printf("raw touch -> %d,%d (w=%d h=%d)\n", tx, ty, w, h);
        tx = w - 1 - tx;
        ty = h - 1 - ty;
        Serial.printf("mapped touch -> %d,%d\n", tx, ty);
        ui.onTouch(tx, ty, true);
        was_pressed = true;
    } else if (was_pressed) {
        was_pressed = false;
    }

    ui.tick();
    delay(10);
}
