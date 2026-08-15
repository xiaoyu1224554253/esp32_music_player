#include <Arduino.h>
#include <SPI.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "board/board_pins.h"
#include "board/board_spi.h"
#include "hal/board_hw_control.h"

SPIClass SPI_UI;            // UI TFT 专用 SPI 实例 (LovyanGFX 内部也用, 这里仅作安全态管理)
static SemaphoreHandle_t s_ui_spi_mtx = nullptr;

/* 初始化板级总线 - ES3C28P 版
 * 无 MCP23017 / 无 RC522 NFC。
 * 背光(PIN_LCD_BL)与功放使能(PIN_I2S_EN)均为直连 GPIO。
 * 触摸 FT6336 的 I2C 由 LovyanGFX Touch_FT5x06 自行管理(bus_shared)。
 */
void board_spi_init(void)
{
    static bool inited = false;
    if (inited) return;
    inited = true;

    Serial.println("[启动] 初始化板级总线 (ES3C28P)...");

    // ---- 背光: 高电平点亮 ----
    pinMode(board::PIN_LCD_BL, OUTPUT);
    digitalWrite(board::PIN_LCD_BL, HIGH);

    // ---- 功放使能: 默认静音(低电平) ----
    pinMode(board::PIN_I2S_EN, OUTPUT);
    digitalWrite(board::PIN_I2S_EN, LOW);

    // ---- 触摸 I2C 总线 (FT6336) ----
    Wire.begin(board::PIN_TP_SDA, board::PIN_TP_SCL);
    Wire.setClock(400000);

    if (!s_ui_spi_mtx) {
        s_ui_spi_mtx = xSemaphoreCreateRecursiveMutex();
    }

    // ---- 屏幕 CS 安全态（LovyanGFX 内部会自行接管 SPI 总线）----
    pinMode(PIN_TFT_CS, OUTPUT);
    digitalWrite(PIN_TFT_CS, HIGH);

    Serial.printf("[总线] TFT: CS=%d DC=%d RST=%d\n",
                  PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_RST);
    Serial.printf("[总线] 背光 GPIO=%d 功放使能 GPIO=%d 触摸 I2C SDA=%d SCL=%d\n",
                  board::PIN_LCD_BL, board::PIN_I2S_EN,
                  board::PIN_TP_SDA, board::PIN_TP_SCL);
}

void board_spi_ui_lock(void)
{
    if (s_ui_spi_mtx) {
        xSemaphoreTakeRecursive(s_ui_spi_mtx, portMAX_DELAY);
    }
}

void board_spi_ui_unlock(void)
{
    if (s_ui_spi_mtx) {
        xSemaphoreGiveRecursive(s_ui_spi_mtx);
    }
}
