#include <LovyanGFX.hpp>
#include "config.h"
#include "lgfx_config.h"
#include "touch_driver.h"
#include "ui.h"
#include "player.h"
#include "audio_i2s.h"

static LGFX lcd;
static TouchDriver touch;
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

    touch.begin();
    ui.begin(&lcd);
    ui.render();

    digitalWrite(PIN_LCD_BL, HIGH);
    Serial.printf("LCD init ok, %dx%d\n", lcd.width(), lcd.height());
}

void loop() {
    static bool was_pressed = false;

    touch.read();
    bool pressed = touch.touched();
    if (pressed) {
        ui.onTouch(touch.x(), touch.y(), true);
        was_pressed = true;
    } else if (was_pressed) {
        was_pressed = false;
    }

    ui.tick();
    delay(10);
}
