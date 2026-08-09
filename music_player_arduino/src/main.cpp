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

    ui.begin(&lcd);
    ui.render();

    digitalWrite(PIN_LCD_BL, HIGH);
    Serial.printf("LCD init ok, %dx%d\n", lcd.width(), lcd.height());
}

void loop() {
    static bool was_pressed = false;

    uint16_t rx = 0, ry = 0;
    if (lcd.getTouch(&rx, &ry)) {
        // LGFX offset_rotation=0 返回物理坐标，X/Y 轴方向均与显示相反，
        // 同时轴顺序为物理 240x320。映射到显示 320x240 (rotation=1)：
        //   显示X = 319 - rawX, 显示Y = 239 - rawY
        uint16_t dx = 319 - rx;
        uint16_t dy = 239 - ry;
        ui.onTouch(dx, dy, true);
        was_pressed = true;
    } else if (was_pressed) {
        was_pressed = false;
    }

    ui.tick();
    delay(10);
}
