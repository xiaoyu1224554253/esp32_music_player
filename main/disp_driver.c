/* ILI9341V SPI 显示驱动 (基于 ESP-IDF esp_lcd, v6.0 API) */
#include "disp_driver.h"
#include "board.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_dev.h"
#include "esp_lcd_ili9341.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_check.h"

static esp_lcd_panel_handle_t panel_handle = NULL;
static esp_lcd_panel_io_handle_t io_handle = NULL;

esp_err_t disp_driver_init(void)
{
    /* 背光 */
    gpio_config_t bl = {
        .pin_bit_mask = 1ULL << PIN_LCD_BL,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = 0,
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&bl);
    gpio_set_level(PIN_LCD_BL, PIN_LCD_BL_ON);

    /* SPI 总线 */
    spi_bus_config_t buscfg = {
        .sclk_io_num = PIN_LCD_SCK,
        .mosi_io_num = PIN_LCD_MOSI,
        .miso_io_num = PIN_LCD_MISO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_H_RES * LCD_V_RES * 2 + 8,
    };
    ESP_RETURN_ON_ERROR(spi_bus_initialize(LCD_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO), "disp", "spi bus");

    /* LCD IO (SPI) */
    esp_lcd_panel_io_spi_config_t io_cfg = {
        .cs_gpio_num = PIN_LCD_CS,
        .dc_gpio_num = PIN_LCD_DC,
        .spi_mode = 0,
        .pclk_hz = LCD_SPI_CLOCK_HZ,
        .trans_queue_depth = 10,
        .on_color_trans_done = NULL,
        .user_ctx = NULL,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_SPI_HOST, &io_cfg, &io_handle), "disp", "io");

    /* ILI9341 面板 (ESP-IDF v6.0 API) */
    esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = PIN_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,  /* 参考 freenove 2.8 配置 */
        .bits_per_pixel = 16,   /* LVGL 默认 RGB565 16bpp, 必须一致 */
        .flags.reset_active_high = 0,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_ili9341(io_handle, &panel_cfg, &panel_handle), "disp", "panel");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_reset(panel_handle), "disp", "reset");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_init(panel_handle), "disp", "init");
    /* 横屏 320x240 */
    ESP_RETURN_ON_ERROR(esp_lcd_panel_swap_xy(panel_handle, true), "disp", "swap");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_mirror(panel_handle, false, false), "disp", "mirror");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_set_gap(panel_handle, 0, 0), "disp", "gap");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_invert_color(panel_handle, true), "disp", "inv");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_disp_on_off(panel_handle, true), "disp", "on");

    return ESP_OK;
}

/* LVGL 9 flush 回调 */
void disp_driver_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *color_p)
{
    int x1 = area->x1, y1 = area->y1, x2 = area->x2, y2 = area->y2;
    esp_lcd_panel_draw_bitmap(panel_handle, x1, y1, x2 + 1, y2 + 1, color_p);
    lv_display_flush_ready(disp);
}

esp_lcd_panel_handle_t disp_driver_get_panel(void) { return panel_handle; }
