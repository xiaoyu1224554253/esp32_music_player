# ESP-IDF 主组件: 占位骨架
# 后续在此实现音乐播放器主逻辑 (LVGL UI + 音频解码 + 触摸)
idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
)
