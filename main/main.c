#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl/lvgl.h"
#include "esp_timer.h"
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

    static lv_disp_draw_buf_t draw_buf;
    static lv_color_t buf1[LCD_H_RES * 20];
    static lv_color_t buf2[LCD_H_RES * 20];
    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, LCD_H_RES * 20);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = LCD_H_RES;
    disp_drv.ver_res = LCD_V_RES;
    disp_drv.flush_cb = disp_driver_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = tp_driver_read;
    lv_indev_drv_register(&indev_drv);

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
