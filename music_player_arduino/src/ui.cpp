#include "ui.h"
#include "config.h"
#include "player.h"
#include <algorithm>

// 文泉驿合并点阵中文字体
lgfx::U8g2font g_font_cjk(u8g2_font_wenquanyi_merged);

UI::UI(LGFX* display) : lcd(display) {
    for (int i = 0; i < 4; i++) {
        tabs[i].icon = (UI::TabIcon)i;
        tabs[i].label = (i == 0 ? "播放" : i == 1 ? "歌单" : i == 2 ? "电台" : "搜索");
        tabs[i].page = (UI::Page)i;
    }
    current_page = PAGE_PLAY;
    prev_page = PAGE_PLAY;
    selected_index = 0;
    playing = false;
    search_text[0] = 0;
    selected_source = 0;
    radio_search_active = false;
    for (int i = 0; i < 7; i++) radio_cat_sel[i] = (i == 0);
}

// ---------- 小工具 ----------
static void drawRoundRect(LGFX* g, int x, int y, int w, int h, int r, uint16_t c) {
    g->fillRoundRect(x, y, w, h, r, c);
}
static void strokeRoundRect(LGFX* g, int x, int y, int w, int h, int r, uint16_t c) {
    g->drawRoundRect(x, y, w, h, r, c);
}
static int textW(LGFX* g, const char* s) { return g->textWidth(s); }
static void centerText(LGFX* g, const char* s, int cx, int y, uint16_t c, float sz = 1.0f) {
    g->setTextSize(sz); g->setTextColor(c);
    int w = textW(g, s);
    g->setCursor(cx - w / 2, y);
    g->print(s);
}

// 简单 UTF-8 字节长度(用于截断)
static int utf8Len(const char* s) {
    int n = 0; while (s[n]) n++;
    return n;
}

// ---------- 状态栏 ----------
void UI::begin(LGFX* lcd) {
    this->lcd = lcd;
    lcd->setFont(&g_font_cjk);
}

void UI::drawStatusBar() {
    lcd->fillRect(0, 0, SCREEN_WIDTH, UI_STATUS_H, C_BG);
    lcd->setTextSize(1.0f);
    lcd->setTextColor(C_TEXT);
    lcd->setCursor(8, 4);
    lcd->print("12:08");
    // 右侧: WiFi 图标 + 电量
    int rx = SCREEN_WIDTH - 8;
    lcd->setTextColor(C_TEXT2);
    lcd->setCursor(rx - lcd->textWidth("85%") - 18, 4);
    lcd->print("85%");
    // WiFi 弧线(简化)
    lcd->drawFastHLine(rx - 14, 11, 8, C_TEXT2);
    lcd->drawFastHLine(rx - 11, 8, 2, C_TEXT2);
    lcd->drawFastHLine(rx - 8, 8, 2, C_TEXT2);
}

// ---------- 底栏 ----------
void UI::drawNavBar() {
    int y = SCREEN_HEIGHT - UI_NAV_H;
    lcd->fillRect(0, y, SCREEN_WIDTH, UI_NAV_H, C_BG);
    lcd->drawFastHLine(0, y, SCREEN_WIDTH, C_TRACK_LINE);
    int tabW = SCREEN_WIDTH / TAB_COUNT;
    for (int i = 0; i < TAB_COUNT; i++) {
        int x = i * tabW;
        bool sel = (tabs[i].page == current_page);
        if (sel) {
            // 半透明紫底(近似)
            lcd->fillRoundRect(x + 6, y + 3, tabW - 12, UI_NAV_H - 6, 6, C_NAV_SEL_BG);
        }
        uint16_t col = sel ? C_PRIMARY : C_TEXT3;
        int cx = x + tabW / 2;
        drawTabIcon(tabs[i].icon, cx, y + 8, col);
        lcd->setTextSize(0.9f);
        lcd->setTextColor(col);
        int tw = lcd->textWidth(tabs[i].label);
        lcd->setCursor(cx - tw / 2, y + 19);
        lcd->print(tabs[i].label);
    }
}

void UI::drawTabIcon(TabIcon ic, int cx, int cy, uint16_t col) {
    lcd->setTextColor(col);
    lcd->setTextSize(1.0f);
    switch (ic) {
        case TAB_PLAY:  lcd->setCursor(cx - 4, cy); lcd->print(">"); break;
        case TAB_LIST:  lcd->setCursor(cx - 4, cy); lcd->print("="); break;
        case TAB_RADIO: lcd->setCursor(cx - 4, cy); lcd->print("("); break;
        case TAB_SEARCH:lcd->setCursor(cx - 4, cy); lcd->print("?"); break;
    }
}

// ---------- 封面(圆形渐变) ----------
void UI::drawCover(int x, int y, int d, uint16_t c1, uint16_t c2, uint16_t c3) {
    // 外圈白晕
    lcd->fillCircle(x + d / 2, y + d / 2, d / 2 + 1, C_SURFACE2);
    // 渐变圆: 用同心圆近似(内深外浅)
    int r = d / 2;
    for (int i = r; i > 0; i--) {
        uint16_t c = (i > r * 2 / 3) ? c1 : (i > r / 3 ? c2 : c3);
        lcd->fillCircle(x + r, y + r, i, c);
    }
    // 音符
    lcd->setTextColor(C_WHITE);
    lcd->setTextSize(1.6f);
    int nw = lcd->textWidth("♪");
    lcd->setCursor(x + r - nw / 2, y + r - 6);
    lcd->print("♪");
}

// ---------- 来源标签 ----------
void UI::drawSourceTag(const char* src, int x, int y, int h) {
    uint16_t dot = (!src || src[0] == 0 || strcmp(src, "网络") == 0) ? C_ONLINE : C_SDCARD;
    int dotR = 3;
    lcd->fillCircle(x + dotR + 1, y + h / 2, dotR, dot);
    const char* label = (!src || src[0] == 0) ? "网络" : src;
    lcd->setTextSize(0.85f);
    lcd->setTextColor(dot);
    int tw = lcd->textWidth(label);
    lcd->setCursor(x + dotR * 2 + 5, y + h / 2 - 5);
    lcd->print(label);
}

// ---------- 播放页 ----------
void UI::drawPlayPage() {
    int y = UI_CONTENT_Y;
    int pad = 12;
    // 封面(圆形)
    int cov = 72;
    drawCover(pad, y, cov, C_PRIMARY, C_ACCENT, rgb(255, 107, 107));
    int tx = pad + cov + 14;
    int tw = SCREEN_WIDTH - tx - pad;
    const Song& s = playlist[cur_song];
    // 歌名
    lcd->setTextSize(1.25f); lcd->setTextColor(C_TEXT);
    auto trim = [](const char* s)->const char*{ const char* p = strchr(s, '-'); return p ? p + 2 : s; };
    const char* title = s.title;            // 歌名(取 subtitle 中 '-' 前)
    // 取歌手/专辑
    const char* artist = s.subtitle;
    const char* album = strchr(s.subtitle, '-');
    // 标题仅歌名
    char titleBuf[32];
    strncpy(titleBuf, s.title, 31); titleBuf[31] = 0;
    // 若 title 本身含 '-' 取前半
    char* dash = strchr(titleBuf, '-'); if (dash) *dash = 0;
    lcd->setCursor(tx, y + 2);
    lcd->print(titleBuf);
    // 歌手 - 专辑
    lcd->setTextSize(0.95f); lcd->setTextColor(C_TEXT2);
    char artistBuf[48];
    const char* art = s.subtitle;
    strncpy(artistBuf, art, 47); artistBuf[47] = 0;
    lcd->setCursor(tx, y + 22);
    lcd->print(artistBuf);
    // 来源标签
    drawSourceTag(s.source, tx, y + 42, 16);

    // 歌词胶囊
    int ly = y + cov + 8;
    int lh = 22;
    drawRoundRect(lcd, pad, ly, SCREEN_WIDTH - pad * 2, lh, 8, C_SURFACE);
    lcd->setTextSize(0.9f); lcd->setTextColor(C_TEXT2);
    const char* lyric = (lyric_index >= 0 && lyric_index < lyric_count) ? lyric_lines[lyric_index] : "";
    int lw = lcd->textWidth(lyric);
    int lmax = SCREEN_WIDTH - pad * 2 - 16;
    if (lw > lmax) {
        // 简单截断
        char tmp[48]; strncpy(tmp, lyric, 47); tmp[47] = 0;
        int n = strlen(tmp);
        while (n > 1 && lcd->textWidth(tmp) > lmax) tmp[--n] = 0;
        strcat(tmp, "..");
        lcd->setCursor(pad + 8, ly + 5); lcd->print(tmp);
    } else {
        lcd->setCursor(pad + 8, ly + 5); lcd->print(lyric);
    }

    // 进度条
    int py = ly + lh + 14;
    int barX = pad, barW = SCREEN_WIDTH - pad * 2, barH = 5;
    lcd->fillRoundRect(barX, py, barW, barH, 3, C_SURFACE2);
    unsigned long dur = song_duration_ms ? song_duration_ms : 1;
    unsigned long el = std::min(song_elapsed_ms, dur);
    int fillW = (int)((uint64_t)el * barW / dur);
    if (fillW < 1) fillW = 1;
    // 紫渐变填充(用主色)
    lcd->fillRoundRect(barX, py, fillW, barH, 3, C_PRIMARY);
    // 白色滑块
    lcd->fillCircle(barX + fillW, py + barH / 2, 5, C_WHITE);

    // 时间
    lcd->setTextSize(0.8f); lcd->setTextColor(C_TEXT3);
    char tb[8];
    snprintf(tb, sizeof(tb), "%lu:%02lu", el / 60000, (el / 1000) % 60);
    lcd->setCursor(barX, py + barH + 4); lcd->print(tb);
    snprintf(tb, sizeof(tb), "%lu:%02lu", dur / 60000, (dur / 1000) % 60);
    int tw2 = lcd->textWidth(tb);
    lcd->setCursor(barX + barW - tw2, py + barH + 4); lcd->print(tb);

    // 控制按钮
    int cy = py + barH + 22;
    int playD = 44, sideD = 34;
    int totalW = sideD + 16 + playD + 16 + sideD;
    int sx = (SCREEN_WIDTH - totalW) / 2;
    // 上一首
    drawRoundRect(lcd, sx, cy - sideD / 2, sideD, sideD, sideD / 2, C_SURFACE2);
    drawControlIcon(0, sx + sideD / 2, cy, C_TEXT);
    // 播放/暂停
    int px = sx + sideD + 16;
    lcd->fillCircle(px + playD / 2, cy, playD / 2, C_PRIMARY);
    drawControlIcon(playing ? 1 : 2, px + playD / 2, cy, C_WHITE);
    // 下一首
    int nx = px + playD + 16;
    drawRoundRect(lcd, nx, cy - sideD / 2, sideD, sideD, sideD / 2, C_SURFACE2);
    drawControlIcon(3, nx + sideD / 2, cy, C_TEXT);
}

void UI::drawControlIcon(int type, int cx, int cy, uint16_t col) {
    lcd->setTextColor(col);
    lcd->setTextSize(1.3f);
    switch (type) {
        case 0: // 上一首 ⏮
            lcd->setCursor(cx - 7, cy - 8); lcd->print("|<");
            break;
        case 1: // 暂停
            lcd->setCursor(cx - 5, cy - 8); lcd->print("||");
            break;
        case 2: // 播放
            lcd->setCursor(cx - 4, cy - 8); lcd->print(">");
            break;
        case 3: // 下一首 ⏭
            lcd->setCursor(cx - 7, cy - 8); lcd->print(">|");
            break;
    }
}

// ---------- 歌单页 ----------
void UI::drawPlaylistPage() {
    int y = UI_CONTENT_Y;
    int pad = 12;
    // 标题
    lcd->setTextSize(1.3f); lcd->setTextColor(C_TEXT);
    lcd->setCursor(pad, y);
    lcd->print("播放列表");
    // 右上角 首数
    char cnt[16];
    snprintf(cnt, sizeof(cnt), "%d 首", playlist_count);
    lcd->setTextSize(0.9f); lcd->setTextColor(C_TEXT3);
    int cw = lcd->textWidth(cnt);
    lcd->setCursor(SCREEN_WIDTH - pad - cw, y + 2);
    lcd->print(cnt);

    // 模式按钮(图标+文字胶囊)
    int my = y + 22;
    const char* modes[3] = {"\x10 顺序", "\x11 随机", "\x12 单曲"};
    int mw = 64, mh = 22, gap = 8;
    for (int i = 0; i < 3; i++) {
        int mx = pad + i * (mw + gap);
        bool sel = (play_mode == (PlayMode)i);
        drawRoundRect(lcd, mx, my, mw, mh, mh / 2, sel ? C_PRIMARY : C_SURFACE2);
        lcd->setTextSize(0.85f);
        lcd->setTextColor(sel ? C_WHITE : C_TEXT2);
        int tw = lcd->textWidth(modes[i] + 2); // 跳过图标占位
        lcd->setCursor(mx + mw / 2 - tw / 2, my + 5);
        lcd->print(modes[i] + 2);
        // 简单图标
        lcd->setCursor(mx + 8, my + 5);
        lcd->print(sel ? (i == 0 ? "<" : i == 1 ? "*" : "O") : (i == 0 ? "<" : i == 1 ? "*" : "O"));
    }

    // 列表
    int ly = my + mh + 10;
    int rowH = 34, rowW = SCREEN_WIDTH - pad * 2;
    for (int i = 0; i < playlist_count; i++) {
        int ry = ly + i * rowH;
        if (ry + rowH > SCREEN_HEIGHT - UI_NAV_H - 2) break;
        bool sel = (i == selected_index);
        if (sel) {
            drawRoundRect(lcd, pad, ry, rowW, rowH - 2, 8, C_SURFACE2);
        }
        // 序号/播放三角
        lcd->setTextSize(0.9f);
        lcd->setTextColor(sel ? C_PRIMARY : C_TEXT3);
        if (sel) {
            lcd->setCursor(pad + 6, ry + 9); lcd->print(">");
        } else {
            char num[4]; snprintf(num, sizeof(num), "%d", i + 1);
            lcd->setCursor(pad + 8, ry + 9); lcd->print(num);
        }
        // 歌名
        lcd->setTextSize(0.95f); lcd->setTextColor(sel ? C_TEXT : C_TEXT2);
        const char* t = playlist[i].title;
        char tb[40]; strncpy(tb, t, 39); tb[39] = 0;
        char* d = strchr(tb, '-'); if (d) *d = 0;
        lcd->setCursor(pad + 26, ry + 7); lcd->print(tb);
        // 歌手·来源
        lcd->setTextSize(0.75f); lcd->setTextColor(C_TEXT3);
        char sub[48];
        const char* art = playlist[i].subtitle;
        strncpy(sub, art, 47); sub[47] = 0;
        lcd->setCursor(pad + 26, ry + 20); lcd->print(sub);
        // 右侧状态点
        uint16_t dot = (strcmp(playlist[i].source, "网络") == 0) ? C_ONLINE : C_SDCARD;
        lcd->fillCircle(pad + rowW - 10, ry + (rowH - 2) / 2, 4, dot);
        // 分隔线
        if (i < playlist_count - 1)
            lcd->drawFastHLine(pad + 26, ry + rowH - 1, rowW - 26, C_TRACK_LINE);
    }
}

// ---------- 电台页 ----------
void UI::drawRadioPage() {
    int y = UI_CONTENT_Y;
    int pad = 12;
    // 标题
    lcd->setTextSize(1.3f); lcd->setTextColor(C_TEXT);
    lcd->setCursor(pad, y);
    lcd->print("网络电台");
    // 右上在线频道
    lcd->setTextSize(0.85f); lcd->setTextColor(C_ONLINE);
    const char* on = "8 频道在线";
    int ow = lcd->textWidth(on);
    lcd->setCursor(SCREEN_WIDTH - pad - ow, y + 2); lcd->print(on);

    // 搜索框
    int sy = y + 22;
    int sh = 26, sw = SCREEN_WIDTH - pad * 2;
    drawRoundRect(lcd, pad, sy, sw, sh, sh / 2, C_SURFACE);
    lcd->setTextSize(0.85f); lcd->setTextColor(C_TEXT3);
    lcd->setCursor(pad + 10, sy + 7);
    if (radio_search_active && search_text[0]) {
        lcd->setTextColor(C_TEXT);
        lcd->print(search_text);
    } else {
        lcd->print("搜索电台...");
    }
    // 放大镜图标
    lcd->setTextColor(C_TEXT3);
    lcd->setCursor(pad + sw - 18, sy + 7);
    lcd->print("?");

    // 分类胶囊
    int cy = sy + sh + 8;
    int capH = 20, capGap = 6;
    int cx = pad;
    for (int i = 0; i < radio_category_count; i++) {
        const char* cat = radio_categories[i];
        int cw = lcd->textWidth(cat) + 16;
        if (cx + cw > SCREEN_WIDTH - pad) { cx = pad; cy += capH + capGap; }
        bool sel = radio_cat_sel[i];
        drawRoundRect(lcd, cx, cy, cw, capH, capH / 2, sel ? C_PRIMARY : C_SURFACE2);
        lcd->setTextSize(0.8f);
        lcd->setTextColor(sel ? C_WHITE : C_TEXT3);
        int tw = lcd->textWidth(cat);
        lcd->setCursor(cx + cw / 2 - tw / 2, cy + 5);
        lcd->print(cat);
        cx += cw + capGap;
    }

    // 当前播放卡片
    int cardY = cy + capH + 8;
    int cardH = 44, cardW = SCREEN_WIDTH - pad * 2;
    if (cardY + cardH < SCREEN_HEIGHT - UI_NAV_H) {
        drawRoundRect(lcd, pad, cardY, cardW, cardH, 8, C_SURFACE2);
        // 左侧图标
        lcd->fillCircle(pad + 18, cardY + cardH / 2, 12, C_ONLINE);
        lcd->setTextColor(C_WHITE); lcd->setTextSize(0.9f);
        lcd->setCursor(pad + 14, cardY + cardH / 2 - 5); lcd->print("R");
        // 文字
        lcd->setTextSize(0.95f); lcd->setTextColor(C_TEXT);
        lcd->setCursor(pad + 38, cardY + 8); lcd->print("华语流行 FM");
        lcd->setTextSize(0.75f); lcd->setTextColor(C_TEXT3);
        lcd->setCursor(pad + 38, cardY + 24); lcd->print("正在播放：晴天");
        // 暂停按钮
        lcd->fillCircle(pad + cardW - 16, cardY + cardH / 2, 11, C_PRIMARY);
        lcd->setTextColor(C_WHITE); lcd->setTextSize(1.0f);
        lcd->setCursor(pad + cardW - 19, cardY + cardH / 2 - 5); lcd->print(playing ? "||" : ">");
    }

    // 列表
    int ly = cardY + cardH + 6;
    int rowH = 32;
    const char* titles[2] = {"城市音乐广播", "经典老歌电台"};
    const char* cats[2] = {"流行 · 96.8", "怀旧 · 102.1"};
    uint16_t dots[2] = {C_ACCENT, C_SDCARD};
    for (int i = 0; i < 2; i++) {
        int ry = ly + i * rowH;
        if (ry + rowH > SCREEN_HEIGHT - UI_NAV_H) break;
        lcd->fillCircle(pad + 10, ry + rowH / 2, 7, dots[i]);
        lcd->setTextSize(0.95f); lcd->setTextColor(C_TEXT);
        lcd->setCursor(pad + 24, ry + 6); lcd->print(titles[i]);
        lcd->setTextSize(0.75f); lcd->setTextColor(C_TEXT3);
        lcd->setCursor(pad + 24, ry + 19); lcd->print(cats[i]);
    }
}

// ---------- 搜索页 ----------
void UI::drawSearchPage() {
    int y = UI_CONTENT_Y;
    int pad = 12;
    // 搜索框
    int sy = y, sh = 32, sw = SCREEN_WIDTH - pad * 2;
    drawRoundRect(lcd, pad, sy, sw, sh, sh / 2, C_SURFACE);
    lcd->setTextSize(0.9f); lcd->setTextColor(C_TEXT3);
    lcd->setCursor(pad + 12, sy + 9);
    if (search_text[0]) { lcd->setTextColor(C_TEXT); lcd->print(search_text); }
    else lcd->print("搜索歌曲、歌手...");
    // 放大镜(搜索)按钮
    int bx = pad + sw - 30;
    drawRoundRect(lcd, bx, sy + 4, 24, sh - 8, (sh - 8) / 2, C_PRIMARY);
    lcd->setTextColor(C_WHITE); lcd->setTextSize(1.0f);
    lcd->setCursor(bx + 8, sy + 9); lcd->print("?");

    // 来源平台标签
    int ly2 = sy + sh + 8;
    const char* srcs[4] = {"自动", "网易云", "QQ音乐", "酷狗"};
    int capH = 18, capGap = 5;
    int cx = pad;
    for (int i = 0; i < 4; i++) {
        int cw = lcd->textWidth(srcs[i]) + 14;
        drawRoundRect(lcd, cx, ly2, cw, capH, capH / 2, (i == selected_source) ? C_PRIMARY : C_SURFACE2);
        lcd->setTextSize(0.78f);
        lcd->setTextColor((i == selected_source) ? C_WHITE : C_TEXT3);
        int tw = lcd->textWidth(srcs[i]);
        lcd->setCursor(cx + cw / 2 - tw / 2, ly2 + 4);
        lcd->print(srcs[i]);
        cx += cw + capGap;
    }

    // 搜索历史 或 结果
    int secY = ly2 + capH + 10;
    if (!search_text[0]) {
        lcd->setTextSize(0.85f); lcd->setTextColor(C_TEXT3);
        lcd->setCursor(pad, secY); lcd->print("搜索历史");
        int hy = secY + 8;
        int hcapH = 18, hgap = 5;
        int hx = pad;
        for (int i = 0; i < search_history_count; i++) {
            int cw = lcd->textWidth(search_history[i]) + 14;
            drawRoundRect(lcd, hx, hy, cw, hcapH, hcapH / 2, C_SURFACE2);
            lcd->setTextSize(0.78f); lcd->setTextColor(C_TEXT2);
            int tw = lcd->textWidth(search_history[i]);
            lcd->setCursor(hx + cw / 2 - tw / 2, hy + 4);
            lcd->print(search_history[i]);
            hx += cw + hgap;
        }
    } else {
        // 结果
        int ry = secY;
        int rowH = 38, rowW = SCREEN_WIDTH - pad * 2;
        for (int i = 0; i < search_result_count; i++) {
            int ry2 = ry + i * rowH;
            if (ry2 + rowH > SCREEN_HEIGHT - UI_NAV_H) break;
            bool sel = (i == selected_index);
            if (sel) drawRoundRect(lcd, pad, ry2, rowW, rowH - 2, 8, C_SURFACE2);
            uint16_t d = (strcmp(search_results[i].source, "网络") == 0) ? C_ONLINE : C_SDCARD;
            lcd->fillCircle(pad + 12, ry2 + (rowH - 2) / 2, 8, d);
            lcd->setTextColor(C_WHITE); lcd->setTextSize(0.95f);
            lcd->setCursor(pad + 28, ry2 + 7); lcd->print(search_results[i].title);
            lcd->setTextSize(0.75f); lcd->setTextColor(C_TEXT3);
            lcd->setCursor(pad + 28, ry2 + 22); lcd->print(search_results[i].subtitle);
            // 时长
            const char* dur = strchr(search_results[i].subtitle, ':');
            if (dur) {
                lcd->setTextColor(C_TEXT3);
                int dw = lcd->textWidth(dur);
                lcd->setCursor(pad + rowW - dw - 4, ry2 + 7); lcd->print(dur);
            }
        }
    }
}

// ---------- 主绘制 ----------
void UI::draw() {
    lcd->fillScreen(C_BG);
    drawStatusBar();
    switch (current_page) {
        case PAGE_PLAY:    drawPlayPage(); break;
        case PAGE_PLAYLIST:drawPlaylistPage(); break;
        case PAGE_RADIO:   drawRadioPage(); break;
        case PAGE_SEARCH:  drawSearchPage(); break;
    }
    drawNavBar();
}

void UI::tick() {
    draw();
}

// ---------- 触摸处理 ----------
void UI::onTouch(uint16_t x, uint16_t y, bool pressed) {
    if (!pressed) return;
    int navY = SCREEN_HEIGHT - UI_NAV_H;
    if (y >= navY) {
        int tabW = SCREEN_WIDTH / TAB_COUNT;
        int idx = x / tabW;
        if (idx < 0) idx = 0; if (idx > 3) idx = 3;
        current_page = (Page)idx;
        prev_page = current_page;
        selected_index = 0;
        return;
    }
    switch (current_page) {
        case PAGE_PLAY: touchPlay(x, y); break;
        case PAGE_PLAYLIST: touchPlaylist(x, y); break;
        case PAGE_RADIO: touchRadio(x, y); break;
        case PAGE_SEARCH: touchSearch(x, y); break;
    }
}

void UI::touchPlay(uint16_t x, uint16_t y) {
    int pad = 12;
    int py = UI_CONTENT_Y + 72 + 8 + 22 + 14 + 5; // 进度条附近
    int cy = py + 22;
    int playD = 44, sideD = 34;
    int totalW = sideD + 16 + playD + 16 + sideD;
    int sx = (SCREEN_WIDTH - totalW) / 2;
    int px = sx + sideD + 16;
    int nx = px + playD + 16;
    if (y >= cy - sideD / 2 - 6 && y <= cy + sideD / 2 + 6) {
        if (x >= sx && x <= sx + sideD) { // 上一首
            cur_song = (cur_song - 1 + playlist_count) % playlist_count;
            lyric_index = 0;
        } else if (x >= px && x <= px + playD) { // 播放/暂停
            playing = !playing;
        } else if (x >= nx && x <= nx + sideD) { // 下一首
            cur_song = (cur_song + 1) % playlist_count;
            lyric_index = 0;
        }
    }
}

void UI::touchPlaylist(uint16_t x, uint16_t y) {
    int y0 = UI_CONTENT_Y;
    int pad = 12;
    int my = y0 + 22;
    int mh = 22, mw = 64, gap = 8;
    if (y >= my && y <= my + mh) {
        for (int i = 0; i < 3; i++) {
            int mx = pad + i * (mw + gap);
            if (x >= mx && x <= mx + mw) { play_mode = (PlayMode)i; return; }
        }
    }
    int ly = my + mh + 10;
    int rowH = 34;
    for (int i = 0; i < playlist_count; i++) {
        int ry = ly + i * rowH;
        if (y >= ry && y <= ry + rowH) {
            selected_index = i;
            cur_song = i;
            current_page = PAGE_PLAY;
            return;
        }
    }
}

void UI::touchRadio(uint16_t x, uint16_t y) {
    int y0 = UI_CONTENT_Y;
    int pad = 12;
    int sy = y0 + 22, sh = 26, sw = SCREEN_WIDTH - pad * 2;
    if (y >= sy && y <= sy + sh) {
        radio_search_active = !radio_search_active;
        return;
    }
    int cy = sy + sh + 8;
    int capH = 20, capGap = 6;
    int cx = pad;
    for (int i = 0; i < radio_category_count; i++) {
        const char* cat = radio_categories[i];
        int cw = lcd->textWidth(cat) + 16;
        if (cx + cw > SCREEN_WIDTH - pad) { cx = pad; cy += capH + capGap; }
        if (y >= cy && y <= cy + capH && x >= cx && x <= cx + cw) {
            for (int j = 0; j < radio_category_count; j++) radio_cat_sel[j] = (j == i);
            return;
        }
        cx += cw + capGap;
    }
    // 卡片暂停按钮
    int cardY = cy + capH + 8;
    int cardH = 44, cardW = SCREEN_WIDTH - pad * 2;
    if (y >= cardY && y <= cardY + cardH && x >= pad + cardW - 27 && x <= pad + cardW - 5) {
        playing = !playing;
    }
}

void UI::touchSearch(uint16_t x, uint16_t y) {
    int y0 = UI_CONTENT_Y;
    int pad = 12;
    int sy = y0, sh = 32, sw = SCREEN_WIDTH - pad * 2;
    int bx = pad + sw - 30;
    if (y >= sy && y <= sy + sh) {
        if (x >= bx && x <= bx + 24) {
            // 搜索触发
            if (!search_text[0]) strcpy(search_text, "夜曲");
            return;
        }
        radio_search_active = !radio_search_active; // 复用: 进入输入
        return;
    }
    int ly2 = sy + sh + 8;
    int capH = 18, capGap = 5;
    int cx = pad;
    for (int i = 0; i < 4; i++) {
        int cw = lcd->textWidth((i==0?"自动":i==1?"网易云":i==2?"QQ音乐":"酷狗")) + 14;
        if (y >= ly2 && y <= ly2 + capH && x >= cx && x <= cx + cw) {
            selected_source = i; return;
        }
        cx += cw + capGap;
    }
    if (search_text[0]) {
        int secY = ly2 + capH + 10;
        int rowH = 38;
        for (int i = 0; i < search_result_count; i++) {
            int ry = secY + i * rowH;
            if (y >= ry && y <= ry + rowH) {
                selected_index = i;
                return;
            }
        }
    }
}
