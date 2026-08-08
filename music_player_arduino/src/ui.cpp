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

// 内容区域（原型 page padding: 4px 8px 2px）
#define PAGE_X 8
#define PAGE_Y (UI_STATUS_H + 4)
#define PAGE_W (SCREEN_WIDTH - 16)
#define PAGE_H (UI_CONTENT_H - 6)

// 图标色（对应原型封面/列表图标）
static const uint16_t ICON_COLORS[4] = {
    rgb(124, 108, 255),  // 紫
    rgb(168, 85, 247),   // 玫紫
    rgb(236, 72, 153),   // 粉
    rgb(0, 200, 255)     // 青蓝
};

static inline uint16_t blend(uint16_t c1, uint16_t c2, uint8_t f) {
    uint8_t r1 = (c1 >> 11) << 3, g1 = ((c1 >> 5) & 0x3F) << 2, b1 = (c1 & 0x1F) << 3;
    uint8_t r2 = (c2 >> 11) << 3, g2 = ((c2 >> 5) & 0x3F) << 2, b2 = (c2 & 0x1F) << 3;
    uint8_t r = (r1 * (255 - f) + r2 * f) >> 8;
    uint8_t g = (g1 * (255 - f) + g2 * f) >> 8;
    uint8_t b = (b1 * (255 - f) + b2 * f) >> 8;
    return rgb(r, g, b);
}

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
    _lcd->setTextWrap(false);
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
    if (playing && _tab == 0) {
        if (now - _last_tick >= 1000) {
            _last_tick = now;
            song_elapsed_ms += 1000;
            if (song_elapsed_ms >= song_duration_ms) {
                song_elapsed_ms = song_duration_ms;
                playing = false;
            }
            render();
        }
    }
}

void UI::render() {
    if (!_lcd) return;
    _lcd->setTextWrap(false);
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
    _lcd->setTextWrap(false);
    _lcd->setTextDatum(textdatum_t::middle_center);
    _lcd->drawString(txt, x + w / 2, y + h / 2);
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

// ===== 状态栏 (18px) =====
void UI::drawStatusBar() {
    _lcd->setFont(FONT_S);
    _lcd->setTextWrap(false);
    _lcd->setTextColor(C_TEXT);
    _lcd->drawString("12:08", 8, 7);
    _lcd->setTextColor(C_TEXT3);
    _lcd->drawString("WiFi", SCREEN_WIDTH - 70, 7);
    _lcd->drawString("85%", SCREEN_WIDTH - 42, 7);
}

// 绘制导航图标：音符
static void drawMusicIcon(LGFX* lcd, int cx, int cy, int sz, uint16_t c) {
    int r = sz / 2;
    lcd->fillCircle(cx, cy + r - 2, r - 2, c);
    lcd->drawFastVLine(cx + 3, cy - r + 2, sz - 2, c);
    lcd->drawFastHLine(cx + 3, cy - r + 2, 4, c);
    lcd->drawFastHLine(cx + 3, cy - r + 5, 3, c);
}

// 绘制导航图标：歌单
static void drawListIcon(LGFX* lcd, int cx, int cy, int sz, uint16_t c) {
    int y0 = cy - sz / 2 + 1;
    lcd->drawFastHLine(cx - sz / 2 + 2, y0, sz - 4, c);
    lcd->drawFastHLine(cx - sz / 2 + 2, y0 + 3, sz - 4, c);
    lcd->drawFastHLine(cx - sz / 2 + 2, y0 + 6, sz - 4, c);
}

// 绘制导航图标：电台
static void drawRadioIcon(LGFX* lcd, int cx, int cy, int sz, uint16_t c) {
    int r = sz / 2 - 1;
    lcd->drawCircle(cx, cy, r, c);
    lcd->drawFastVLine(cx, cy - r + 2, r * 2 - 3, c);
    lcd->drawFastHLine(cx - r + 2, cy, r * 2 - 3, c);
}

// 绘制导航图标：搜索
static void drawSearchIcon(LGFX* lcd, int cx, int cy, int sz, uint16_t c) {
    int r = sz / 2 - 2;
    lcd->drawCircle(cx - 1, cy - 1, r, c);
    lcd->drawLine(cx + r - 2, cy + r - 2, cx + r + 2, cy + r + 2, c);
}

// ===== 底部导航 (30px) =====
void UI::drawNavBar() {
    int y = SCREEN_HEIGHT - UI_NAV_H;
    // 全宽 surface 背景
    _lcd->fillRect(0, y, SCREEN_WIDTH, UI_NAV_H, C_SURFACE);
    int bw = SCREEN_WIDTH / TAB_COUNT;
    const char* labels[TAB_COUNT] = {"播放", "歌单", "电台", "搜索"};
    for (int i = 0; i < TAB_COUNT; i++) {
        int bx = i * bw;
        int by = y;
        bool active = (i == _tab);
        if (active) {
            _lcd->fillRoundRect(bx + 2, by + 2, bw - 4, UI_NAV_H - 4, 6, rgb(30, 26, 58));
        }
        uint16_t col = active ? C_PRIMARY : C_TEXT3;
        int cx = bx + bw / 2;
        int cy = by + 8;
        int ics = 10;
        switch (i) {
            case 0: drawMusicIcon(_lcd, cx, cy, ics, col); break;
            case 1: drawListIcon(_lcd, cx, cy, ics, col); break;
            case 2: drawRadioIcon(_lcd, cx, cy, ics, col); break;
            case 3: drawSearchIcon(_lcd, cx, cy, ics, col); break;
        }
        setCJK();
        _lcd->setTextColor(col);
        drawCenteredText(bx, by + 16, bw, 12, labels[i], col, &g_font_cjk);
    }
}

// 封面渐变块（模拟原型紫色渐变）
static void drawCoverGradient(LGFX* lcd, int x, int y, int w, int h, int r) {
    for (int i = 0; i < h; i++) {
        uint16_t c = blend(rgb(124, 108, 255), rgb(236, 72, 153), (i * 200) / h);
        lcd->drawFastHLine(x, y + i, w, c);
    }
    // 用圆角mask（简单做法：四角用背景色覆盖）
    lcd->fillCircleHelper(x + r, y + r, r, 2, h - r * 2, C_BG);
    lcd->fillCircleHelper(x + w - r - 1, y + r, r, 1, h - r * 2, C_BG);
    lcd->fillCircleHelper(x + r, y + h - r - 1, r, 4, h - r * 2, C_BG);
    lcd->fillCircleHelper(x + w - r - 1, y + h - r - 1, r, 8, h - r * 2, C_BG);
}

// 简单音符图标
static void drawNoteIcon(LGFX* lcd, int x, int y, int w, int h, uint16_t c) {
    int cx = x + w / 2, cy = y + h / 2;
    lcd->fillCircle(cx - 4, cy + 5, 4, c);
    lcd->drawFastVLine(cx + 3, cy - 6, 12, c);
    lcd->drawFastHLine(cx + 3, cy - 6, 5, c);
    lcd->drawFastHLine(cx + 3, cy - 3, 4, c);
}

// ===== 播放页 =====
void UI::drawPlayerTab() {
    int x = PAGE_X, y = PAGE_Y;
    int w = PAGE_W, h = PAGE_H;

    // 封面 70x70 圆角 8
    int cover_x = x + 8, cover_y = y + 4;
    int cover_w = 70, cover_h = 70, cover_r = 8;
    drawCoverGradient(_lcd, cover_x, cover_y, cover_w, cover_h, cover_r);
    drawNoteIcon(_lcd, cover_x, cover_y, cover_w, cover_h, C_WHITE);

    // 右侧信息
    int tx = cover_x + cover_w + 10;
    int ty = cover_y + 6;
    setCJK();
    drawText(tx, ty, playlist[cur_song].title, C_TEXT, &g_font_cjk);
    drawText(tx, ty + 18, playlist[cur_song].subtitle, C_TEXT2, &g_font_cjk);
    drawText(tx, ty + 34, "十一月的萧邦", C_TEXT3, &g_font_cjk);

    // 来源标签
    int tag_w = 60, tag_h = 16;
    int tag_x = tx, tag_y = ty + 50;
    drawRoundedRect(tag_x, tag_y, tag_w, tag_h, 4, C_SURFACE2);
    _lcd->fillCircle(tag_x + 8, tag_y + tag_h / 2, 3, C_ONLINE);
    drawText(tag_x + 14, tag_y + 3, "网络搜索", C_ONLINE, &g_font_cjk);

    // 歌词区：单行居中（原型高 34，圆角 8）
    int ly_y = cover_y + cover_h + 8;
    int ly_h = 34;
    drawRoundedRect(x + 8, ly_y, w - 16, ly_h, 8, C_SURFACE2);
    if (lyric_count > 0 && lyric_index >= 0 && lyric_index < lyric_count) {
        setCJK();
        _lcd->setTextColor(C_PRIMARY);
        _lcd->setTextDatum(textdatum_t::middle_center);
        _lcd->drawString(lyric_lines[lyric_index], x + w / 2, ly_y + ly_h / 2);
        _lcd->setTextDatum(textdatum_t::top_left);
    }

    // 进度条
    int pb_y = ly_y + ly_h + 14;
    int pb_x = x + 10, pb_w = w - 20, pb_h = 5;
    int radius = pb_h / 2;
    // 背景轨道
    _lcd->fillRoundRect(pb_x, pb_y - radius, pb_w, pb_h, radius, C_DISABLED);
    int fill = (int)((float)song_elapsed_ms / song_duration_ms * pb_w);
    if (fill < 0) fill = 0;
    if (fill > pb_w) fill = pb_w;
    _lcd->fillRoundRect(pb_x, pb_y - radius, fill, pb_h, radius, C_PRIMARY);
    _lcd->fillCircle(pb_x + fill, pb_y, radius + 1, C_WHITE);

    // 时间
    char buf[8];
    auto fmt = [](unsigned long ms, char* b) {
        int s = ms / 1000;
        snprintf(b, 8, "%d:%02d", s / 60, s % 60);
    };
    _lcd->setFont(FONT_S);
    _lcd->setTextColor(C_TEXT3);
    fmt(song_elapsed_ms, buf); _lcd->drawString(buf, pb_x, pb_y + 10);
    fmt(song_duration_ms, buf); _lcd->drawString(buf, pb_x + pb_w - 30, pb_y + 10);

    // 控制按钮
    int cy = pb_y + 42;
    int cx = x + w / 2;
    // 上一首：双竖线+箭头
    _lcd->fillCircle(cx - 52, cy, 16, C_SURFACE2);
    _lcd->fillRect(cx - 60, cy - 6, 3, 12, C_TEXT2);
    _lcd->fillRect(cx - 54, cy - 6, 3, 12, C_TEXT2);
    _lcd->fillTriangle(cx - 50, cy - 7, cx - 50, cy + 7, cx - 44, cy, C_TEXT2);
    // 播放/暂停（几何绘制）
    _lcd->fillCircle(cx, cy, 22, C_PRIMARY);
    if (playing) {
        _lcd->fillRect(cx - 7, cy - 8, 5, 16, C_WHITE);
        _lcd->fillRect(cx + 2, cy - 8, 5, 16, C_WHITE);
    } else {
        _lcd->fillTriangle(cx - 8, cy - 10, cx - 8, cy + 10, cx + 9, cy, C_WHITE);
    }
    // 下一首：箭头+双竖线
    _lcd->fillCircle(cx + 52, cy, 16, C_SURFACE2);
    _lcd->fillTriangle(cx + 44, cy - 7, cx + 44, cy + 7, cx + 50, cy, C_TEXT2);
    _lcd->fillRect(cx + 54, cy - 6, 3, 12, C_TEXT2);
    _lcd->fillRect(cx + 60, cy - 6, 3, 12, C_TEXT2);
}

void UI::handlePlayerTouch(uint16_t x, uint16_t y) {
    int cx = PAGE_X + PAGE_W / 2;
    int cy = PAGE_Y + 70 + 8 + 34 + 14 + 42; // 与 drawPlayerTab 对齐 (≈194)
    // 上一首
    if (hit(cx - 52 - 16, cy - 16, 32, 32, x, y)) {
        cur_song = (cur_song - 1 + playlist_count) % playlist_count;
        song_elapsed_ms = 0;
        render(); return;
    }
    // 下一首
    if (hit(cx + 52 - 16, cy - 16, 32, 32, x, y)) {
        cur_song = (cur_song + 1) % playlist_count;
        song_elapsed_ms = 0;
        render(); return;
    }
    // 播放/暂停
    if (hit(cx - 22, cy - 22, 44, 44, x, y)) {
        playing = !playing;
        render(); return;
    }
}

// ===== 歌单页 =====
void UI::drawPlaylistTab() {
    int x = PAGE_X, y = PAGE_Y;
    int w = PAGE_W, h = PAGE_H;

    setCJK();
    drawText(x + 8, y + 4, "我的歌单", C_TEXT, &g_font_cjk);
    _lcd->setFont(FONT_S);
    _lcd->setTextColor(C_TEXT3);
    _lcd->drawString("SD卡 · 12首", x + w - 70, y + 8);

    // 模式按钮（3 个）
    const char* modes[3] = {"顺序播放", "随机播放", "单曲循环"};
    int gap = 6;
    int total_gaps = gap * 2;
    int bw = (w - 16 - total_gaps) / 3;
    int by = y + 26;
    for (int i = 0; i < 3; i++) {
        int bx = x + 8 + i * (bw + gap);
        bool active = (play_mode == i);
        drawRoundedRect(bx, by, bw, 22, 11, active ? C_PRIMARY : C_SURFACE2);
        setCJK();
        drawCenteredText(bx, by, bw, 22, modes[i], active ? C_WHITE : C_TEXT2, &g_font_cjk);
    }

    // 列表
    int list_y = by + 28;
    int row_h = 34;
    int max_rows = (y + h - list_y) / row_h;
    if (max_rows > playlist_count) max_rows = playlist_count;
    for (int i = 0; i < max_rows; i++) {
        int row_y = list_y + i * row_h;
        bool active = (i == cur_song);
        if (active) {
            drawRoundedRect(x + 4, row_y, w - 8, row_h - 2, 6, rgb(30, 26, 58));
        }
        setCJK();
        if (active) {
            // 播放三角，垂直居中于行
            int tx = x + 13, ty = row_y + 11;
            _lcd->fillTriangle(tx, ty, tx, ty + 10, tx + 7, ty + 5, C_PRIMARY);
        } else {
            _lcd->setFont(FONT_S);
            _lcd->setTextColor(C_TEXT3);
            char numbuf[4];
            snprintf(numbuf, sizeof(numbuf), "%d", i + 1);
            _lcd->drawString(numbuf, x + 12, row_y + 11);
        }
        drawText(x + 30, row_y + 5, playlist[i].title, active ? C_PRIMARY : C_TEXT, &g_font_cjk);
        drawText(x + 30, row_y + 20, playlist[i].subtitle, C_TEXT3, &g_font_cjk);
        // 右侧网络图标
        _lcd->fillCircle(x + w - 18, row_y + 14, 5, C_ONLINE);
    }
}

void UI::handlePlaylistTouch(uint16_t x, uint16_t y) {
    int x0 = PAGE_X, y0 = PAGE_Y;
    int w = PAGE_W, h = PAGE_H;
    const char* modes[3] = {"顺序播放", "随机播放", "单曲循环"};
    int gap = 6;
    int total_gaps = gap * 2;
    int bw = (w - 16 - total_gaps) / 3;
    int by = y0 + 26;
    for (int i = 0; i < 3; i++) {
        int bx = x0 + 8 + i * (bw + gap);
        if (hit(bx, by, bw, 22, x, y)) { play_mode = (PlayMode)i; render(); return; }
    }
    int list_y = by + 28;
    int row_h = 34;
    int max_rows = (y0 + h - list_y) / row_h;
    if (max_rows > playlist_count) max_rows = playlist_count;
    for (int i = 0; i < max_rows; i++) {
        int row_y = list_y + i * row_h;
        if (hit(x0 + 4, row_y, w - 8, row_h - 2, x, y)) {
            cur_song = i;
            song_elapsed_ms = 0;
            render(); return;
        }
    }
}

// ===== 电台页 =====
void UI::drawRadioTab() {
    int x = PAGE_X, y = PAGE_Y;
    int w = PAGE_W, h = PAGE_H;

    setCJK();
    drawText(x + 8, y + 4, "电台", C_TEXT, &g_font_cjk);
    _lcd->setFont(FONT_S);
    _lcd->setTextColor(C_TEXT3);
    _lcd->drawString("8 频道在线", x + w - 66, y + 8);

    // 分类标签（用 FONT_S 避免溢出）
    int cat_y = y + 24;
    int cx = x + 8;
    for (int i = 0; i < radio_category_count; i++) {
        _lcd->setFont(FONT_S);
        int tw = _lcd->textWidth(radio_categories[i]) + 14;
        bool active = (i == cur_radio_cat);
        drawRoundedRect(cx, cat_y, tw, 18, 9, active ? C_PRIMARY : C_SURFACE2);
        _lcd->setTextColor(active ? C_WHITE : C_TEXT2);
        _lcd->setTextWrap(false);
        _lcd->setTextDatum(textdatum_t::middle_center);
        _lcd->drawString(radio_categories[i], cx + tw / 2, cat_y + 9);
        _lcd->setTextDatum(textdatum_t::top_left);
        cx += tw + 5;
        if (cx > x + w) break;  // 防止溢出屏幕
    }

    // 当前播放卡片
    int card_y = cat_y + 26;
    drawRoundedRect(x + 6, card_y, w - 12, 58, 10, C_SURFACE2);
    // 左侧渐变图标
    int ic_x = x + 14, ic_y = card_y + 7, ic_sz = 44;
    drawCoverGradient(_lcd, ic_x, ic_y, ic_sz, ic_sz, 6);
    // 收音机天线
    _lcd->drawLine(ic_x + 8, ic_y + ic_sz - 6, ic_x + ic_sz - 8, ic_y + 6, C_WHITE);
    _lcd->fillCircle(ic_x + ic_sz - 8, ic_y + 6, 3, C_WHITE);
    _lcd->fillRect(ic_x + 10, ic_y + ic_sz - 10, ic_sz - 20, 6, C_WHITE);

    setCJK();
    drawText(ic_x + ic_sz + 10, card_y + 10, "华语流行 FM", C_TEXT, &g_font_cjk);
    drawText(ic_x + ic_sz + 10, card_y + 26, "正在播放: 晴天", C_TEXT2, &g_font_cjk);
    _lcd->fillCircle(ic_x + ic_sz + 14, card_y + 44, 3, C_PRIMARY);
    drawText(ic_x + ic_sz + 22, card_y + 42, "直播中 128k", C_TEXT3, &g_font_cjk);

    // 右侧暂停按钮
    _lcd->fillCircle(x + w - 34, card_y + 29, 14, C_PRIMARY);
    drawCenteredText(x + w - 48, card_y + 15, 28, 28, "||", C_WHITE, FONT_M);

    // 电台列表
    int list_y = card_y + 64;
    const char* radios[2] = {"华语流行 FM", "古典音乐厅"};
    const char* descs[2] = {"热门金曲 24h 不停歇", "古典音乐不间断"};
    const char* freqs[2] = {"FM 88.7", "FM 92.1"};
    uint16_t colors[2] = {C_ACCENT, C_ONLINE};
    for (int i = 0; i < 2; i++) {
        int row_y = list_y + i * 36;
        int ico_x = x + 10, ico_y = row_y + 4, ico_sz = 28;
        _lcd->fillRoundRect(ico_x, ico_y, ico_sz, ico_sz, 4, colors[i]);
        if (i == 0) {
            // 麦克风
            _lcd->fillRect(ico_x + 10, ico_y + 4, 8, 12, C_WHITE);
            _lcd->fillCircle(ico_x + 14, ico_y + 18, 5, C_WHITE);
        } else {
            // 吉他/音符
            drawNoteIcon(_lcd, ico_x, ico_y, ico_sz, ico_sz, C_WHITE);
        }
        setCJK();
        drawText(ico_x + ico_sz + 8, row_y + 4, radios[i], C_TEXT, &g_font_cjk);
        drawText(ico_x + ico_sz + 8, row_y + 19, descs[i], C_TEXT3, &g_font_cjk);
        _lcd->setFont(FONT_S);
        _lcd->setTextColor(C_PRIMARY);
        _lcd->drawString(freqs[i], x + w - 50, row_y + 12);
    }
}

void UI::handleRadioTouch(uint16_t x, uint16_t y) {
    int x0 = PAGE_X, y0 = PAGE_Y;
    int w = PAGE_W;
    int cat_y = y0 + 24;
    int cx = x0 + 8;
    for (int i = 0; i < radio_category_count; i++) {
        int tw = textWidth(radio_categories[i], &g_font_cjk) + 14;
        if (hit(cx, cat_y, tw, 18, x, y)) { cur_radio_cat = i; render(); return; }
        cx += tw + 6;
    }
}

// ===== 搜索页 =====
void UI::drawSearchTab() {
    int x = PAGE_X, y = PAGE_Y;
    int w = PAGE_W, h = PAGE_H;

    // 搜索框
    int sb_h = 34;
    int sb_y = y + 6;
    drawRoundedRect(x + 8, sb_y, w - 16, sb_h, sb_h / 2, C_SURFACE2);
    _lcd->setFont(FONT_M);
    _lcd->setTextColor(C_TEXT3);
    _lcd->drawString("Q", x + 22, sb_y + 9);
    setCJK();
    drawText(x + 40, sb_y + 9, "搜索歌曲、歌手...", C_TEXT3, &g_font_cjk);
    // 语音按钮（原型右侧圆形，但搜索框内）
    int mic_r = 11;
    int mic_cx = x + w - 24, mic_cy = sb_y + sb_h / 2;
    _lcd->fillCircle(mic_cx, mic_cy, mic_r, C_PRIMARY);
    _lcd->drawFastVLine(mic_cx, mic_cy - 4, 7, C_WHITE);
    _lcd->fillRect(mic_cx - 3, mic_cy + 3, 6, 3, C_WHITE);

    // 搜索历史
    int hist_y = sb_y + sb_h + 12;
    setCJK();
    drawText(x + 8, hist_y, "搜索历史", C_TEXT3, &g_font_cjk);
    int hx = x + 10;
    for (int i = 0; i < search_history_count; i++) {
        int tw = textWidth(search_history[i], &g_font_cjk) + 16;
        drawRoundedRect(hx, hist_y + 14, tw, 20, 10, C_SURFACE2);
        setCJK();
        drawCenteredText(hx, hist_y + 14, tw, 20, search_history[i], C_TEXT2, &g_font_cjk);
        hx += tw + 6;
    }

    // 搜索结果
    int res_y = hist_y + 44;
    setCJK();
    drawText(x + 8, res_y, "搜索结果", C_TEXT3, &g_font_cjk);
    for (int i = 0; i < search_result_count; i++) {
        int row_y = res_y + 16 + i * 34;
        int ico_x = x + 10, ico_y = row_y + 3, ico_sz = 28;
        int ic = _icon_idx(search_results[i].title);
        _lcd->fillRoundRect(ico_x, ico_y, ico_sz, ico_sz, 4, ICON_COLORS[ic]);
        drawNoteIcon(_lcd, ico_x, ico_y, ico_sz, ico_sz, C_WHITE);
        setCJK();
        drawText(ico_x + ico_sz + 8, row_y + 4, search_results[i].title, C_TEXT, &g_font_cjk);
        drawText(ico_x + ico_sz + 8, row_y + 19, search_results[i].subtitle, C_TEXT3, &g_font_cjk);
    }

    // 滚动条
    _lcd->fillRoundRect(x + w - 10, res_y + 14, 4, 50, 2, C_DISABLED);
    _lcd->fillRoundRect(x + w - 10, res_y + 20, 4, 22, 2, C_TEXT2);
}

void UI::handleSearchTouch(uint16_t x, uint16_t y) {
    int x0 = PAGE_X, y0 = PAGE_Y;
    int w = PAGE_W;
    int sb_y = y0 + 6;
    int hist_y = sb_y + 34 + 12;
    int hx = x0 + 10;
    for (int i = 0; i < search_history_count; i++) {
        int tw = textWidth(search_history[i], &g_font_cjk) + 16;
        if (hit(hx, hist_y + 14, tw, 20, x, y)) {
            render(); return;
        }
        hx += tw + 6;
    }
    // 语音按钮
    int mic_cx = x0 + w - 24, mic_cy = sb_y + 17;
    if (hit(mic_cx - 11, mic_cy - 11, 22, 22, x, y)) { render(); return; }
}
