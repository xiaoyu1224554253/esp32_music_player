#pragma once

#include <LovyanGFX.hpp>
#include "config.h"
#include "lgfx_config.h"
#include "u8g2_font_wenquanyi_merged.h"

// 中文字体（文泉驿合并点阵，来自参考项目）
extern lgfx::U8g2font g_font_cjk;

class UI {
public:
    enum Page { PAGE_PLAY, PAGE_PLAYLIST, PAGE_RADIO, PAGE_SEARCH };
    enum TabIcon { TAB_PLAY, TAB_LIST, TAB_RADIO, TAB_SEARCH };

    UI(LGFX* display = nullptr);

    void begin(LGFX* lcd);
    void setTab(int tab);
    int  getTab() const { return (int)current_page; }
    void render() { draw(); }
    void draw();
    void onTouch(uint16_t x, uint16_t y, bool pressed);
    void tick();

private:
    LGFX* lcd;
    Page current_page = PAGE_PLAY;
    Page prev_page = PAGE_PLAY;
    int selected_index = 0;

    struct { TabIcon icon; const char* label; Page page; } tabs[4];

    // 搜索/电台状态
    char search_text[32];
    int selected_source = 0;
    bool radio_search_active = false;
    bool radio_cat_sel[7];

    // 绘制
    void drawStatusBar();
    void drawNavBar();
    void drawTabIcon(TabIcon ic, int cx, int cy, uint16_t col);
    void drawCover(int x, int y, int d, uint16_t c1, uint16_t c2, uint16_t c3);
    void drawSourceTag(const char* src, int x, int y, int h);
    void drawControlIcon(int type, int cx, int cy, uint16_t col);

    void drawPlayPage();
    void drawPlaylistPage();
    void drawRadioPage();
    void drawSearchPage();

    void touchPlay(uint16_t x, uint16_t y);
    void touchPlaylist(uint16_t x, uint16_t y);
    void touchRadio(uint16_t x, uint16_t y);
    void touchSearch(uint16_t x, uint16_t y);
};
