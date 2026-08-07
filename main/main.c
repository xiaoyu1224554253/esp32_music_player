#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ESP32-S3 音乐播放器固件 - 占位入口
// 板子: LCDWiki 2.8inch ESP32-S3 Display (ES3C28P)
//   屏幕: ILI9341V 240x320 SPI
//   触摸: FT6336 I2C
void app_main(void)
{
    printf("esp32_music_player: skeleton boot OK\n");
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
