#pragma once

#include <cstdint>

// FT6336 电容触摸输入层 (ES3C28P)
// 把触摸手势映射为与 EC06 编码器/按键相同的 key_event_t，
// 通过 keys_inject_event() 注入，复用原有按键分发逻辑。

void touch_input_init();
void touch_input_update();

// 手势阈值（像素）
namespace touch_cfg {
constexpr int SLIDE_MIN_PX   = 24;    // 判定为滑动的最小位移
constexpr int TAP_MAX_PX     = 14;    // 小于此位移视为点击
constexpr int TAP_MAX_MS     = 300;   // 点击最长时长
}  // namespace touch_cfg
