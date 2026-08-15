#pragma once
// ES3C28P 兼容空壳：目标板无 MCP23017 IO 扩展器。
// 保留声明仅为兼容旧调用点；所有函数返回安全默认值（无操作）。

#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

static inline bool mcp23017_u3_is_ready() { return false; }
static inline bool mcp23017_u3_begin() { return false; }
static inline uint8_t mcp23017_u3_read_a() { return 0xFF; }
static inline uint8_t mcp23017_u3_read_b() { return 0xFF; }
static inline void mcp23017_u3_pinMode(uint8_t, uint8_t) {}
static inline void mcp23017_u3_digitalWrite(uint8_t, uint8_t) {}
static inline int mcp23017_u3_digitalRead(uint8_t) { return HIGH; }

#ifdef __cplusplus
}
#endif
