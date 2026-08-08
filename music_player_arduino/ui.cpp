#include "ui.h"
#include "player.h"
#include <math.h>

// 中文字体（文泉驿）
lgfx::U8g2font g_font_cjk(u8g2_font_wenquanyi_merged);

// 英文字体
#define FONT_S (&fonts::Font0)   // 6x8
#define FONT_M (&fonts::Font2)   // 10x16
#define FONT_L (&fonts::Font4)   // 12x24
#define FONT_XL (&fonts::Font6)  // 16x32

void UI::begin(LGFX* lcd) {
    _lcd = lcd;
    _last_tick = millis();
    _last_render = millis();
}

void UI::setTab(int tab) {
    if (tab < 0) tab = 0;
    if (tab >= TAB_COUNT) tab = TAB_COUNT - 1;
    _tab = tab;
}

void UI::setCJK() { _lcd->setFont(&g_font_cjk); }
void UI::setASC() { _lcd->setFont(FONT_M); }

bool UI::hit(int x, int y, int w, int h, uint16_t tx, uint16_t ty) {
    return (tx >= x && tx < x + w && ty >= y && ty < y + h);
}

void UI::onTouch(uint16_t x, uint16_t y, bool pressed) {
    if (!pressed) return;
    // 底部导航
    int nav_y = SCREEN_HEIGHT - UI_NAV_H;
    if (y >= nav_y) {
        int bw = SCREEN_WIDTH / TAB_COUNT;
        int t = x / bw;
        if (t != _tab) { setTab(t); render(); }
        return;
    }
    switch (_tab) {
        case 0: handlePlayerTouch(x, y); break;
        case 1: handlePlaylistTouch(x, y); break;
        case 2: handleRadioTouch(x, y); break;
        case 3: handleSearchTouch(x, y); break;
    }
}

void UI::tick() {
    unsigned long now = millis();
    // 播放进度推进
    if (playing && _tab == 0) {
        if (now - _last_tick >= 1000) {
            _last_tick = now;
            song_elapsed_ms += 1000;
            if (song_elapsed_ms >= song_duration_ms) {
                song_elapsed_ms = song_duration_ms;
                playing = false;
            }
            render(); // 刷新播放页进度
        }
    }
}

void UI::render() {
    if (!_lcd) return;
    _lcd->startWrite();
    _lcd->fillScreen(C_BG);
    drawStatusBar();
    switch (_tab) {
        case 0: drawPlayerTab(); break;
        case 1: drawPlaylistTab(); break;
        case 2: drawRadioTab(); break;
        case 3: drawSearchTab(); break;
    }
    drawNavBar();
    _lcd->endWrite();
}

void UI::drawRoundedRect(int x, int y, int w, int h, int r, uint16_t c) {
    _lcd->fillRoundRect(x, y, w, h, r, c);
}

int16_t UI::textWidth(const char* txt, const lgfx::v1::IFont* font) {
    _lcd->setFont(font);
    return _lcd->textWidth(txt);
}

void UI::drawCenteredText(int x, int y, int w, int h, const char* txt, uint16_t c, const lgfx::v1::IFont* font) {
    _lcd->setFont(font);
    _lcd->setTextColor(c);
    int16_t tw = _lcd->textWidth(txt);
    int16_t th = _lcd->fontHeight();
    _lcd->setCursor(x + (w - tw) / 2, y + (h - th) / 2 + th);
    _lcd->print(txt);
}

// ===== 状态栏 =====
void UI::drawStatusBar() {
    _lcd->setFont(FONT_S);
    _lcd->setTextColor(C_TEXT);
    _lcd->setCursor(8, 6 + 8);
    _lcd->print("12:08");

    _lcd->setTextColor(C_TEXT3);
    _lcd->setCursor(SCREEN_WIDTH - 70, 6 + 8);
    _lcd->print("WiFi");

    _lcd->setCursor(SCREEN_WIDTH - 42, 6 + 8);
    _lcd->print("85%");
}

// ===== 底部导航 =====
void UI::drawNavBar() {
    int y = SCREEN_HEIGHT - UI_NAV_H;
    _lcd->fillRoundRect(8, y + 4, SCREEN_WIDTH - 16, UI_NAV_H - 8, 8, C_SURFACE);
    int bw = (SCREEN_WIDTH - 16) / TAB_COUNT;
    const char* labels[TAB_COUNT] = {"播放", "歌单", "电台", "搜索"};
    int icons[TAB_COUNT] = {0, 1, 2, 3}; // 仅占位
    (void)icons;
    for (int i = 0; i < TAB_COUNT; i++) {
        int bx = 8 + i * bw;
        int by = y + 4;
        bool active = (i == _tab);
        if (active) {
            _lcd->fillRoundRect(bx + 2, by + 2, bw - 4, UI_NAV_H - 12, 6, rgb(31, 27, 58));
            _lcd->setTextColor(C_PRIMARY);
        } else {
            _lcd->setTextColor(C_TEXT3);
        }
        _lcd->setFont(FONT_S);
        int16_t tw = _lcd->textWidth(labels[i]);
        int16_t th = _lcd->fontHeight();
        _lcd->setCursor(bx + (bw - tw) / 2, by + (UI_NAV_H - 12 - th) / 2 + th - 1);
        _lcd->print(labels[i]);
    }
}

// ===== 播放页 =====
void UI::drawPlayerTab() {
    int x = 8, y = UI_STATUS_H + 4;
    int w = SCREEN_WIDTH - 16, h = UI_CONTENT_H - 4;

    drawRoundedRect(x, y, w, h, 12, C_SURFACE);

    // 封面
    int cover_x = x + 14, cover_y = y + 14, cover_w = 76, cover_h = 76;
    drawRoundedRect(cover_x, cover_y, cover_w, cover_h, 8, C_PRIMARY);
    setCJK();
    _lcd->setTextColor(C_WHITE);
    drawCenteredText(cover_x, cover_y, cover_w, cover_h, "♪", C_WHITE, &g_font_cjk);

    // 标题/歌手
    setCJK();
    _lcd->setTextColor(C_TEXT);
    _lcd->setCursor(cover_x + cover_w + 10, cover_y + 20);
    _lcd->print(playlist[cur_song].title);
    _lcd->setTextColor(C_TEXT2);
    _lcd->setCursor(cover_x + cover_w + 10, cover_y + 40);
    _lcd->print(playlist[cur_song].subtitle);

    // 来源标签
    drawRoundedRect(cover_x + cover_w + 10, cover_y + 54, 44, 14, 4, C_SURFACE2);
    _lcd->setTextColor(C_ONLINE);
    _lcd->setCursor(cover_x + cover_w + 16, cover_y + 65);
    _lcd->print(playlist[cur_song].source);

    // 歌词区
    int ly_y = cover_y + cover_h + 14;
    drawRoundedRect(x + 12, ly_y, w - 24, 58, 8, C_SURFACE2);
    for (int i = 0; i < lyric_count; i++) {
        int ly = ly_y + 10 + i * 11;
        setCJK();
        if (i == lyric_index) {
            _lcd->setTextColor(C_PRIMARY);
            _lcd->fillRoundRect(x + 18, ly - 9, w - 36, 12, 4, rgb(45, 40, 80));
        } else {
            _lcd->setTextColor(C_TEXT3);
        }
        _lcd->setCursor(x + 22, ly);
        _lcd->print(lyric_lines[i]);
    }

    // 进度条
    int pb_y = ly_y + 68;
    int pb_x = x + 12, pb_w = w - 24;
    _lcd->drawFastHLine(pb_x, pb_y, pb_w, C_DISABLED);
    int fill = (int)((float)song_elapsed_ms / song_duration_ms * pb_w);
    _lcd->drawFastHLine(pb_x, pb_y, fill, C_PRIMARY);
    _lcd->fillCircle(pb_x + fill, pb_y, 5, C_WHITE);

    char buf[8];
    auto fmt = [](unsigned long ms, char* b) {
        int s = ms / 1000;
        snprintf(b, 8, "%d:%02d", s / 60, s % 60);
    };
    _lcd->setFont(FONT_S);
    _lcd->setTextColor(C_TEXT3);
    fmt(song_elapsed_ms, buf); _lcd->setCursor(pb_x, pb_y + 12); _lcd->print(buf);
    fmt(song_duration_ms, buf); _lcd->setCursor(pb_x + pb_w - 32, pb_y + 12); _lcd->print(buf);

    // 控制
    int cy = pb_y + 34;
    int cx = x + w / 2;
    drawRoundedRect(cx - 40, cy - 14, 80, 28, 14, C_PRIMARY);
    setCJK();
    drawCenteredText(cx - 40, cy - 14, 80, 28, playing ? "暂停" : "播放", C_WHITE, &g_font_cjk);
    _lcd->setTextColor(C_TEXT2);
    _lcd->setFont(FONT_L);
    _lcd->setCursor(cx - 84, cy + 6);
    _lcd->print("|<<");
    _lcd->setCursor(cx + 52, cy + 6);
    _lcd->print(">>|");
}

void UI::handlePlayerTouch(uint16_t x, uint16_t y) {
    int nav_y = SCREEN_HEIGHT - UI_NAV_H;
    (void)nav_y;
    int cx = 8 + (SCREEN_WIDTH - 16) / 2; // 与控制按钮中心大致对齐
    int cy = UI_STATUS_H + 4 + 200;        // 播放/暂停按钮所在行
    // 上一首
    if (hit(cx - 84 - 20, cy - 10, 40, 28, x, y)) {
        cur_song = (cur_song - 1 + playlist_count) % playlist_count;
        song_elapsed_ms = 0;
        render(); return;
    }
    // 下一首
    if (hit(cx + 52 - 20, cy - 10, 40, 28, x, y)) {
        cur_song = (cur_song + 1) % playlist_count;
        song_elapsed_ms = 0;
        render(); return;
    }
    // 播放/暂停
    if (hit(cx - 40, cy - 14, 80, 28, x, y)) {
        playing = !playing;
        render(); return;
    }
}

// ===== 歌单页 =====
void UI::drawPlaylistTab() {
    int x = 8, y = UI_STATUS_H + 4;
    int w = SCREEN_WIDTH - 16, h = UI_CONTENT_H - 4;
    drawRoundedRect(x, y, w, h, 12, C_SURFACE);

    setCJK();
    _lcd->setTextColor(C_TEXT);
    _lcd->setFont(FONT_L);
    _lcd->setCursor(x + 12, y + 18);
    _lcd->print("我的歌单");
    _lcd->setFont(FONT_S);
    _lcd->setTextColor(C_TEXT3);
    _lcd->setCursor(x + w - 86, y + 20);
    _lcd->print("SD卡 12");

    // 模式按钮
    const char* modes[3] = {"顺序", "随机", "单曲"};
    int bw = (w - 28) / 3;
    for (int i = 0; i < 3; i++) {
        int bx = x + 10 + i * (bw + 4);
        int by = y + 36;
        bool active = (play_mode == i);
        drawRoundedRect(bx, by, bw, 22, 8, active ? C_PRIMARY : C_SURFACE2);
        setCJK();
        _lcd->setTextColor(active ? C_WHITE : C_TEXT2);
        drawCenteredText(bx, by, bw, 22, modes[i], active ? C_WHITE : C_TEXT2, &g_font_cjk);
    }

    // 歌曲列表
    int list_y = y + 70;
    for (int i = 0; i < playlist_count; i++) {
        int row_y = list_y + i * 38;
        if (i == cur_song) {
            drawRoundedRect(x + 8, row_y - 2, w - 16, 34, 6, rgb(25, 22, 45));
            _lcd->setTextColor(C_PRIMARY);
            _lcd->setFont(FONT_S);
            _lcd->setCursor(x + 16, row_y + 14);
            _lcd->print(">");
        }
        setCJK();
        _lcd->setTextColor(C_TEXT);
        _lcd->setCursor(x + 28, row_y + 8);
        _lcd->print(playlist[i].title);
        _lcd->setTextColor(C_TEXT3);
        _lcd->setCursor(x + 28, row_y + 22);
        _lcd->print(playlist[i].subtitle);
        _lcd->setTextColor(i == cur_song ? C_PRIMARY : C_TEXT3);
        char numbuf[4];
        snprintf(numbuf, sizeof(numbuf), "%d", i + 1);
        _lcd->setCursor(x + w - 40, row_y + 8);
        _lcd->print(numbuf);
    }
}

void UI::handlePlaylistTouch(uint16_t x, uint16_t y) {
    int x0 = 8, y0 = UI_STATUS_H + 4;
    int w = SCREEN_WIDTH - 16;
    // 模式按钮
    int bw = (w - 28) / 3;
    for (int i = 0; i < 3; i++) {
        int bx = x0 + 10 + i * (bw + 4);
        int by = y0 + 36;
        if (hit(bx, by, bw, 22, x, y)) { play_mode = (PlayMode)i; render(); return; }
    }
    // 歌曲行
    int list_y = y0 + 70;
    for (int i = 0; i < playlist_count; i++) {
        int row_y = list_y + i * 38;
        if (hit(x0 + 8, row_y - 2, w - 16, 34, x, y)) {
            cur_song = i;
            song_elapsed_ms = 0;
            render(); return;
        }
    }
}

// ===== 电台页 =====
void UI::drawRadioTab() {
    int x = 8, y = UI_STATUS_H + 4;
    int w = SCREEN_WIDTH - 16, h = UI_CONTENT_H - 4;
    drawRoundedRect(x, y, w, h, 12, C_SURFACE);

    setCJK();
    _lcd->setTextColor(C_TEXT);
    _lcd->setFont(FONT_L);
    _lcd->setCursor(x + 12, y + 16);
    _lcd->print("电台");
    _lcd->setFont(FONT_S);
    _lcd->setTextColor(C_TEXT3);
    _lcd->setCursor(x + w - 80, y + 20);
    _lcd->print("8 在线");

    // 分类标签
    int cat_y = y + 36;
    int cx = x + 8;
    for (int i = 0; i < radio_category_count; i++) {
        int tw = textWidth(radio_categories[i], &g_font_cjk) + 14;
        bool active = (i == cur_radio_cat);
        drawRoundedRect(cx, cat_y, tw, 18, 9, active ? C_PRIMARY : C_SURFACE2);
        setCJK();
        _lcd->setTextColor(active ? C_WHITE : C_TEXT2);
        drawCenteredText(cx, cat_y, tw, 18, radio_categories[i], active ? C_WHITE : C_TEXT2, &g_font_cjk);
        cx += tw + 6;
    }

    // 当前播放卡片
    int card_y = cat_y + 28;
    drawRoundedRect(x + 8, card_y, w - 16, 64, 10, C_SURFACE2);
    drawRoundedRect(x + 16, card_y + 8, 48, 48, 6, C_ACCENT);
    setCJK();
    _lcd->setTextColor(C_WHITE);
    drawCenteredText(x + 16, card_y + 8, 48, 48, "R", C_WHITE, &g_font_cjk);
    _lcd->setTextColor(C_TEXT);
    _lcd->setCursor(x + 72, card_y + 14);
    _lcd->print("华语流行 FM");
    _lcd->setTextColor(C_TEXT2);
    _lcd->setCursor(x + 72, card_y + 30);
    _lcd->print("正在播放: 晴天");
    _lcd->fillCircle(x + 72, card_y + 46, 3, C_PRIMARY);
    _lcd->setTextColor(C_TEXT3);
    _lcd->setCursor(x + 80, card_y + 49);
    _lcd->print("直播 128kbps");
    drawRoundedRect(x + w - 50, card_y + 18, 32, 32, 16, C_PRIMARY);
    setCJK();
    drawCenteredText(x + w - 50, card_y + 18, 32, 32, "暂停", C_WHITE, &g_font_cjk);

    // 电台列表
    int list_y = card_y + 74;
    const char* radios[2] = {"华语流行 FM", "古典音乐厅"};
    const char* descs[2] = {"热歌 24 小时", "古典不间断"};
    const char* freqs[2] = {"FM 88.7", "FM 91.5"};
    uint16_t colors[2] = {C_PRIMARY, C_ACCENT};
    for (int i = 0; i < 2; i++) {
        int row_y = list_y + i * 38;
        drawRoundedRect(x + 10, row_y, 34, 28, 4, colors[i]);
        setCJK();
        _lcd->setTextColor(C_WHITE);
        drawCenteredText(x + 10, row_y, 34, 28, i == 0 ? "播" : "乐", C_WHITE, &g_font_cjk);
        _lcd->setTextColor(C_TEXT);
        _lcd->setCursor(x + 52, row_y + 6);
        _lcd->print(radios[i]);
        _lcd->setTextColor(C_TEXT3);
        _lcd->setCursor(x + 52, row_y + 20);
        _lcd->print(descs[i]);
        _lcd->setTextColor(C_PRIMARY);
        _lcd->setCursor(x + w - 60, row_y + 10);
        _lcd->print(freqs[i]);
    }
}

void UI::handleRadioTouch(uint16_t x, uint16_t y) {
    int x0 = 8, y0 = UI_STATUS_H + 4;
    int w = SCREEN_WIDTH - 16;
    // 分类标签
    int cat_y = y0 + 36;
    int cx = x0 + 8;
    for (int i = 0; i < radio_category_count; i++) {
        int tw = textWidth(radio_categories[i], &g_font_cjk) + 14;
        if (hit(cx, cat_y, tw, 18, x, y)) { cur_radio_cat = i; render(); return; }
        cx += tw + 6;
    }
}

// ===== 搜索页 =====
void UI::drawSearchTab() {
    int x = 8, y = UI_STATUS_H + 4;
    int w = SCREEN_WIDTH - 16, h = UI_CONTENT_H - 4;
    drawRoundedRect(x, y, w, h, 12, C_SURFACE);

    // 搜索框
    int sb_y = y + 12;
    drawRoundedRect(x + 8, sb_y, w - 52, 26, 10, C_SURFACE2);
    setCJK();
    _lcd->setTextColor(C_TEXT3);
    _lcd->setCursor(x + 18, sb_y + 17);
    _lcd->print("搜索歌曲、歌手...");
    drawRoundedRect(x + w - 40, sb_y, 32, 26, 8, C_PRIMARY);
    _lcd->setTextColor(C_WHITE);
    drawCenteredText(x + w - 40, sb_y, 32, 26, "语音", C_WHITE, &g_font_cjk);

    // 历史
    int hist_y = sb_y + 36;
    setCJK();
    _lcd->setTextColor(C_TEXT3);
    _lcd->setCursor(x + 10, hist_y);
    _lcd->print("历史");
    int hx = x + 12;
    for (int i = 0; i < search_history_count; i++) {
        int tw = textWidth(search_history[i], &g_font_cjk) + 16;
        drawRoundedRect(hx, hist_y + 8, tw, 18, 9, C_SURFACE2);
        _lcd->setTextColor(C_TEXT2);
        drawCenteredText(hx, hist_y + 8, tw, 18, search_history[i], C_TEXT2, &g_font_cjk);
        hx += tw + 6;
    }

    // 搜索结果
    int res_y = hist_y + 36;
    setCJK();
    _lcd->setTextColor(C_TEXT3);
    _lcd->setCursor(x + 10, res_y);
    _lcd->print("结果");
    for (int i = 0; i < search_result_count; i++) {
        int row_y = res_y + 12 + i * 32;
        drawRoundedRect(x + 10, row_y, 34, 26, 4, C_SURFACE2);
        _lcd->setTextColor(C_PRIMARY);
        drawCenteredText(x + 10, row_y, 34, 26, "♪", C_PRIMARY, &g_font_cjk);
        _lcd->setTextColor(C_TEXT);
        _lcd->setCursor(x + 52, row_y + 6);
        _lcd->print(search_results[i].title);
        _lcd->setTextColor(C_TEXT3);
        _lcd->setCursor(x + 52, row_y + 20);
        _lcd->print(search_results[i].subtitle);
    }

    // 滚动条
    _lcd->fillRoundRect(x + w - 14, res_y + 8, 4, 34, 2, C_DISABLED);
    _lcd->fillRoundRect(x + w - 14, res_y + 14, 4, 14, 2, C_TEXT2);
}

void UI::handleSearchTouch(uint16_t x, uint16_t y) {
    int x0 = 8, y0 = UI_STATUS_H + 4;
    int w = SCREEN_WIDTH - 16;
    int sb_y = y0 + 12;
    int hist_y = sb_y + 36;
    int hx = x0 + 12;
    for (int i = 0; i < search_history_count; i++) {
        int tw = textWidth(search_history[i], &g_font_cjk) + 16;
        if (hit(hx, hist_y + 8, tw, 18, x, y)) {
            // 点历史项：填入搜索结果（演示）
            render(); return;
        }
        hx += tw + 6;
    }
    // 语音按钮
    if (hit(x0 + w - 40, sb_y, 32, 26, x, y)) { render(); return; }
}
