#include "hal/board_hw_control.h"

#include <Arduino.h>
#include <driver/gpio.h>

#include "board/board_pins.h"
#include "utils/log.h"

namespace {

// 高阻分压输入，采样次数稍微多一点，降低 ESP32 ADC 抖动。
static constexpr uint8_t BATTERY_ADC_SAMPLE_COUNT = 16;
static constexpr uint16_t BATTERY_ADC_SETTLE_US = 300;

// EMA 平滑比例：新值占 1/4，旧值占 3/4。
static constexpr uint32_t BATTERY_EMA_NEW_NUM = 1;
static constexpr uint32_t BATTERY_EMA_DEN = 4;

// ADC 读数校准。
static constexpr uint32_t BATTERY_ADC_CAL_NUM = 1532;
static constexpr uint32_t BATTERY_ADC_CAL_DEN = 1580;

// 电池分压倍率校准。
static constexpr uint32_t BATTERY_DIVIDER_CAL_NUM = 3810;
static constexpr uint32_t BATTERY_DIVIDER_CAL_DEN = 1480;

// 电池平滑滤波状态
static bool s_battery_filter_ready = false;
static uint32_t s_battery_filtered_raw = 0;
static uint32_t s_battery_filtered_mv_adc = 0;
static uint32_t s_battery_filtered_mv_battery = 0;

static constexpr uint32_t BATTERY_UI_BOOT_SAMPLE_INTERVAL_MS = 3000;
static constexpr uint32_t BATTERY_UI_STABLE_SAMPLE_INTERVAL_MS = 60UL * 1000UL;
static constexpr uint32_t CHARGER_UI_SAMPLE_INTERVAL_MS = 1000;
static constexpr uint8_t BATTERY_UI_BOOT_SAMPLE_COUNT = 5;

static BatteryUiStatus s_battery_ui_status{};
static uint32_t s_battery_ui_last_sample_ms = 0;
static uint32_t s_charger_ui_last_sample_ms = 0;
static uint8_t s_battery_ui_sample_count = 0;

// ES3C28P 功放使能 (PIN_I2S_EN, 高=开启)。
// 静音语义：enabled=true -> 功放关闭(低)；enabled=false -> 功放开启(高)。
static bool s_amp_mute_enabled = true;
static bool s_backlight_enabled = true;

static uint8_t battery_percent_from_mv(uint32_t mv)
{
    if (mv >= 4200) return 100;
    if (mv >= 4000) return 80 + static_cast<uint8_t>((mv - 4000) * 20 / 200);
    if (mv >= 3800) return 55 + static_cast<uint8_t>((mv - 3800) * 25 / 200);
    if (mv >= 3700) return 40 + static_cast<uint8_t>((mv - 3700) * 15 / 100);
    if (mv >= 3600) return 25 + static_cast<uint8_t>((mv - 3600) * 15 / 100);
    if (mv >= 3500) return 12 + static_cast<uint8_t>((mv - 3500) * 13 / 100);
    if (mv >= 3300) return static_cast<uint8_t>((mv - 3300) * 12 / 200);
    return 0;
}

static void configure_battery_adc_input()
{
    pinMode(PIN_BAT_ADC, INPUT);

#if defined(ARDUINO_ARCH_ESP32)
    gpio_num_t gpio = static_cast<gpio_num_t>(PIN_BAT_ADC);
    gpio_set_direction(gpio, GPIO_MODE_INPUT);
    gpio_pullup_dis(gpio);
    gpio_pulldown_dis(gpio);
    analogSetPinAttenuation(PIN_BAT_ADC, ADC_11db);
#endif
}

static uint32_t apply_ema_filter(uint32_t old_value, uint32_t new_value)
{
    return ((old_value * (BATTERY_EMA_DEN - BATTERY_EMA_NEW_NUM)) +
            (new_value * BATTERY_EMA_NEW_NUM)) /
           BATTERY_EMA_DEN;
}

static BatterySample apply_battery_filter(const BatterySample& sample)
{
    if (!s_battery_filter_ready) {
        s_battery_filtered_raw = sample.raw;
        s_battery_filtered_mv_adc = sample.mv_adc;
        s_battery_filtered_mv_battery = sample.mv_battery;
        s_battery_filter_ready = true;
    } else {
        s_battery_filtered_raw = apply_ema_filter(s_battery_filtered_raw, sample.raw);
        s_battery_filtered_mv_adc = apply_ema_filter(s_battery_filtered_mv_adc, sample.mv_adc);
        s_battery_filtered_mv_battery = apply_ema_filter(s_battery_filtered_mv_battery, sample.mv_battery);
    }

    BatterySample out = sample;
    out.raw = static_cast<uint16_t>(s_battery_filtered_raw);
    out.mv_adc = s_battery_filtered_mv_adc;
    out.mv_battery = s_battery_filtered_mv_battery;
    return out;
}

}  // namespace

bool board_hw_control_begin()
{
    configure_battery_adc_input();

    // 背光点亮
    pinMode(board::PIN_LCD_BL, OUTPUT);
    digitalWrite(board::PIN_LCD_BL, HIGH);
    s_backlight_enabled = true;

    // 功放使能：默认静音(关闭)
    pinMode(board::PIN_I2S_EN, OUTPUT);
    digitalWrite(board::PIN_I2S_EN, LOW);
    s_amp_mute_enabled = true;

    LOGI("[硬件控制] 初始化成功 BAT_ADC=%d 背光=%d 功放使能=%d",
         PIN_BAT_ADC, board::PIN_LCD_BL, board::PIN_I2S_EN);

    return true;
}

BatterySample board_hw_read_battery()
{
    BatterySample s{};

    configure_battery_adc_input();

    (void)analogRead(PIN_BAT_ADC);
    delayMicroseconds(BATTERY_ADC_SETTLE_US);
    (void)analogRead(PIN_BAT_ADC);
    delayMicroseconds(BATTERY_ADC_SETTLE_US);

    uint32_t raw_sum = 0;
    uint32_t mv_sum = 0;
    uint32_t raw_min = 0xFFFFFFFFu;
    uint32_t raw_max = 0;
    uint32_t mv_min = 0xFFFFFFFFu;
    uint32_t mv_max = 0;

    for (uint8_t i = 0; i < BATTERY_ADC_SAMPLE_COUNT; ++i) {
        const uint32_t raw = static_cast<uint32_t>(analogRead(PIN_BAT_ADC));
#if defined(ARDUINO_ARCH_ESP32)
        const uint32_t mv = static_cast<uint32_t>(analogReadMilliVolts(PIN_BAT_ADC));
#else
        const uint32_t mv = 0;
#endif
        raw_sum += raw;
        mv_sum += mv;
        if (raw < raw_min) raw_min = raw;
        if (raw > raw_max) raw_max = raw;
        if (mv < mv_min) mv_min = mv;
        if (mv > mv_max) mv_max = mv;
        delayMicroseconds(BATTERY_ADC_SETTLE_US);
    }

    static constexpr uint8_t EFFECTIVE_SAMPLE_COUNT = BATTERY_ADC_SAMPLE_COUNT - 2;
    const uint32_t raw_avg = (raw_sum - raw_min - raw_max) / EFFECTIVE_SAMPLE_COUNT;
    const uint32_t mv_adc_raw = (mv_sum - mv_min - mv_max) / EFFECTIVE_SAMPLE_COUNT;

    s.raw = static_cast<uint16_t>(raw_avg);

#if defined(ARDUINO_ARCH_ESP32)
    s.mv_adc = static_cast<uint32_t>(
        (static_cast<uint64_t>(mv_adc_raw) * BATTERY_ADC_CAL_NUM) / BATTERY_ADC_CAL_DEN);
#else
    s.mv_adc = 0;
#endif

    s.mv_battery = static_cast<uint32_t>(
        (static_cast<uint64_t>(s.mv_adc) * BATTERY_DIVIDER_CAL_NUM) / BATTERY_DIVIDER_CAL_DEN);

    return apply_battery_filter(s);
}

ChargerStatus board_hw_read_charger_status()
{
    // ES3C28P 无充电管理 IC 连接，返回默认状态。
    ChargerStatus s{};
    s.valid = true;
    s.external_power_good = false;
    s.charging = false;
    return s;
}

static void board_hw_update_battery_status_cache()
{
    const BatterySample bat = board_hw_read_battery();
    const ChargerStatus chg = board_hw_read_charger_status();

    BatteryUiStatus out = s_battery_ui_status;
    out.valid = bat.mv_battery > 0;
    out.mv_battery = bat.mv_battery;
    out.mv_adc = bat.mv_adc;
    out.raw = bat.raw;
    out.percent = battery_percent_from_mv(bat.mv_battery);
    out.external_power_good = chg.external_power_good;
    out.charging = chg.charging;

    out.updated_ms = millis();
    s_battery_ui_status = out;
    s_battery_ui_last_sample_ms = out.updated_ms;

    if (s_battery_ui_sample_count < 255) {
        ++s_battery_ui_sample_count;
    }
}

void board_hw_battery_status_tick()
{
    const uint32_t now = millis();
    const bool boot_sampling = s_battery_ui_sample_count < BATTERY_UI_BOOT_SAMPLE_COUNT;
    const uint32_t interval_ms = boot_sampling
        ? BATTERY_UI_BOOT_SAMPLE_INTERVAL_MS
        : BATTERY_UI_STABLE_SAMPLE_INTERVAL_MS;

    if (s_battery_ui_last_sample_ms != 0 &&
        now - s_battery_ui_last_sample_ms < interval_ms) {
        return;
    }
    board_hw_update_battery_status_cache();
}

BatteryUiStatus board_hw_get_battery_status_cached()
{
    return s_battery_ui_status;
}

bool board_hw_set_bt_power(bool /*enabled*/) { return true; }   // 无蓝牙硬件
bool board_hw_get_bt_power() { return false; }
bool board_hw_set_bt_wakeup(bool /*enabled*/) { return true; }
bool board_hw_get_bt_wakeup() { return false; }
bool board_hw_set_bt_switch(bool /*level*/) { return true; }
bool board_hw_get_bt_switch() { return false; }
bool board_hw_pulse_bt_switch(uint32_t /*pulse_ms*/) { return true; }

bool board_hw_set_backlight(bool enabled)
{
    pinMode(board::PIN_LCD_BL, OUTPUT);
    digitalWrite(board::PIN_LCD_BL, enabled ? HIGH : LOW);
    s_backlight_enabled = enabled;
    LOGI("[硬件控制] 背光 %s", enabled ? "开启" : "关闭");
    return true;
}

bool board_hw_get_backlight()
{
    return s_backlight_enabled;
}

bool board_hw_set_amp_mute(bool enabled)
{
    // enabled=true -> 功放静音(关闭), PIN_I2S_EN 拉低
    s_amp_mute_enabled = enabled;
    pinMode(board::PIN_I2S_EN, OUTPUT);
    digitalWrite(board::PIN_I2S_EN, enabled ? LOW : HIGH);
    LOGD("[硬件控制] 功放静音 %s", enabled ? "开启" : "关闭");
    return true;
}

bool board_hw_get_amp_mute()
{
    return s_amp_mute_enabled;
}

bool board_hw_set_amp_shutdown(bool enabled)
{
    // 合并到功放使能脚：enabled=true -> 关断(低)
    pinMode(board::PIN_I2S_EN, OUTPUT);
    digitalWrite(board::PIN_I2S_EN, enabled ? LOW : HIGH);
    return true;
}

bool board_hw_get_amp_shutdown()
{
    return s_amp_mute_enabled;
}

void board_hw_debug_dump()
{
    const BatterySample bat = board_hw_read_battery();
    LOGD("[硬件控制] 状态 bat_raw=%u adc=%lumV 电池=%lumV 静音=%d",
         bat.raw,
         (unsigned long)bat.mv_adc,
         (unsigned long)bat.mv_battery,
         s_amp_mute_enabled ? 1 : 0);
}

void board_hw_solenoid_tick()
{
    // 目标板无电磁铁，空操作。
}

void board_hw_solenoid_stop()
{
    // 目标板无电磁铁，空操作。
}

void board_hw_solenoid_flip(uint32_t /*pulse_ms*/)
{
    // 目标板无电磁铁，空操作。
}

bool board_hw_solenoid_is_busy()
{
    return false;
}

void board_hw_power_off()
{
    LOGI("[硬件控制] ES3C28P 无电源自锁，关机仅熄屏+静音");
    board_hw_set_backlight(false);
    board_hw_set_amp_mute(true);
}
