#pragma once

#include <LovyanGFX.hpp>
#include "config.h"

// 中文字体（文泉驿合并点阵，来自参考项目）
extern lgfx::U8g2font g_font_cjk;

class UI {
public:
    void begin(LGFX* lcd);
    void setTab(int tab);
    int getTab() const { return _tab; }
    void render();
    void onTouch(uint16_t x, uint16_t y, bool pressed);
    void tick(); // 定时刷新（进度/歌词滚动）
private:
    LGFX* _lcd = nullptr;
    int _tab = 0;
    unsigned long _last_tick = 0;
    unsigned long _last_render = 0;

    // 触摸交互状态
    bool _live_paused = false;

    void drawStatusBar();
    void drawNavBar();
    void drawPlayerTab();
    void drawPlaylistTab();
    void drawRadioTab();
    void drawSearchTab();

    void drawRoundedRect(int x, int y, int w, int h, int r, uint16_t c);
    void drawCenteredText(int x, int y, int w, int h, const char* txt, uint16_t c, const void* font);
    int16_t textWidth(const char* txt, const void* font);
    void setCJK();   // 切换中文字体
    void setASC();   // 切换英文字体
    bool hit(int x, int y, int w, int h, uint16_t tx, uint16_t ty);

    // 页面内交互处理
    void handlePlayerTouch(uint16_t x, uint16_t y);
    void handlePlaylistTouch(uint16_t x, uint16_t y);
    void handleRadioTouch(uint16_t x, uint16_t y);
    void handleSearchTouch(uint16_t x, uint16_t y);
};
