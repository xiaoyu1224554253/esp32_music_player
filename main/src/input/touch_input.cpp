#include "input/touch_input.h"

#include <LovyanGFX.hpp>

#include "board/board_pins.h"
#include "keys/keys.h"
#include "ui/ui.h"             // 复用全局 tft (LGFX 单例, 含 FT6336 触摸)
#include "ui/ui_internal.h"

namespace {

enum class TouchPhase {
    Idle,
    Down,
};

TouchPhase s_phase = TouchPhase::Idle;
int s_down_x = 0;
int s_down_y = 0;
uint32_t s_down_ms = 0;
bool s_moved = false;

// 去抖：相邻两次判定最小间隔
uint32_t s_last_inject_ms = 0;

bool get_touch_point(int& x, int& y)
{
    // LovyanGFX FT5x06：返回是否检测到触摸点
    lgfx::touch_point_t tp[1];
    const int n = tft.getTouch(tp, 1);
    if (n > 0) {
        x = tp[0].x;
        y = tp[0].y;
        return true;
    }
    return false;
}

}  // namespace

void touch_input_init()
{
    s_phase = TouchPhase::Idle;
}

void touch_input_update()
{
    int x = 0, y = 0;
    const bool touched = get_touch_point(x, y);
    const uint32_t now = millis();

    if (touched) {
        if (s_phase == TouchPhase::Idle) {
            s_phase = TouchPhase::Down;
            s_down_x = x;
            s_down_y = y;
            s_down_ms = now;
            s_moved = false;
        } else {
            // 移动中：累计位移判断是否滑动
            const int dx = x - s_down_x;
            const int dy = y - s_down_y;
            if (abs(dx) > touch_cfg::TAP_MAX_PX || abs(dy) > touch_cfg::TAP_MAX_PX) {
                s_moved = true;
            }
        }
    } else {
        if (s_phase == TouchPhase::Down) {
            // 松手：结算手势
            const int dx = 0;  // 已在 down 时记录起点，这里只用位移方向
            const int dy = 0;
            (void)dx; (void)dy;
            const int move_x = x - s_down_x;  // 注意：touched=false 时 x=上一次？用累积
            (void)move_x; (void)dy;

            const uint32_t dur = now - s_down_ms;

            // 用 s_moved 判断是否为点击；滑动方向在下面用累计位移
            if (!s_moved && dur <= touch_cfg::TAP_MAX_MS) {
                // 点击 -> 确认 / 播放暂停
                if (now - s_last_inject_ms > 120) {
                    keys_inject_event(KEY_PLAY_SHORT);
                    s_last_inject_ms = now;
                }
            }
            // 滑动判定在移动过程中实时触发（见下方实时分支），松手无需额外处理

            s_phase = TouchPhase::Idle;
        }
    }

    // 实时滑动判定：手指移动超过阈值即触发一次方向事件
    if (s_phase == TouchPhase::Down && s_moved) {
        const int dx = x - s_down_x;
        const int dy = y - s_down_y;
        if (abs(dx) >= touch_cfg::SLIDE_MIN_PX || abs(dy) >= touch_cfg::SLIDE_MIN_PX) {
            if (now - s_last_inject_ms > 250) {
                if (abs(dy) >= abs(dx)) {
                    // 上下滑动 -> 列表上/下移动 (KEY_PREV/NEXT_SHORT 在列表模式即上下)
                    keys_inject_event(dy < 0 ? KEY_NEXT_SHORT : KEY_PREV_SHORT);
                } else {
                    // 左右滑动 -> 上一曲/下一曲
                    keys_inject_event(dx < 0 ? KEY_NEXT_SHORT : KEY_PREV_SHORT);
                }
                s_last_inject_ms = now;
                // 重置起点，支持连续滑动多触发
                s_down_x = x;
                s_down_y = y;
                s_moved = false;
            }
        }
    }
}
