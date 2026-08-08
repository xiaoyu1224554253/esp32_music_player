# ESP32-S3 音乐播放器 (Arduino IDE + LovyanGFX)

基于参考项目 `AppStateV2.5.0round23b-radio-backend-no-external-dep-main` 重构，迁移到 **Arduino IDE** 编译，针对 **LCDWiki ES3C28P** (ILI9341 320×240 横屏 + FT6336 触摸)。

## 硬件
- 主控：ESP32-S3 N16R8
- 屏幕：ILI9341V 2.8" 320×240 SPI
- 触摸：FT6336G I2C
- 音频功放：I2S

## 引脚
见 `config.h`。

## 依赖库
在 Arduino IDE 库管理器中安装：
- **LovyanGFX** (by lovyan03)

## 打开方式
1. 在 Arduino IDE 中选择 **文件 → 打开**。
2. 选择 `c:\Users\WS\Desktop\567\music_player_arduino\music_player_arduino.ino`。
3. 开发板选择 **ESP32S3 Dev Module**（或自定义分区 16MB Flash）。
4. 编译并上传到 COM5。

## 当前实现
- ILI9341 驱动（20MHz SPI）
- FT6336 触摸读取
- 4 个标签页 1:1 复刻设计：播放 / 歌单 / 电台 / 搜索
- 触摸点击底部导航切换页面
- 播放状态、歌词、进度条、电台分类/列表、搜索历史/结果等 mock 数据

## 后续待完成
- 中文字体：LovyanGFX 默认字体不含中文，需用 [LovyanGFX Font Converter](https://m5stack.github.io/M5GFX/) 或 BDF 字库生成自定义字体并替换 FONT_M/FONT_L
- 音频播放：从参考项目中提取 VS1053 / I2S 音频后端，替换 `player.cpp` 中的 mock
- 触摸交互：页面内按钮（播放/暂停、切歌、模式切换、分类筛选、搜索）
- SD 卡 / 网络音乐源集成

## 复刻依据
- `music_player_ui_prototype.html` 中的颜色、布局、交互
- 4 张截图：播放页、歌单页、电台页、搜索页
