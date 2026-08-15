#pragma once

#include "board/board_pins.h"

// ES3C28P：无 MCP23017 / EC06 编码器 / 实体导航键。
// 所有导航交互由 FT6336 触摸层通过 keys_inject_event() 注入 key_event_t。
// 仅保留 BOOT 键(GPIO0) 作为物理播放/暂停，其余虚拟引脚统一设为 -1（禁用）。

#define PIN_KEY_DISABLED        (-1)

#define PIN_KEY_MODE   PIN_KEY_DISABLED   // 触摸注入 KEY_MODE_SHORT
#define PIN_KEY_PLAY   PIN_POWER_PLAY     // BOOT 键 GPIO0 = 播放/暂停
#define PIN_KEY_PREV   PIN_KEY_DISABLED   // 触摸注入 KEY_PREV_SHORT
#define PIN_KEY_NEXT   PIN_KEY_DISABLED   // 触摸注入 KEY_NEXT_SHORT
#define PIN_KEY_MCP_EC06_E  PIN_KEY_DISABLED   // 触摸注入 KEY_PLAY_SHORT
#define PIN_KEY_VOLDN  PIN_KEY_DISABLED
#define PIN_KEY_VOLUP  PIN_KEY_DISABLED

// 霍尔输入（本板无）
#define PIN_KEY_HALL_OUT PIN_HALL_OUT