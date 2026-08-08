#pragma once

#include <Arduino.h>
#include <Wire.h>
#include "config.h"

struct TouchPoint {
    bool valid;
    uint16_t x;
    uint16_t y;
    bool pressed;
};

class TouchDriver {
public:
    bool begin();
    void read();
    bool touched() const { return _pressed; }
    uint16_t x() const { return _x; }
    uint16_t y() const { return _y; }
private:
    uint16_t _x = 0, _y = 0;
    bool _pressed = false;
    bool writeReg(uint8_t reg, uint8_t val);
    uint8_t readReg(uint8_t reg);
};
