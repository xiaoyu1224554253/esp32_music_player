#pragma once
#include <Arduino.h>

// 简单的 ESP32 I2S 音频输出（使用内置 i2s 外设 / 兼容 arduino-esp32 >=2.x）
bool audio_i2s_init(int bck, int ws, int dout, int mclk, int sample_rate);
void audio_i2s_deinit();
size_t audio_i2s_write(const uint8_t* data, size_t len);
bool audio_i2s_set_sample_rate(int sample_rate);
void audio_i2s_mute(bool mute);
