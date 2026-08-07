# ESP32 Music Player Firmware (灵镜 AI 音响)

LCDWiki 2.8" ESP32-S3 Display (ES3C28P) 音乐播放器固件，完美复刻 `music_player_ui_prototype.html` UI。

- **主控**: ESP32-S3 N16R8 (16MB Flash + 8MB PSRAM)
- **屏幕**: ILI9341V, 320x240, SPI
- **触摸**: FT6336G, I2C
- **音频**: I2S (板载功放接口)
- **板级参考**: freenove-esp32s3-display-2.8-lcd

## 技术栈

- **ESP-IDF 6.0** + **LVGL 8.3**（GUI，完整复刻 320x240 深色主题）
- **全中文字库**：CI 用 `lv_font_conv` 从 Noto CJK 生成 GB2312 字库（编译进 Flash）
- 4 个页面：播放 / 歌单 / 电台 / 搜索 + 底部导航，交互逻辑对齐原型

## 构建（全部云端完成）

- **GitHub Actions**：`.github/workflows/build-esp-idf.yml` 自动生成中文全字库并编译，产物 `.bin` 作为 Artifact
- **Codespaces**：`.devcontainer/` 提供 ESP-IDF 6.0 环境，浏览器内编译

## 目录结构

```
main/
  board.h          板级 GPIO 定义 (ILI9341V / FT6336 / I2S / SD)
  disp_driver.c/h  SPI 显示驱动 (esp_lcd + ILI9341V)
  tp_driver.c/h    I2C 触摸驱动 (FT6336) + LVGL 输入回调
  ui.c             完整 LVGL UI (4 页 + 导航 + 交互)
  ui_fonts.c       字体声明 (CI 生成覆盖)
  gen_fonts.py     中文字库生成脚本 (lv_font_conv)
  main.c           app_main: LVGL 初始化 + 主循环
  partitions.csv   自定义分区表 (含 font 分区)
  idf_component.yml 依赖 (lvgl, esp_lcd_ili9341)
```

## 本地开发（可选）

用 GitHub Codespaces（云端）。如需本机编译，自行配置 ESP-IDF 6.0 环境。
