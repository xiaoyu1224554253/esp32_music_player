#include <Arduino.h>
#include <LovyanGFX.hpp>
#include <Wire.h>
#include "config.h"
#include "lgfx_config.h"
#include "ui.h"
#include "player.h"
#include "audio_i2s.h"

static LGFX lcd;
static UI ui;

// FT6336 原始触摸读取（绕过 LovyanGFX 自动校准，自行映射到显示坐标系）
// 物理面板: 宽 240(X:0..239) 高 320(Y:0..319)，横屏 rotation=1 显示 320x240
static bool ft6336_read(uint16_t* px, uint16_t* py) {
    Wire.beginTransmission(TP_I2C_ADDR);
    Wire.write(0x02);  // 触摸点数 + 第1点 X/Y
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom((int)TP_I2C_ADDR, 4) != 4) return false;
    uint8_t b[4];
    for (int i = 0; i < 4; i++) b[i] = Wire.read();
    uint8_t points = b[0] >> 2;
    if (points == 0) return false;
    *px = ((uint16_t)(b[0] & 0x0F) << 8) | b[1];  // 物理 X (0..239)
    *py = ((uint16_t)(b[2] & 0x0F) << 8) | b[3];  // 物理 Y (0..319)
    return true;
}

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

    // FT6336 裸读取使用的 I2C（与 LGFX 触摸同引脚 SDA16/SCL15）
    Wire.begin(PIN_TP_SDA, PIN_TP_SCL);

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

    uint16_t phys_x = 0, phys_y = 0;
    if (ft6336_read(&phys_x, &phys_y)) {
        // 物理面板 240(宽,X) x 320(高,Y)，横屏显示 320x240
        // 映射: 显示X = 物理Y, 显示Y = 物理X（rotation=1）
        uint16_t dx = phys_y;            // 0..319 -> 显示 X
        uint16_t dy = phys_x;            // 0..239 -> 显示 Y
        Serial.printf("phys -> %d,%d | disp(no-flip) -> %d,%d | flip -> %d,%d\n",
                      phys_x, phys_y, dx, dy, 319 - dx, 239 - dy);
        // 当前采用: 无额外翻转（如仍反，改下一行）
        ui.onTouch(dx, dy, true);
        was_pressed = true;
    } else if (was_pressed) {
        was_pressed = false;
    }

    ui.tick();
    delay(10);
}
