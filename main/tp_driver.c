/* FT6336G 电容触摸驱动 (I2C) + LVGL 输入回调 */
#include "tp_driver.h"
#include "board.h"
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "ft6336";

/* FT6336 寄存器 */
#define REG_TD_STATUS   0x02
#define REG_P1_XH       0x03
#define REG_P1_YH       0x05
#define REG_GEST_ID     0x01

/*
 * 触摸坐标与屏幕逻辑坐标的对应关系
 * 屏幕已做 swap_xy=true (裸屏 240x320 -> 逻辑 320x240)。
 * FT6336 控制器输出的 x 沿玻璃长边(0~320)、y 沿短边(0~240)，
 * 与逻辑坐标轴向一致，故默认 1:1 映射，无需 swap。
 * 若现场发现触摸左右/上下翻转，把对应宏改为 1 即可。
 */
#define TOUCH_SWAP_XY   0
#define TOUCH_MIRROR_X  0
#define TOUCH_MIRROR_Y  0

static void tp_map(uint16_t raw_x, uint16_t raw_y, lv_coord_t *out_x, lv_coord_t *out_y)
{
    uint16_t x = raw_x, y = raw_y;
#if TOUCH_SWAP_XY
    uint16_t t = x; x = y; y = t;
#endif
#if TOUCH_MIRROR_X
    x = LCD_H_RES - 1 - x;
#endif
#if TOUCH_MIRROR_Y
    y = LCD_V_RES - 1 - y;
#endif
    *out_x = x;
    *out_y = y;
}

static esp_err_t tp_write_reg(uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = { reg, val };
    return i2c_master_write_to_device(TP_I2C_HOST, TP_I2C_ADDR, buf, 2,
                                       pdMS_TO_TICKS(100));
}

static esp_err_t tp_read_reg(uint8_t reg, uint8_t *buf, size_t len)
{
    return i2c_master_write_read_device(TP_I2C_HOST, TP_I2C_ADDR, &reg, 1,
                                         buf, len, pdMS_TO_TICKS(100));
}

esp_err_t tp_driver_init(void)
{
    /* 复位引脚 */
    gpio_config_t rst = {
        .pin_bit_mask = 1ULL << PIN_TP_RST,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = 0, .pull_down_en = 0, .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&rst);
    gpio_set_level(PIN_TP_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level(PIN_TP_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(60));

    /* I2C 主机 */
    i2c_config_t cfg = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = PIN_TP_SDA,
        .scl_io_num = PIN_TP_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = TP_I2C_CLOCK_HZ,
    };
    ESP_RETURN_ON_ERROR(i2c_param_config(TP_I2C_HOST, &cfg), TAG, "i2c param");
    ESP_RETURN_ON_ERROR(i2c_driver_install(TP_I2C_HOST, I2C_MODE_MASTER, 0, 0, 0), TAG, "i2c install");

    uint8_t id = 0;
    if (tp_read_reg(0xA3, &id, 1) == ESP_OK) {
        ESP_LOGI(TAG, "FT6336 chip id: 0x%02X", id);
    }
    /* 进入正常模式 */
    tp_write_reg(0x00, 0x00);
    return ESP_OK;
}

/* LVGL 9 read 回调: 返回当前触摸点 */
void tp_driver_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    uint8_t status = 0;
    if (tp_read_reg(REG_TD_STATUS, &status, 1) != ESP_OK) {
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }
    uint8_t n = status & 0x0F;
    if (n == 0) {
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }
    uint8_t xh = 0, xl = 0, yh = 0, yl = 0;
    tp_read_reg(REG_P1_XH, &xh, 1);
    tp_read_reg(REG_P1_XH + 1, &xl, 1);
    tp_read_reg(REG_P1_YH, &yh, 1);
    tp_read_reg(REG_P1_YH + 1, &yl, 1);
    uint16_t x = ((xh & 0x0F) << 8) | xl;
    uint16_t y = ((yh & 0x0F) << 8) | yl;
    tp_map(x, y, &data->point.x, &data->point.y);
    data->state = LV_INDEV_STATE_PRESSED;
}
