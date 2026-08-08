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

// 图标色块颜色
static const uint16_t ICON_COLORS[4] = { C_PRIMARY, C_ACCENT, C_PINK, C_BLUE };
static int _icon_idx(const char* s) { return ((int)(s[0]) & 0x7) % 4; }

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

void UI::setCJK() {
    _lcd->setFont(&g_font_cjk);
    _lcd->setTextWrap(false);   // 关键：关闭自动换行，避免中文竖排
}
void UI::setASC() {
    _lcd->setFont(FONT_M);
    _lcd->setTextWrap(false);
}

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
    _lcd->setTextWrap(false);    // 兜底：关闭自动换行，防止任何 print() 中文竖排
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
    if (!txt || !*txt) return;
    _lcd->setFont(font);
    _lcd->setTextColor(c);
    _lcd->setTextWrap(false);                 // 关键：关闭自动换行，避免中文竖排
    _lcd->setTextDatum(textdatum_t::middle_center);
    int16_t cx = x + w / 2;
    int16_t cy = y + h / 2;                     // middle_center 已垂直居中
    _lcd->drawString(txt, cx, cy);
    _lcd->setTextDatum(textdatum_t::top_left);
}

void UI::drawText(int x, int y, const char* txt, uint16_t c, const lgfx::v1::IFont* font) {
    if (!txt || !*txt) return;
    _lcd->setFont(font);
    _lcd->setTextColor(c);
    _lcd->setTextWrap(false);
    _lcd->setTextDatum(textdatum_t::top_left);
    _lcd->drawString(txt, x, y);
}

// ===== 状态栏 =====
void UI::drawStatusBar() {
    _lcd->setFont(FONT_S);
    _lcd->setTextWrap(false);
    _lcd->setTextColor(C_TEXT);
    _lcd->drawString("12:08", 8, 6 + 8);
    _lcd->setTextColor(C_TEXT3);
    _lcd->drawString("WiFi", SCREEN_WIDTH - 70, 6 + 8);
    _lcd->drawString("85%", SCREEN_WIDTH - 42, 6 + 8);
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
        _lcd->setTextWrap(false);
        drawCenteredText(bx, by + 2, bw, UI_NAV_H - 12, labels[i],
                         active ? C_PRIMARY : C_TEXT3, FONT_S);
    }
}

// ===== 播放页 =====
void UI::drawPlayerTab() {
    int x = 8, y = UI_STATUS_H + 4;
    int w = SCREEN_WIDTH - 16, h = UI_CONTENT_H - 4;

    drawRoundedRect(x, y, w, h, 12, C_SURFACE);

    // 封面：更大，紫-粉渐变块模拟
    int cover_x = x + 14, cover_y = y + 12, cover_w = 78, cover_h = 78;
    drawRoundedRect(cover_x, cover_y, cover_w, cover_h, 10, C_PRIMARY);
    _lcd->fillRoundRect(cover_x + 8, cover_y + cover_h - 28, cover_w - 16, 20, 6, C_ACCENT);
    setCJK();
    drawCenteredText(cover_x, cover_y + 10, cover_w, 36, "♪", C_WHITE, &g_font_cjk);

    // 标题/歌手/专辑
    setCJK();
    int tx = cover_x + cover_w + 12;
    drawText(tx, cover_y + 10, playlist[cur_song].title, C_TEXT, &g_font_cjk);
    drawText(tx, cover_y + 30, playlist[cur_song].subtitle, C_TEXT2, &g_font_cjk);
    drawText(tx, cover_y + 48, "十一月的萧邦", C_TEXT3, &g_font_cjk);

    // 来源标签
    drawRoundedRect(tx, cover_y + 62, 50, 18, 4, C_SURFACE2);
    _lcd->fillCircle(tx + 8, cover_y + 71, 3, C_ONLINE);
    drawText(tx + 14, cover_y + 67, "网络", C_ONLINE, &g_font_cjk);

    // 歌词区（5 行，当前句高亮）
    int ly_y = cover_y + cover_h + 10;
    int ly_h = 70;
    drawRoundedRect(x + 12, ly_y, w - 24, ly_h, 8, C_SURFACE2);
    int disp = 5;
    if (disp > lyric_count) disp = lyric_count;
    int start = lyric_index - 2;
    if (start < 0) start = 0;
    if (start + disp > lyric_count) start = lyric_count - disp;
    for (int i = 0; i < disp; i++) {
        int idx = start + i;
        int ly = ly_y + 10 + i * 14;
        setCJK();
        if (idx == lyric_index) {
            _lcd->fillRoundRect(x + 16, ly - 8, w - 32, 15, 4, rgb(45, 38, 80));
            drawText(x + 22, ly - 7, lyric_lines[idx], C_PRIMARY, &g_font_cjk);
        } else {
            drawText(x + 22, ly - 7, lyric_lines[idx], C_TEXT3, &g_font_cjk);
        }
    }

    // 进度条
    int pb_y = ly_y + ly_h + 10;
    int pb_x = x + 14, pb_w = w - 28;
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
    fmt(song_elapsed_ms, buf); _lcd->drawString(buf, pb_x, pb_y + 10);
    fmt(song_duration_ms, buf); _lcd->drawString(buf, pb_x + pb_w - 32, pb_y + 10);

    // 播放控制：上一首 / 播放暂停 / 下一首
    int cy = pb_y + 34;
    int cx = x + w / 2;
    // 播放/暂停大圆按钮
    _lcd->fillCircle(cx, cy, 20, C_PRIMARY);
    drawCenteredText(cx - 20, cy - 20, 40, 40, playing ? "||" : "▶", C_WHITE, FONT_L);
    // 上一首
    drawCenteredText(cx - 62, cy - 14, 28, 28, "|◀", C_TEXT2, FONT_L);
    // 下一首
    drawCenteredText(cx + 34, cy - 14, 28, 28, "▶|", C_TEXT2, FONT_L);
}

void UI::handlePlayerTouch(uint16_t x, uint16_t y) {
    int cx = 8 + (SCREEN_WIDTH - 16) / 2;
    int cy = UI_STATUS_H + 4 + 172; // 与 drawPlayerTab 大圆按钮中心对齐
    // 上一首
    if (hit(cx - 62 - 14, cy - 14, 28, 28, x, y)) {
        cur_song = (cur_song - 1 + playlist_count) % playlist_count;
        song_elapsed_ms = 0;
        render(); return;
    }
    // 下一首
    if (hit(cx + 34 - 14, cy - 14, 28, 28, x, y)) {
        cur_song = (cur_song + 1) % playlist_count;
        song_elapsed_ms = 0;
        render(); return;
    }
    // 播放/暂停
    if (hit(cx - 20, cy - 20, 40, 40, x, y)) {
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
    drawText(x + 12, y + 12, "我的歌单", C_TEXT, &g_font_cjk);
    drawText(x + w - 78, y + 16, "SD卡 12", C_TEXT3, &g_font_cjk);

    // 模式按钮（两个胶囊：顺序播放 / 随机播放）
    const char* modes[2] = {"顺序播放", "随机播放"};
    int bw = (w - 26) / 2;
    for (int i = 0; i < 2; i++) {
        int bx = x + 10 + i * (bw + 6);
        int by = y + 34;
        bool active = (play_mode == i);
        drawRoundedRect(bx, by, bw, 26, 10, active ? C_PRIMARY : C_SURFACE2);
        setCJK();
        _lcd->setTextColor(active ? C_WHITE : C_TEXT2);
        drawCenteredText(bx, by, bw, 26, modes[i], active ? C_WHITE : C_TEXT2, &g_font_cjk);
    }

    // 歌曲列表
    int list_y = y + 70;
    int max_rows = (h - 74) / 32;
    if (max_rows > playlist_count) max_rows = playlist_count;
    for (int i = 0; i < max_rows; i++) {
        int row_y = list_y + i * 32;
        bool active = (i == cur_song);
        // 左侧彩色小图标
        int ic = _icon_idx(playlist[i].title);
        _lcd->fillRoundRect(x + 12, row_y + 2, 26, 26, 4, ICON_COLORS[ic]);
        drawCenteredText(x + 12, row_y + 2, 26, 26, "♪", C_WHITE, FONT_S);
        // 文字
        setCJK();
        drawText(x + 46, row_y + 4, playlist[i].title, active ? C_PRIMARY : C_TEXT, &g_font_cjk);
        drawText(x + 46, row_y + 18, playlist[i].subtitle, C_TEXT3, &g_font_cjk);
        // 序号 / 网络来源
        _lcd->setFont(FONT_S);
        _lcd->setTextColor(C_TEXT3);
        char numbuf[4];
        snprintf(numbuf, sizeof(numbuf), "%d", i + 1);
        _lcd->drawString(numbuf, x + w - 34, row_y + 10);
    }
}

void UI::handlePlaylistTouch(uint16_t x, uint16_t y) {
    int x0 = 8, y0 = UI_STATUS_H + 4;
    int w = SCREEN_WIDTH - 16;
    int h = UI_CONTENT_H - 4;
    // 模式按钮
    int bw = (w - 26) / 2;
    for (int i = 0; i < 2; i++) {
        int bx = x0 + 10 + i * (bw + 6);
        int by = y0 + 34;
        if (hit(bx, by, bw, 26, x, y)) { play_mode = (PlayMode)i; render(); return; }
    }
    // 歌曲行
    int list_y = y0 + 70;
    int max_rows = (h - 74) / 32;
    if (max_rows > playlist_count) max_rows = playlist_count;
    for (int i = 0; i < max_rows; i++) {
        int row_y = list_y + i * 32;
        if (hit(x0 + 8, row_y - 2, w - 16, 32, x, y)) {
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
    drawText(x + 12, y + 12, "电台", C_TEXT, &g_font_cjk);
    drawText(x + w - 78, y + 16, "8 在线", C_TEXT3, &g_font_cjk);

    // 分类标签
    int cat_y = y + 34;
    int cx = x + 8;
    for (int i = 0; i < radio_category_count; i++) {
        int tw = textWidth(radio_categories[i], &g_font_cjk) + 16;
        bool active = (i == cur_radio_cat);
        drawRoundedRect(cx, cat_y, tw, 20, 10, active ? C_PRIMARY : C_SURFACE2);
        setCJK();
        _lcd->setTextColor(active ? C_WHITE : C_TEXT2);
        drawCenteredText(cx, cat_y, tw, 20, radio_categories[i], active ? C_WHITE : C_TEXT2, &g_font_cjk);
        cx += tw + 6;
    }

    // 当前播放卡片
    int card_y = cat_y + 28;
    drawRoundedRect(x + 8, card_y, w - 16, 66, 10, C_SURFACE2);
    // 左侧彩色图标（模拟麦克风/收音机）
    _lcd->fillRoundRect(x + 16, card_y + 9, 48, 48, 6, C_PINK);
    _lcd->fillRoundRect(x + 24, card_y + 21, 32, 24, 4, C_WHITE);
    _lcd->fillRect(x + 28, card_y + 27, 24, 4, C_PINK);
    setCJK();
    drawText(x + 72, card_y + 12, "华语流行 FM", C_TEXT, &g_font_cjk);
    drawText(x + 72, card_y + 30, "正在播放: 晴天 - 周杰伦", C_TEXT2, &g_font_cjk);
    _lcd->fillCircle(x + 76, card_y + 48, 3, C_PRIMARY);
    drawText(x + 84, card_y + 46, "直播 128kbps", C_TEXT3, &g_font_cjk);
    // 右侧暂停按钮
    _lcd->fillCircle(x + w - 42, card_y + 33, 16, C_PRIMARY);
    drawCenteredText(x + w - 58, card_y + 17, 32, 32, "||", C_WHITE, FONT_M);

    // 电台列表
    int list_y = card_y + 72;
    const char* radios[2] = {"华语流行 FM", "古典音乐厅"};
    const char* descs[2] = {"热歌 24 小时", "古典不间断"};
    const char* freqs[2] = {"FM 88.7", "FM 92.1"};
    uint16_t colors[2] = {C_ACCENT, C_BLUE};
    for (int i = 0; i < 2; i++) {
        int row_y = list_y + i * 34;
        _lcd->fillRoundRect(x + 10, row_y, 34, 28, 4, colors[i]);
        // 简单图标：麦克风 / 吉他
        if (i == 0) {
            _lcd->fillRect(x + 22, row_y + 6, 10, 16, C_WHITE);
            _lcd->fillCircle(x + 27, row_y + 18, 6, C_WHITE);
        } else {
            _lcd->drawFastHLine(x + 16, row_y + 20, 22, C_WHITE);
            _lcd->fillCircle(x + 18, row_y + 20, 4, C_WHITE);
            _lcd->fillCircle(x + 36, row_y + 20, 4, C_WHITE);
        }
        setCJK();
        drawText(x + 52, row_y + 4, radios[i], C_TEXT, &g_font_cjk);
        drawText(x + 52, row_y + 18, descs[i], C_TEXT3, &g_font_cjk);
        drawText(x + w - 58, row_y + 10, freqs[i], C_PRIMARY, &g_font_cjk);
    }
}

void UI::handleRadioTouch(uint16_t x, uint16_t y) {
    int x0 = 8, y0 = UI_STATUS_H + 4;
    int w = SCREEN_WIDTH - 16;
    // 分类标签
    int cat_y = y0 + 34;
    int cx = x0 + 8;
    for (int i = 0; i < radio_category_count; i++) {
        int tw = textWidth(radio_categories[i], &g_font_cjk) + 16;
        if (hit(cx, cat_y, tw, 20, x, y)) { cur_radio_cat = i; render(); return; }
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
    drawRoundedRect(x + 8, sb_y, w - 48, 28, 12, C_SURFACE2);
    _lcd->setFont(FONT_S);
    _lcd->setTextColor(C_TEXT3);
    _lcd->drawString("Q", x + 18, sb_y + 10);
    setCJK();
    drawText(x + 34, sb_y + 8, "搜索歌曲、歌手...", C_TEXT3, &g_font_cjk);
    drawRoundedRect(x + w - 36, sb_y, 28, 28, 10, C_PRIMARY);
    _lcd->setFont(FONT_S);
    _lcd->setTextColor(C_WHITE);
    _lcd->drawString("🎤", x + w - 28, sb_y + 10);

    // 历史
    int hist_y = sb_y + 38;
    setCJK();
    drawText(x + 10, hist_y, "搜索历史", C_TEXT3, &g_font_cjk);
    int hx = x + 12;
    for (int i = 0; i < search_history_count; i++) {
        int tw = textWidth(search_history[i], &g_font_cjk) + 18;
        drawRoundedRect(hx, hist_y + 10, tw, 22, 11, C_SURFACE2);
        _lcd->setTextColor(C_TEXT2);
        drawCenteredText(hx, hist_y + 10, tw, 22, search_history[i], C_TEXT2, &g_font_cjk);
        hx += tw + 6;
    }

    // 搜索结果
    int res_y = hist_y + 44;
    setCJK();
    drawText(x + 10, res_y, "搜索结果", C_TEXT3, &g_font_cjk);
    for (int i = 0; i < search_result_count; i++) {
        int row_y = res_y + 14 + i * 34;
        int ic = _icon_idx(search_results[i].title);
        _lcd->fillRoundRect(x + 10, row_y, 34, 28, 4, ICON_COLORS[ic]);
        drawCenteredText(x + 10, row_y, 34, 28, "♪", C_WHITE, FONT_S);
        drawText(x + 52, row_y + 4, search_results[i].title, C_TEXT, &g_font_cjk);
        drawText(x + 52, row_y + 18, search_results[i].subtitle, C_TEXT3, &g_font_cjk);
    }

    // 滚动条
    _lcd->fillRoundRect(x + w - 14, res_y + 14, 4, 46, 2, C_DISABLED);
    _lcd->fillRoundRect(x + w - 14, res_y + 22, 4, 18, 2, C_TEXT2);
}

void UI::handleSearchTouch(uint16_t x, uint16_t y) {
    int x0 = 8, y0 = UI_STATUS_H + 4;
    int w = SCREEN_WIDTH - 16;
    int sb_y = y0 + 12;
    int hist_y = sb_y + 38;
    int hx = x0 + 12;
    for (int i = 0; i < search_history_count; i++) {
        int tw = textWidth(search_history[i], &g_font_cjk) + 18;
        if (hit(hx, hist_y + 10, tw, 22, x, y)) {
            // 点历史项：填入搜索结果（演示）
            render(); return;
        }
        hx += tw + 6;
    }
    // 语音按钮
    if (hit(x0 + w - 36, sb_y, 28, 28, x, y)) { render(); return; }
}
