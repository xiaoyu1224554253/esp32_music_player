idf_component_register(
    SRCS
        "main.c"
        "ui.c"
        "ui_fonts.c"
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
