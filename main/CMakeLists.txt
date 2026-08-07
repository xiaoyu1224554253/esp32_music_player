idf_component_register(
    SRCS
        "main.c"
        "ui.c"
        "ui_fonts.c"
        "ui_fonts_22.c"
        "disp_driver.c"
        "tp_driver.c"
    INCLUDE_DIRS "."
    REQUIRES
        lvgl
        driver
        esp_lcd
        esp_timer
        freertos
)

# 让 LVGL 使用本项目提供的 lv_conf.h（位于本组件 include 路径）
target_compile_definitions(${COMPONENT_LIB} PRIVATE LV_CONF_INCLUDE_SIMPLE)
