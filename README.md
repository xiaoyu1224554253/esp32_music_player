# ESP32 Music Player Firmware

LCDWiki 2.8inch ESP32-S3 Display (ES3C28P) 音乐播放器固件。

- **主控**: ESP32-S3
- **屏幕**: ILI9341V, 240x320, SPI
- **触摸**: FT6336, I2C
- **板级参考**: freenove-esp32s3-display-2.8-lcd

## 构建方式

所有编译均在 **GitHub 云端** 完成（本机不编译）：

- **ESP-IDF**: GitHub Actions 工作流 `build-esp-idf.yml`，使用 `espressif/esp-idf` 官方 action
- **Arduino / PlatformIO**: GitHub Actions 工作流 `build-platformio.yml`
- **Codespaces**: 提供 `.devcontainer/`，浏览器内直接编译

推送代码即自动触发云端编译，产物（`.bin` 固件）作为 Actions Artifact 下载。

## 目录结构

```
main/                  ESP-IDF 主组件 (CMakeLists.txt + main.c 占位)
components/            驱动组件占位 (ILI9341V / FT6336)
platformio.ini         Arduino / PlatformIO 构建配置
CMakeLists.txt         ESP-IDF 顶层构建
sdkconfig.defaults     ESP-IDF 默认配置
.github/workflows/     云端编译工作流
.devcontainer/         Codespaces 开发容器
```

## 本地开发（可选）

如需本机编译，请使用 GitHub Codespaces（云端），或自行配置 ESP-IDF / PlatformIO 环境。
