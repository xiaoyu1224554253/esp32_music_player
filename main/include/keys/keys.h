#pragma once
#include "player_list_select.h"  // key_event_t

void keys_init();
void keys_update();
void keys_sync_to_hw_state();  // 同步按键状态到硬件，用于状态切换时避免误判

// FT6336 触摸层注入：将触摸手势映射为与物理按键相同的 key_event_t。
void keys_inject_event(key_event_t evt);
