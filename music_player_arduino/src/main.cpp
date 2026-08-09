#include <Arduino.h>
#include <LovyanGFX.hpp>
#include <Preferences.h>
#include "config.h"
#include "lgfx_config.h"
#include "ui.h"
#include "player.h"
#include "audio_i2s.h"

static LGFX lcd;
static UI ui;
static Preferences touchNVS;

// ===== 触摸校准参数 (dispX = a*rx + e, dispY = c*ry + f) =====
static float cal_a = -1.164f;   // 默认经验值（未校准时使用）
static float cal_e = 331.7f;
static float cal_c = -1.278f;
static float cal_f = 259.5f;

// ===== 校准流程状态 =====
enum { CAL_IDLE, CAL_RUNNING, CAL_DONE };
static int   cal_state = CAL_IDLE;
static int   cal_step  = 0;
static uint16_t cal_rx[5], cal_ry[5];   // 各点原始 raw
// 目标显示坐标（用户应按此位置点击）
static const uint16_t cal_tx[5] = { 30, 290, 30, 290, 160 };
static const uint16_t cal_ty[5] = { 30, 30, 210, 210, 120 };

static void loadCalibration() {
    touchNVS.begin("touchcal", true);
    if (touchNVS.isKey("a")) {
        cal_a = touchNVS.getFloat("a", cal_a);
        cal_e = touchNVS.getFloat("e", cal_e);
        cal_c = touchNVS.getFloat("c", cal_c);
        cal_f = touchNVS.getFloat("f", cal_f);
        Serial.println("calibration loaded from NVS");
    } else {
        Serial.println("no calibration in NVS, using defaults");
    }
    touchNVS.end();
}

static void drawCalPoint(int idx) {
    lcd.fillScreen(C_BG);
    lcd.setTextColor(C_PRIMARY);
    lcd.setTextSize(1);
    lcd.setFont(&fonts::Font0);  // 简单英文数字
    lcd.setCursor(8, 8);
    lcd.printf("Calibration %d/5", idx + 1);
    lcd.setCursor(8, 24);
    lcd.printf("Touch the cross:");
    int cx = cal_tx[idx], cy = cal_ty[idx];
    // 画十字
    lcd.drawLine(cx - 14, cy, cx + 14, cy, C_PRIMARY);
    lcd.drawLine(cx, cy - 14, cx, cy + 14, C_PRIMARY);
    lcd.drawCircle(cx, cy, 14, C_PRIMARY);
    lcd.setCursor(cx - 18, cy + 20);
    lcd.printf("(%d,%d)", cx, cy);
}

// 一元线性最小二乘: y = a*x + e
static void leastSquares(const uint16_t* x, const uint16_t* y, int n, float* pa, float* pe) {
    double sx = 0, sy = 0, sxx = 0, sxy = 0;
    for (int i = 0; i < n; i++) {
        sx += x[i]; sy += y[i];
        sxx += (double)x[i] * x[i];
        sxy += (double)x[i] * y[i];
    }
    double denom = n * sxx - sx * sx;
    if (fabs(denom) < 1e-6) { *pa = 1; *pe = 0; return; }
    *pa = (n * sxy - sx * sy) / denom;
    *pe = (sy - (*pa) * sx) / n;
}

static void finishCalibration() {
    float a, e, c, f;
    leastSquares(cal_rx, cal_tx, 5, &a, &e);   // rawX -> dispX
    leastSquares(cal_ry, cal_ty, 5, &c, &f);   // rawY -> dispY
    cal_a = a; cal_e = e; cal_c = c; cal_f = f;
    Serial.printf("CAL a=%.4f e=%.2f c=%.4f f=%.2f\n", a, e, c, f);
    touchNVS.begin("touchcal", false);
    touchNVS.putFloat("a", a);
    touchNVS.putFloat("e", e);
    touchNVS.putFloat("c", c);
    touchNVS.putFloat("f", f);
    touchNVS.end();
    Serial.println("calibration saved. reboot to apply, or it applies now.");
}

// 应用校准：raw -> 显示坐标
static void applyCal(uint16_t rx, uint16_t ry, uint16_t* dx, uint16_t* dy) {
    int vx = (int)(cal_a * rx + cal_e);
    int vy = (int)(cal_c * ry + cal_f);
    if (vx < 0) vx = 0; if (vx > 319) vx = 319;
    if (vy < 0) vy = 0; if (vy > 239) vy = 239;
    *dx = (uint16_t)vx;
    *dy = (uint16_t)vy;
}

void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println("ESP32-S3 Music Player (Arduino) starting...");
    Serial.println("Send 'c' over serial to enter touch calibration.");

    pinMode(PIN_LCD_BL, OUTPUT);
    digitalWrite(PIN_LCD_BL, LOW);

    lcd.init();
    lcd.setBrightness(128);
    lcd.setRotation(LCD_ROTATION);
    lcd.fillScreen(C_BG);

    loadCalibration();

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

    // 串口命令：'c' 进入校准
    if (Serial.available()) {
        int ch = Serial.read();
        if (ch == 'c' || ch == 'C') {
            cal_state = CAL_RUNNING;
            cal_step = 0;
            was_pressed = false;
            drawCalPoint(0);
            Serial.println("calibration started, touch the 5 crosses");
        }
    }

    uint16_t rx = 0, ry = 0;
    if (lcd.getTouch(&rx, &ry)) {
        if (cal_state == CAL_RUNNING) {
            if (!was_pressed) {  // 按下边沿记录一次
                cal_rx[cal_step] = rx;
                cal_ry[cal_step] = ry;
                Serial.printf("cal point %d raw=%d,%d\n", cal_step, rx, ry);
                cal_step++;
                if (cal_step >= 5) {
                    finishCalibration();
                    cal_state = CAL_IDLE;
                    ui.render();
                } else {
                    drawCalPoint(cal_step);
                }
            }
            was_pressed = true;
        } else {
            uint16_t dx, dy;
            applyCal(rx, ry, &dx, &dy);
            ui.onTouch(dx, dy, true);
            was_pressed = true;
        }
    } else if (was_pressed) {
        was_pressed = false;
    }

    ui.tick();
    delay(10);
}
