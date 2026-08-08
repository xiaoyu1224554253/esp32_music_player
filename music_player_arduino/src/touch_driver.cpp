#include "touch_driver.h"

#define FT6336_REG_TD_STATUS  0x02
#define FT6336_REG_P1_XH      0x03
#define FT6336_REG_P1_XL      0x04
#define FT6336_REG_P1_YH      0x05
#define FT6336_REG_P1_YL      0x06

bool TouchDriver::begin() {
    Wire.begin(PIN_TP_SDA, PIN_TP_SCL);
    Wire.setClock(100000);

    // 复位
    if (PIN_TP_RST >= 0) {
        pinMode(PIN_TP_RST, OUTPUT);
        digitalWrite(PIN_TP_RST, LOW);
        delay(10);
        digitalWrite(PIN_TP_RST, HIGH);
        delay(50);
    }

    // 读取 chip id 用于判断是否存在
    uint8_t chip = readReg(0xA3);
    Serial.printf("FT6336 chip id: 0x%02X\n", chip);
    return true; // 即使读不到也继续
}

uint8_t TouchDriver::readReg(uint8_t reg) {
    Wire.beginTransmission(TP_I2C_ADDR);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom((int)TP_I2C_ADDR, 1, true);
    if (Wire.available()) return Wire.read();
    return 0;
}

void TouchDriver::read() {
    uint8_t td_status = readReg(FT6336_REG_TD_STATUS);
    int touches = td_status & 0x0F;
    _pressed = (touches > 0);

    if (!_pressed) return;

    uint8_t xh = readReg(FT6336_REG_P1_XH);
    uint8_t xl = readReg(FT6336_REG_P1_XL);
    uint8_t yh = readReg(FT6336_REG_P1_YH);
    uint8_t yl = readReg(FT6336_REG_P1_YL);

    uint16_t raw_x = ((xh & 0x0F) << 8) | xl;
    uint16_t raw_y = ((yh & 0x0F) << 8) | yl;

    // 根据实际测试方向调整 swap/mirror
    const bool SWAP_XY = false;
    const bool MIRROR_X = false;
    const bool MIRROR_Y = false;

    uint16_t tx = raw_x, ty = raw_y;
    if (SWAP_XY) { tx = raw_y; ty = raw_x; }
    if (MIRROR_X) tx = 319 - tx;
    if (MIRROR_Y) ty = 239 - ty;

    // 触摸原始坐标典型为 0~319 (x) / 0~239 (y)，直接 1:1 映射
    _x = constrain(tx, 0, SCREEN_WIDTH - 1);
    _y = constrain(ty, 0, SCREEN_HEIGHT - 1);
}
