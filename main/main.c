#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "esp_timer.h"
#include "board.h"
#include "disp_driver.h"
#include "tp_driver.h"
#include "ui.h"

#define LV_TICK_PERIOD_MS 5

static void lv_tick_task(void *arg)
{
    (void)arg;
    lv_tick_inc(LV_TICK_PERIOD_MS);
}

static void lv_task_handler_task(void *arg)
{
    (void)arg;
    while (1) {
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void app_main(void)
{
    printf("esp32_music_player: boot\n");

    /* 显示 + 触摸 */
    disp_driver_init();
    tp_driver_init();

    /* LVGL 初始化 */
    lv_init();

    static lv_color_t buf1[LCD_H_RES * 20];
    static lv_color_t buf2[LCD_H_RES * 20];

    lv_display_t *disp = lv_display_create(LCD_H_RES, LCD_V_RES);
    lv_display_set_flush_cb(disp, disp_driver_flush);
    lv_display_set_buffers(disp, buf1, buf2, sizeof(buf1),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);

    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, tp_driver_read);

    /* UI */
    ui_init();

    /* 定时器 */
    const esp_timer_create_args_t tick_args = { .callback = lv_tick_task, .name = "lv_tick" };
    esp_timer_handle_t tick_timer;
    esp_timer_create(&tick_args, &tick_timer);
    esp_timer_start_periodic(tick_timer, LV_TICK_PERIOD_MS * 1000);

    /* LVGL 主循环 */
    xTaskCreate(lv_task_handler_task, "lv_task", 8192, NULL, 5, NULL);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
