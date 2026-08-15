#include "storage/storage.h"
#include "board/board_pins.h"
#include "board/board_spi.h"
#include "storage/storage_io.h"
#include "utils/log.h"

#include <Arduino.h>
#include <FS.h>
#include <SdFat.h>
#include <SPI.h>

// 全局 SD 文件系统对象（SdFat = FAT16/32，足够用）
SdFat sd;

// ES3C28P TF 卡物理接在 SDIO 引脚，SdFat 以 SPI 模式驱动（SCK=CLK/MOSI=CMD/MISO=D0/CS=D3）。
// 使用 SPI3(FSPI) 避免与屏幕 SPI2 冲突。
SPIClass SPI_SD(FSPI);

static volatile bool storage_ready = false;
static volatile bool s_recent_io_error = false;

// TF 卡身份标识
static uint32_t s_card_hash = 0;
static char s_card_snapshot_key[16] = "snap_default";

// FNV-1a 32-bit hash
static uint32_t fnv1a32(const uint8_t* data, size_t len)
{
    uint32_t hash = 2166136261UL;

    for (size_t i = 0; i < len; ++i) {
        hash ^= data[i];
        hash *= 16777619UL;
    }

    return hash;
}

// 轻量探测用缓冲区。读 sector 0 可以强制触碰物理卡，
// 比 root.open("/") 更可靠，避免目录对象缓存导致误判"卡还在"。
static uint8_t s_probe_sector[512];

static void storage_refresh_card_identity_locked()
{
    s_card_hash = 0;
    snprintf(s_card_snapshot_key,
             sizeof(s_card_snapshot_key),
             "snap_default");

    if (!sd.card()) {
        Serial.println("[存储] 跳过卡身份读取：无卡对象");
        return;
    }

    cid_t cid;
    memset(&cid, 0, sizeof(cid));

    if (!sd.card()->readCID(&cid)) {
        Serial.printf("[存储] 读取 CID 失败 错误=%u data=%u\n",
                      sd.card()->errorCode(),
                      sd.card()->errorData());
        return;
    }

    s_card_hash = fnv1a32(reinterpret_cast<const uint8_t*>(&cid), sizeof(cid));

    snprintf(s_card_snapshot_key,
             sizeof(s_card_snapshot_key),
             "snap_%08lX",
             static_cast<unsigned long>(s_card_hash));

    Serial.printf("[存储] TF 卡标识=%08lX，快照键=%s\n",
                  static_cast<unsigned long>(s_card_hash),
                  s_card_snapshot_key);
}

uint32_t storage_card_hash(void)
{
    return s_card_hash;
}

const char* storage_card_snapshot_key(void)
{
    return s_card_snapshot_key;
}

void storage_clear_card_identity(void)
{
    s_card_hash = 0;
    snprintf(s_card_snapshot_key,
             sizeof(s_card_snapshot_key),
             "snap_default");
}

bool storage_mount(void)
{
    Serial.println("[存储] 挂载 TF 卡 (SdFat SDMMC 4-bit, ES3C28P)");

    // 初始化 SD 卡访问互斥锁，在 sd.begin() 之前
    if (!storage_sd_init_mutex()) {
        Serial.println("[存储] 创建 SD 互斥锁失败");
        storage_ready = false;
        return false;
    }

    StorageSdLockGuard sd_lock(2000);
    if (!sd_lock) {
        Serial.println("[存储] 挂载失败：等待 SD 锁超时");
        storage_ready = false;
        return false;
    }

    // 检查是否已经挂载，如果是则先卸载
    if (storage_ready) {
        Serial.println("[存储] 检测到已有挂载，尝试重新挂载");
        sd.end();
        storage_ready = false;
    }

    // ES3C28P 板载 TF 卡物理连接在 SDIO 专用引脚 (CLK=48/CMD=47/D0=38/D1=40/D2=39/D3=41)。
    // ESP32 上 SdFat 仅支持 SPI 模式，因此用 SPI 控制器驱动这组 GPIO
    // (SCK=CLK, MOSI=CMD, MISO=D0, CS=D3)，SD 卡以 SPI 模式访问，API 保持不变。
    SPI_SD.begin(PIN_SD_CLK, PIN_SD_D0, PIN_SD_CMD, PIN_SD_D3);  // SCK, MISO, MOSI, SS
    SdSpiConfig cfg(PIN_SD_D3, SHARED_SPI, SD_SCK_MHZ(24), &SPI_SD);

    if (!sd.begin(cfg)) {
        Serial.println("[存储] TF 卡挂载失败 (SPI over SDIO 引脚)");
        storage_ready = false;
        SPI_SD.end();
        return false;
    }

    Serial.println("[存储] TF 卡挂载成功 (SDMMC)");
    storage_ready = true;
    s_recent_io_error = false;

    storage_refresh_card_identity_locked();

    storage_list_root();
    return true;
}

bool storage_init(void)
{
    return storage_mount();
}

void storage_mark_not_ready(void)
{
    storage_ready = false;
}

bool storage_unmount(void)
{
    storage_mark_not_ready();

    StorageSdLockGuard sd_lock(2000);
    if (!sd_lock) {
        Serial.println("[存储] 卸载失败：等待 SD 锁超时");
        return false;
    }

    sd.end();
    storage_clear_card_identity();

    pinMode(PIN_SD_CS, OUTPUT);
    digitalWrite(PIN_SD_CS, HIGH);

    Serial.println("[存储] TF 卡已卸载");
    return true;
}



bool storage_is_ready(void)
{
    return storage_ready;
}

bool storage_probe_alive(void)
{
    if (!storage_ready) {
        return false;
    }

    StorageSdLockGuard sd_lock(80);
    if (!sd_lock) {
        // 锁超时不代表 TF 卡异常，可能只是音频/扫描正在访问 SD。
        return true;
    }

    if (!sd.card()) {
        Serial.println("[存储] 探测失败：无卡对象");
        return false;
    }

    // 强制读物理 0 扇区。无 CD 脚时这是比 root.open("/") 更可靠的存在性探测。
    const bool ok = sd.card()->readSector(0, s_probe_sector);
    if (!ok) {
        Serial.printf("[存储] 探测读扇区失败 错误=%u data=%u\n",
                      sd.card()->errorCode(),
                      sd.card()->errorData());
    }

    return ok;
}

void storage_report_io_error(const char* where)
{
    s_recent_io_error = true;
    Serial.printf("[存储] 记录 IO 错误：%s\n", where ? where : "(未知)");
}

bool storage_has_recent_io_error(void)
{
    return s_recent_io_error;
}

void storage_clear_io_error(void)
{
    s_recent_io_error = false;
}

void storage_list_root(void)
{
    if (!storage_ready) return;

    StorageSdLockGuard sd_lock(1000);
    if (!sd_lock) {
        Serial.println("[存储] 无法获取 SD 互斥锁");
        return;
    }

    SdFile root;
    if (!root.open("/")) {
        Serial.println("[存储] 打开根目录失败");
        return;
    }

#if LOG_LEVEL >= 3
    // 根目录列表属于启动排查日志，日常 INFO 启动不打印。
    Serial.println("[存储] 根目录列表：");
    SdFile f;
    while (f.openNext(&root, O_RDONLY)) {
        char name[128];
        f.getName(name, sizeof(name));

        if (f.isDir()) {
            Serial.printf("  %s <DIR>\n", name);
        } else {
            Serial.printf("  %s  %lu 字节\n", name, (unsigned long)f.fileSize());
        }
        f.close();
    }
#endif
    root.close();

}