#include "audio_i2s.h"

#include <driver/i2s.h>

static i2s_port_t g_port = I2S_NUM_0;
static bool g_inited = false;

bool audio_i2s_init(int bck, int ws, int dout, int mclk, int sample_rate) {
    if (g_inited) return true;

    i2s_config_t cfg = {};
    cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
    cfg.sample_rate = sample_rate;
    cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
    cfg.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
    cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
    cfg.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
    cfg.dma_buf_count = 8;
    cfg.dma_buf_len = 256;
    cfg.use_apll = true;
    cfg.tx_desc_auto_clear = true;

    i2s_pin_config_t pins = {};
#if SOC_I2S_SUPPORTS_MCLK
    pins.mck_io_num = mclk;
#else
    pins.mck_io_num = -1;
#endif
    pins.bck_io_num = bck;
    pins.ws_io_num = ws;
    pins.data_out_num = dout;
    pins.data_in_num = -1;

    esp_err_t err = i2s_driver_install(g_port, &cfg, 0, nullptr);
    if (err != ESP_OK) return false;
    err = i2s_set_pin(g_port, &pins);
    if (err != ESP_OK) { i2s_driver_uninstall(g_port); return false; }

    g_inited = true;
    return true;
}

void audio_i2s_deinit() {
    if (!g_inited) return;
    i2s_driver_uninstall(g_port);
    g_inited = false;
}

size_t audio_i2s_write(const uint8_t* data, size_t len) {
    if (!g_inited) return 0;
    size_t written = 0;
    esp_err_t err = i2s_write(g_port, (void*)data, len, &written, pdMS_TO_TICKS(100));
    if (err != ESP_OK) return 0;
    return written;
}

bool audio_i2s_set_sample_rate(int sample_rate) {
    if (!g_inited) return false;
    return i2s_set_sample_rates(g_port, sample_rate) == ESP_OK;
}

void audio_i2s_mute(bool mute) {
    // 停止 DMA 写入即可静音；这里通过清空缓冲实现
    if (g_inited) {
#if defined(I2S_ZERO_DMA_BUFFER_SUPPORTED) || defined(ESP_IDF_VERSION)
        // arduino-esp32 >= 2.0.5 提供 i2s_zero_dma_buffer
        i2s_zero_dma_buffer(g_port);
#else
        (void)mute;
#endif
    }
}
