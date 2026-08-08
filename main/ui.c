/*
 * 音乐播放器 UI (LVGL 9.5) - 1:1 复刻 music_player_ui_prototype.html
 * 灵镜 AI 音响 · 320x240 深色主题
 */
#include "ui.h"
#include "lvgl.h"
#include <string.h>
#include <stdio.h>

/* ===== 设计令牌 (来自原型 CSS) ===== */
#define C_BG         lv_color_hex(0x0d0d15)
#define C_SURFACE    lv_color_hex(0x151520)
#define C_SURFACE_L  lv_color_hex(0x1a1a28)
#define C_BORDER     lv_color_hex(0x2a2a3e)
#define C_TEXT       lv_color_hex(0xffffff)
#define C_TEXT2      lv_color_hex(0xaaaaaa)
#define C_TEXTM      lv_color_hex(0x666666)
#define C_TITLE      lv_color_hex(0xe0e0ff)
#define C_PRIMARY    lv_color_hex(0x7c6cff)
#define C_ACCENT     lv_color_hex(0xa855f7)
#define C_BADGE_BG   lv_color_hex(0x1f1b3a)
#define C_ST_EMPTY   lv_color_hex(0x1a1a28)

/* ===== 数据 ===== */
typedef struct { const char *name, *artist, *album, *time; } song_t;
static song_t playlist[] = {
    { "夜曲",   "周杰伦", "十一月的萧邦",     "03:46" },
    { "晴天",   "周杰伦", "叶惠美",           "04:29" },
    { "七里香", "周杰伦", "七里香",           "04:59" },
    { "稻香",   "周杰伦", "魔杰座",           "03:43" },
    { "告白气球", "周杰伦", "周杰伦的床边故事", "03:35" },
    { "青花瓷", "周杰伦", "我很忙",           "03:59" },
};
static const int N_SONGS = sizeof(playlist) / sizeof(playlist[0]);

static const char *lyrics[] = {
    "一群嗜血的蚂蚁 被腐肉所吸引", "我面无表情 看孤独的风景",
    "失去你 爱恨开始分明",         "失去你 还有什么事好关心",
    "当鸽子不再象征和平",         "我终于被提醒 广场上喂食的是秃鹰",
    "我用漂亮的押韵 形容被掠夺一空的爱情",
};
static int lyric_idx = 2;
static int cur_song = 0;
static bool playing = false;

/* 电台 */
typedef struct { const char *name, *desc, *freq, *icon, *cls; } station_t;
static station_t stations[] = {
    { "华语流行 FM", "最新华语金曲轮播",   "98.5", "🎵", "pop" },
    { "古典音乐厅", "贝多芬与莫扎特",     "101.2", "🎻", "classical" },
    { "爵士咖啡馆", "慵懒午夜爵士",       "89.7", "🎷", "jazz" },
    { "电音浪潮",   "EDM 潮流现场",       "102.6", "🎧", "electronic" },
    { "民谣时光",   "吉他与人声",         "95.3", "🪕", "folk" },
    { "新闻资讯台", "24h 时事速递",       "93.0", "📰", "news" },
    { "ACG 动漫电台", "二次元主题曲",     "107.1", "✨", "anime" },
    { "深夜故事会", "睡前陪伴电台",       "90.8", "🌙", "story" },
};
static const char *cat_chips[] = { "全部", "流行", "古典", "爵士", "电子", "民谣", "新闻", "动漫", "故事" };
static const int N_CATS = sizeof(cat_chips) / sizeof(cat_chips[0]);

/* 字体 (全中文字库, 在 ui_fonts.c 中定义; 回退时为指向内置默认字体的指针) */
extern const lv_font_t *font_cn_16;
extern const lv_font_t *font_cn_22;

/* ===== 控件引用 ===== */
static lv_obj_t *lbl_song, *lbl_artist, *lbl_album, *lbl_time_end;
static lv_obj_t *lbl_lyric;
static lv_obj_t *play_btn, *play_label;
static lv_obj_t *prog_dot;
static lv_obj_t *lbl_source_badge;
static lv_obj_t *nav_items[4];
static lv_obj_t *tabview;

static lv_obj_t *add_label(lv_obj_t *parent, const char *txt, const lv_font_t *font,
                           lv_color_t color, lv_text_align_t align)
{
    lv_obj_t *l = lv_label_create(parent);
    lv_label_set_text(l, txt);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, color, 0);
    if (align != LV_TEXT_ALIGN_LEFT) lv_obj_set_style_text_align(l, align, 0);
    return l;
}

/* 电台图标配色 (对应原型 .st-icon.<cls>) */
static lv_color_t station_color(const char *cls)
{
    if (!strcmp(cls, "pop"))        return lv_color_hex(0xee5a24);
    if (!strcmp(cls, "classical"))  return lv_color_hex(0x0984e3);
    if (!strcmp(cls, "jazz"))       return lv_color_hex(0xe17055);
    if (!strcmp(cls, "electronic")) return lv_color_hex(0x6c5ce7);
    if (!strcmp(cls, "folk"))       return lv_color_hex(0x00b894);
    if (!strcmp(cls, "news"))       return lv_color_hex(0xb2bec3);
    if (!strcmp(cls, "anime"))      return lv_color_hex(0xe84393);
    if (!strcmp(cls, "story"))      return lv_color_hex(0xfdcb6e);
    return C_PRIMARY;
}

/* ===== 全局状态栏 (原型 .status-bar: 12:08 / WiFi / 85%) ===== */
static void build_status_bar(lv_obj_t *parent)
{
    lv_obj_t *bar = lv_obj_create(parent);
    lv_obj_set_size(bar, LV_PCT(100), 18);
    lv_obj_set_style_bg_opa(bar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(bar, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bar, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_hor(bar, 8, 0);
    lv_obj_set_style_pad_all(bar, 0, 0);

    add_label(bar, "12:08", font_cn_16, C_TEXT2, LV_TEXT_ALIGN_LEFT);
    lv_obj_t *right = lv_obj_create(bar);
    lv_obj_set_flex_grow(right, 1);
    lv_obj_set_style_bg_opa(right, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(right, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(right, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(right, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(right, 0, 0);
    lv_obj_set_style_pad_column(right, 8, 0);
    add_label(right, "WiFi", font_cn_16, C_TEXT2, LV_TEXT_ALIGN_LEFT);
    add_label(right, "85%", font_cn_16, C_TEXT2, LV_TEXT_ALIGN_LEFT);
}

/* ===== 播放页 ===== */
static void build_player(lv_obj_t *parent)
{
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(parent, 6, 0);

    /* 顶部: 封面 + 信息 */
    lv_obj_t *top = lv_obj_create(parent);
    lv_obj_set_size(top, LV_PCT(100), 70);
    lv_obj_set_flex_flow(top, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_grow(top, 0);
    lv_obj_set_style_bg_opa(top, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(top, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(top, 0, 0);
    lv_obj_set_style_pad_column(top, 8, 0);

    lv_obj_t *cover = lv_obj_create(top);
    lv_obj_set_size(cover, 64, 64);
    lv_obj_set_style_radius(cover, 8, 0);
    lv_obj_set_style_bg_grad_dir(cover, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_grad_color(cover, C_ACCENT, 0);
    lv_obj_set_style_bg_color(cover, C_PRIMARY, 0);
    lv_obj_set_style_shadow_color(cover, C_PRIMARY, 0);
    lv_obj_set_style_shadow_opa(cover, 60, 0);
    lv_obj_set_style_shadow_width(cover, 12, 0);
    lv_obj_t *cover_sym = lv_label_create(cover);
    lv_label_set_text(cover_sym, LV_SYMBOL_AUDIO);
    lv_obj_set_style_text_font(cover_sym, font_cn_22, 0);
    lv_obj_center(cover_sym);

    lv_obj_t *info = lv_obj_create(top);
    lv_obj_set_flex_grow(info, 1);
    lv_obj_set_style_bg_opa(info, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(info, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(info, 0, 0);
    lv_obj_set_flex_flow(info, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_top(info, 2, 0);

    lbl_song = add_label(info, playlist[cur_song].name, font_cn_16, C_TEXT, LV_TEXT_ALIGN_LEFT);
    lbl_artist = add_label(info, playlist[cur_song].artist, font_cn_16, C_TEXT2, LV_TEXT_ALIGN_LEFT);
    lbl_album = add_label(info, playlist[cur_song].album, font_cn_16, C_TEXTM, LV_TEXT_ALIGN_LEFT);

    lbl_source_badge = lv_label_create(info);
    lv_label_set_text(lbl_source_badge, "🌐 网络搜索");
    lv_obj_set_style_text_font(lbl_source_badge, font_cn_16, 0);
    lv_obj_set_style_text_color(lbl_source_badge, C_PRIMARY, 0);
    lv_obj_set_style_bg_color(lbl_source_badge, C_BADGE_BG, 0);
    lv_obj_set_style_bg_opa(lbl_source_badge, 255, 0);
    lv_obj_set_style_pad_hor(lbl_source_badge, 6, 0);
    lv_obj_set_style_pad_ver(lbl_source_badge, 2, 0);
    lv_obj_set_style_radius(lbl_source_badge, 10, 0);
    lv_obj_set_width(lbl_source_badge, LV_SIZE_CONTENT);
    lv_obj_set_style_margin_top(lbl_source_badge, 2, 0);

    /* 歌词区 */
    lv_obj_t *lybox = lv_obj_create(parent);
    lv_obj_set_size(lybox, LV_PCT(100), 30);
    lv_obj_set_style_bg_color(lybox, C_SURFACE_L, 0);
    lv_obj_set_style_bg_opa(lybox, 255, 0);
    lv_obj_set_style_radius(lybox, 8, 0);
    lv_obj_set_style_border_opa(lybox, LV_OPA_TRANSP, 0);
    lv_obj_center(lybox);
    lbl_lyric = add_label(lybox, lyrics[lyric_idx], font_cn_16, C_TEXT, LV_TEXT_ALIGN_CENTER);
    lv_obj_center(lbl_lyric);

    /* 进度条 (含圆点指示器) */
    lv_obj_t *prog = lv_obj_create(parent);
    lv_obj_set_size(prog, LV_PCT(100), 24);
    lv_obj_set_style_bg_opa(prog, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(prog, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(prog, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(prog, 0, 0);

    lv_obj_t *timerow = lv_obj_create(prog);
    lv_obj_set_size(timerow, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(timerow, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(timerow, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(timerow, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_all(timerow, 0, 0);
    add_label(timerow, "01:24", font_cn_16, C_TEXTM, LV_TEXT_ALIGN_LEFT);
    lbl_time_end = add_label(timerow, playlist[cur_song].time, font_cn_16, C_TEXTM, LV_TEXT_ALIGN_RIGHT);
    lv_obj_set_flex_grow(lbl_time_end, 1);

    lv_obj_t *bar = lv_bar_create(prog);
    lv_obj_set_size(bar, LV_PCT(100), 5);
    lv_bar_set_value(bar, 35, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar, C_BORDER, 0);
    lv_obj_set_style_radius(bar, 3, 0);
    lv_obj_set_style_bg_color(bar, C_PRIMARY, LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar, 3, LV_PART_INDICATOR);
    /* 进度圆点 (原型 .progress-dot) */
    prog_dot = lv_obj_create(prog);
    lv_obj_set_size(prog_dot, 11, 11);
    lv_obj_set_style_bg_color(prog_dot, C_TEXT, 0);
    lv_obj_set_style_radius(prog_dot, 5, 0);
    lv_obj_set_style_shadow_color(prog_dot, lv_color_black(), 0);
    lv_obj_set_style_shadow_opa(prog_dot, 128, 0);
    lv_obj_set_style_shadow_width(prog_dot, 6, 0);
    lv_obj_set_style_border_opa(prog_dot, LV_OPA_TRANSP, 0);
    /* 35% 位置 (bar 在 timerow 下方) */
    lv_obj_align_to(prog_dot, bar, LV_ALIGN_LEFT_MID, (int)(320 * 0.35) - 5, 0);

    /* 控制按钮 */
    lv_obj_t *ctrls = lv_obj_create(parent);
    lv_obj_set_size(ctrls, LV_PCT(100), 38);
    lv_obj_set_style_bg_opa(ctrls, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(ctrls, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(ctrls, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(ctrls, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(ctrls, 0, 0);
    lv_obj_set_style_pad_column(ctrls, 16, 0);

    lv_obj_t *prev = lv_btn_create(ctrls);
    lv_obj_set_size(prev, 34, 34);
    lv_obj_t *prev_l = lv_label_create(prev);
    lv_label_set_text(prev_l, LV_SYMBOL_PREV);
    lv_obj_set_style_text_font(prev_l, font_cn_22, 0);
    lv_obj_set_style_radius(prev, 17, 0);
    lv_obj_set_style_bg_color(prev, C_SURFACE_L, 0);

    play_btn = lv_btn_create(ctrls);
    lv_obj_set_size(play_btn, 38, 38);
    play_label = lv_label_create(play_btn);
    lv_label_set_text(play_label, LV_SYMBOL_PLAY);
    lv_obj_set_style_text_font(play_label, font_cn_22, 0);
    lv_obj_set_style_radius(play_btn, 19, 0);
    lv_obj_set_style_bg_grad_dir(play_btn, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_grad_color(play_btn, C_ACCENT, 0);
    lv_obj_set_style_bg_color(play_btn, C_PRIMARY, 0);

    lv_obj_t *next = lv_btn_create(ctrls);
    lv_obj_set_size(next, 34, 34);
    lv_obj_t *next_l = lv_label_create(next);
    lv_label_set_text(next_l, LV_SYMBOL_NEXT);
    lv_obj_set_style_text_font(next_l, font_cn_22, 0);
    lv_obj_set_style_radius(next, 17, 0);
    lv_obj_set_style_bg_color(next, C_SURFACE_L, 0);
}

/* ===== 歌单页 (含播放模式按钮 + 自定义行) ===== */
static void build_playlist(lv_obj_t *parent)
{
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(parent, 6, 0);

    lv_obj_t *hdr = lv_obj_create(parent);
    lv_obj_set_size(hdr, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(hdr, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(hdr, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(hdr, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_all(hdr, 0, 0);
    add_label(hdr, "我的歌单", font_cn_16, C_TITLE, LV_TEXT_ALIGN_LEFT);
    lv_obj_t *c = add_label(hdr, "SD 卡 · 12 首", font_cn_16, C_TEXTM, LV_TEXT_ALIGN_RIGHT);
    lv_obj_set_flex_grow(c, 1);

    /* 播放模式按钮 (顺序/随机/单曲) */
    lv_obj_t *modes = lv_obj_create(parent);
    lv_obj_set_size(modes, LV_PCT(100), 26);
    lv_obj_set_style_bg_opa(modes, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(modes, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(modes, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_all(modes, 0, 0);
    lv_obj_set_style_pad_column(modes, 6, 0);
    const char *mode_txt[3] = { "🔄 顺序播放", "🔀 随机播放", "🔁 单曲循环" };
    for (int i = 0; i < 3; i++) {
        lv_obj_t *m = lv_btn_create(modes);
        lv_obj_set_flex_grow(m, 1);
        lv_obj_set_height(m, 24);
        lv_obj_t *ml = lv_label_create(m);
        lv_label_set_text(ml, mode_txt[i]);
        lv_obj_set_style_text_font(ml, font_cn_16, 0);
        lv_obj_set_style_text_color(ml, (i == 0) ? C_PRIMARY : C_TEXT2, 0);
        lv_obj_set_style_bg_color(m, (i == 0) ? lv_color_hex(0x241f44) : C_SURFACE_L, 0);
        lv_obj_set_style_border_color(m, (i == 0) ? lv_color_hex(0x4a3f8a) : C_BORDER, 0);
        lv_obj_set_style_radius(m, 12, 0);
    }

    /* 歌曲列表: 序号 + 信息(标题/副标题) + 图标 */
    lv_obj_t *list = lv_obj_create(parent);
    lv_obj_set_flex_grow(list, 1);
    lv_obj_set_size(list, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(list, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(list, 0, 0);

    char num[8];
    for (int i = 0; i < N_SONGS; i++) {
        lv_obj_t *row = lv_obj_create(list);
        lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_color(row, C_BORDER, 0);
        lv_obj_set_style_border_width(row, 1, 0);
        lv_obj_set_style_border_opa(row, 10, 0);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_ver(row, 5, 0);
        lv_obj_set_style_pad_hor(row, 2, 0);

        snprintf(num, sizeof(num), "%d", i + 1);
        lv_obj_t *n = add_label(row, num, font_cn_16, C_TEXTM, LV_TEXT_ALIGN_CENTER);
        lv_obj_set_width(n, 18);

        lv_obj_t *info = lv_obj_create(row);
        lv_obj_set_flex_grow(info, 1);
        lv_obj_set_style_bg_opa(info, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_opa(info, LV_OPA_TRANSP, 0);
        lv_obj_set_flex_flow(info, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_all(info, 0, 0);
        lv_obj_set_style_pad_left(info, 4, 0);
        add_label(info, playlist[i].name, font_cn_16, C_TEXT, LV_TEXT_ALIGN_LEFT);
        char sub[64];
        snprintf(sub, sizeof(sub), "%s · %s", playlist[i].artist, playlist[i].album);
        add_label(info, sub, font_cn_16, C_TEXTM, LV_TEXT_ALIGN_LEFT);

        add_label(row, "🌐", font_cn_16, C_PRIMARY, LV_TEXT_ALIGN_LEFT);
    }
}

/* ===== 电台页 (分类标签 + 当前播放卡片 + 列表) ===== */
static void build_radio(lv_obj_t *parent)
{
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(parent, 6, 0);

    lv_obj_t *hdr = lv_obj_create(parent);
    lv_obj_set_size(hdr, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(hdr, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(hdr, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(hdr, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_all(hdr, 0, 0);
    add_label(hdr, "电台", font_cn_16, C_TITLE, LV_TEXT_ALIGN_LEFT);
    lv_obj_t *c = add_label(hdr, "8 个频道在线", font_cn_16, C_TEXTM, LV_TEXT_ALIGN_RIGHT);
    lv_obj_set_flex_grow(c, 1);

    /* 分类标签行 */
    lv_obj_t *cats = lv_obj_create(parent);
    lv_obj_set_size(cats, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(cats, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(cats, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(cats, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_all(cats, 0, 0);
    lv_obj_set_style_pad_column(cats, 6, 0);
    for (int i = 0; i < N_CATS; i++) {
        lv_obj_t *chip = lv_btn_create(cats);
        lv_obj_set_height(chip, 24);
        lv_obj_set_width(chip, LV_SIZE_CONTENT);
        lv_obj_t *cl = lv_label_create(chip);
        lv_label_set_text(cl, cat_chips[i]);
        lv_obj_set_style_text_font(cl, font_cn_16, 0);
        bool act = (i == 0);
        lv_obj_set_style_text_color(cl, act ? C_PRIMARY : C_TEXT2, 0);
        lv_obj_set_style_bg_color(cl, act ? lv_color_hex(0x241f44) : C_SURFACE_L, 0);
        lv_obj_set_style_border_color(cl, act ? lv_color_hex(0x4a3f8a) : C_BORDER, 0);
        lv_obj_set_style_radius(chip, 12, 0);
        lv_obj_set_style_pad_hor(chip, 10, 0);
    }

    /* 当前播放卡片 */
    lv_obj_t *np = lv_obj_create(parent);
    lv_obj_set_size(np, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_grad_dir(np, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_grad_color(np, lv_color_hex(0x2a1f44), 0);
    lv_obj_set_style_bg_color(np, lv_color_hex(0x1f1740), 0);
    lv_obj_set_style_radius(np, 10, 0);
    lv_obj_set_style_border_color(np, lv_color_hex(0x4a3f8a), 0);
    lv_obj_set_style_border_opa(np, 255, 0);
    lv_obj_set_flex_flow(np, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(np, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(np, 7, 0);
    lv_obj_set_style_pad_column(np, 8, 0);

    lv_obj_t *cov = lv_obj_create(np);
    lv_obj_set_size(cov, 42, 42);
    lv_obj_set_style_radius(cov, 6, 0);
    lv_obj_set_style_bg_grad_dir(cov, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_grad_color(cov, lv_color_hex(0xf5576c), 0);
    lv_obj_set_style_bg_color(cov, lv_color_hex(0xf093fb), 0);
    lv_obj_t *cov_s = lv_label_create(cov);
    lv_label_set_text(cov_s, "📻");
    lv_obj_set_style_text_font(cov_s, font_cn_22, 0);
    lv_obj_center(cov_s);

    lv_obj_t *ninfo = lv_obj_create(np);
    lv_obj_set_flex_grow(ninfo, 1);
    lv_obj_set_style_bg_opa(ninfo, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(ninfo, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(ninfo, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(ninfo, 0, 0);
    add_label(ninfo, "华语流行 FM", font_cn_16, C_TEXT, LV_TEXT_ALIGN_LEFT);
    add_label(ninfo, "最新华语金曲轮播", font_cn_16, C_TEXT2, LV_TEXT_ALIGN_LEFT);
    lv_obj_t *meta = lv_obj_create(ninfo);
    lv_obj_set_style_bg_opa(meta, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(meta, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(meta, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_all(meta, 0, 0);
    lv_obj_set_style_pad_column(meta, 4, 0);
    lv_obj_t *dot = lv_obj_create(meta);
    lv_obj_set_size(dot, 6, 6);
    lv_obj_set_style_bg_color(dot, lv_color_hex(0xff4757), 0);
    lv_obj_set_style_radius(dot, 3, 0);
    lv_obj_set_style_border_opa(dot, LV_OPA_TRANSP, 0);
    add_label(meta, "直播中", font_cn_16, C_TEXTM, LV_TEXT_ALIGN_LEFT);

    lv_obj_t *npbtn = lv_btn_create(np);
    lv_obj_set_size(npbtn, 32, 32);
    lv_obj_t *npl = lv_label_create(npbtn);
    lv_label_set_text(npl, LV_SYMBOL_PAUSE);
    lv_obj_set_style_text_font(npl, font_cn_22, 0);
    lv_obj_set_style_radius(npbtn, 16, 0);
    lv_obj_set_style_bg_grad_dir(npbtn, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_grad_color(npbtn, C_ACCENT, 0);
    lv_obj_set_style_bg_color(npbtn, C_PRIMARY, 0);
    lv_obj_set_style_shadow_color(npbtn, C_PRIMARY, 0);
    lv_obj_set_style_shadow_opa(npbtn, 90, 0);
    lv_obj_set_style_shadow_width(npbtn, 10, 0);

    /* 电台列表 */
    lv_obj_t *list = lv_obj_create(parent);
    lv_obj_set_flex_grow(list, 1);
    lv_obj_set_size(list, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(list, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(list, 0, 0);
    for (int i = 0; i < 8; i++) {
        lv_obj_t *row = lv_obj_create(list);
        lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_color(row, C_BORDER, 0);
        lv_obj_set_style_border_width(row, 1, 0);
        lv_obj_set_style_border_opa(row, 10, 0);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_ver(row, 6, 0);
        lv_obj_set_style_pad_hor(row, 2, 0);

        lv_obj_t *ic = lv_obj_create(row);
        lv_obj_set_size(ic, 34, 34);
        lv_obj_set_style_radius(ic, 6, 0);
        lv_obj_set_style_bg_color(ic, station_color(stations[i].cls), 0);
        lv_obj_t *ics = lv_label_create(ic);
        lv_label_set_text(ics, stations[i].icon);
        lv_obj_set_style_text_font(ics, font_cn_22, 0);
        lv_obj_center(ics);

        lv_obj_t *info = lv_obj_create(row);
        lv_obj_set_flex_grow(info, 1);
        lv_obj_set_style_bg_opa(info, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_opa(info, LV_OPA_TRANSP, 0);
        lv_obj_set_flex_flow(info, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_all(info, 0, 0);
        lv_obj_set_style_pad_left(info, 6, 0);
        add_label(info, stations[i].name, font_cn_16, C_TEXT, LV_TEXT_ALIGN_LEFT);
        add_label(info, stations[i].desc, font_cn_16, C_TEXTM, LV_TEXT_ALIGN_LEFT);

        add_label(row, stations[i].freq, font_cn_16, C_PRIMARY, LV_TEXT_ALIGN_LEFT);
    }
}

/* ===== 搜索页 (搜索框 + 状态 + 结果 + 历史) ===== */
static void build_search(lv_obj_t *parent)
{
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(parent, 6, 0);

    lv_obj_t *title = add_label(parent, "搜索", font_cn_16, C_TITLE, LV_TEXT_ALIGN_LEFT);

    lv_obj_t *box = lv_obj_create(parent);
    lv_obj_set_size(box, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(box, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(box, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(box, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_all(box, 0, 0);
    lv_obj_set_style_pad_row(box, 6, 0);

    lv_obj_t *input = lv_textarea_create(box);
    lv_obj_set_flex_grow(input, 1);
    lv_obj_set_height(input, 30);
    lv_textarea_set_placeholder_text(input, "搜索歌曲、歌手、专辑...");
    lv_obj_set_style_text_font(input, font_cn_16, 0);
    lv_obj_set_style_bg_color(input, C_SURFACE_L, 0);
    lv_obj_set_style_radius(input, 15, 0);

    lv_obj_t *voice = lv_btn_create(box);
    lv_obj_set_size(voice, 30, 30);
    lv_obj_t *v = lv_label_create(voice);
    lv_label_set_text(v, LV_SYMBOL_VOLUME_MAX);
    lv_obj_set_style_text_font(v, font_cn_16, 0);
    lv_obj_set_style_radius(voice, 15, 0);
    lv_obj_set_style_bg_color(voice, lv_color_hex(0x241f44), 0);

    /* 搜索状态 (spinner) */
    lv_obj_t *status = lv_obj_create(parent);
    lv_obj_set_size(status, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(status, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(status, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(status, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(status, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(status, 4, 0);
    lv_obj_set_style_pad_column(status, 6, 0);
    lv_obj_t *spin = lv_spinner_create(status);
    lv_spinner_set_spin_time(spin, 1000);
    lv_obj_set_size(spin, 11, 11);
    lv_obj_set_style_arc_color(spin, C_PRIMARY, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(spin, 2, 0);
    lv_obj_set_style_arc_length(spin, 60, LV_PART_INDICATOR);
    add_label(status, "搜索中...", font_cn_16, C_PRIMARY, LV_TEXT_ALIGN_LEFT);

    /* 搜索结果区 */
    lv_obj_t *results = lv_obj_create(parent);
    lv_obj_set_flex_grow(results, 1);
    lv_obj_set_size(results, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(results, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(results, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(results, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(results, 0, 0);
    const char *res_name[3] = { "夜曲", "夜的第七章", "夜空中最亮的星" };
    const char *res_src[3] = { "网络搜索", "本地音乐", "网络搜索" };
    for (int i = 0; i < 3; i++) {
        lv_obj_t *r = lv_obj_create(results);
        lv_obj_set_size(r, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(r, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_color(r, C_BORDER, 0);
        lv_obj_set_style_border_width(r, 1, 0);
        lv_obj_set_style_border_opa(r, 10, 0);
        lv_obj_set_flex_flow(r, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(r, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_ver(r, 5, 0);
        lv_obj_set_style_pad_hor(r, 2, 0);

        lv_obj_t *th = lv_obj_create(r);
        lv_obj_set_size(th, 28, 28);
        lv_obj_set_style_radius(th, 4, 0);
        lv_obj_set_style_bg_grad_dir(th, LV_GRAD_DIR_VER, 0);
        lv_obj_set_style_bg_grad_color(th, lv_color_hex(0x764ba2), 0);
        lv_obj_set_style_bg_color(th, lv_color_hex(0x667eea), 0);

        lv_obj_t *rinfo = lv_obj_create(r);
        lv_obj_set_flex_grow(rinfo, 1);
        lv_obj_set_style_bg_opa(rinfo, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_opa(rinfo, LV_OPA_TRANSP, 0);
        lv_obj_set_flex_flow(rinfo, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_all(rinfo, 0, 0);
        lv_obj_set_style_pad_left(rinfo, 6, 0);
        add_label(rinfo, res_name[i], font_cn_16, C_TEXT, LV_TEXT_ALIGN_LEFT);
        add_label(rinfo, res_src[i], font_cn_16, C_TEXTM, LV_TEXT_ALIGN_LEFT);
    }

    /* 搜索历史 */
    lv_obj_t *hist_title = add_label(parent, "搜索历史", font_cn_16, C_TEXTM, LV_TEXT_ALIGN_LEFT);
    lv_obj_set_style_margin_top(hist_title, 4, 0);
    lv_obj_t *tags = lv_obj_create(parent);
    lv_obj_set_size(tags, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(tags, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(tags, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(tags, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_all(tags, 0, 0);
    lv_obj_set_style_pad_column(tags, 5, 0);
    lv_obj_set_style_pad_row(tags, 5, 0);
    const char *hist[] = { "周杰伦", "林俊杰", "陈奕迅", "夜曲" };
    for (int i = 0; i < 4; i++) {
        lv_obj_t *t = lv_btn_create(tags);
        lv_obj_set_height(t, 24);
        lv_obj_t *tl = lv_label_create(t);
        lv_label_set_text(tl, hist[i]);
        lv_obj_set_style_text_font(tl, font_cn_16, 0);
        lv_obj_set_style_text_color(tl, C_TEXT2, 0);
        lv_obj_set_style_bg_color(t, C_SURFACE_L, 0);
        lv_obj_set_style_radius(t, 10, 0);
        lv_obj_set_style_pad_hor(t, 8, 0);
    }
}

/* ===== 导航栏激活高亮 (原型 .nav-item.active) ===== */
static void update_nav_active(int act)
{
    for (int i = 0; i < 4; i++) {
        lv_obj_t *item = nav_items[i];
        bool on = (i == act);
        lv_obj_set_style_bg_color(item, on ? lv_color_hex(0x1f1b3a) : lv_color_hex(0x151520), 0);
        lv_obj_set_style_bg_opa(item, on ? 255 : 255, 0);
        lv_obj_t *ic = lv_obj_get_child(item, 0);
        lv_obj_t *tx = lv_obj_get_child(item, 1);
        lv_obj_set_style_text_color(ic, on ? C_PRIMARY : C_TEXT2, 0);
        lv_obj_set_style_text_color(tx, on ? C_PRIMARY : C_TEXT2, 0);
    }
}

static void tabview_changed(lv_event_t *e)
{
    lv_obj_t *tv = lv_event_get_target(e);
    int act = lv_tabview_get_tab_act(tv);
    update_nav_active(act);
}

static void nav_handler(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    int tab = (int)(intptr_t)lv_obj_get_user_data(btn);
    lv_tabview_set_act(tabview, tab, LV_ANIM_ON);
    update_nav_active(tab);
}

void ui_init(void)
{
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, C_BG, 0);
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(scr, 0, 0);

    /* 全局状态栏 */
    build_status_bar(scr);

    /* Tabview: 占满剩余高度, 用自定义底部导航 */
    tabview = lv_tabview_create(scr);
    lv_obj_set_flex_grow(tabview, 1);
    lv_obj_set_width(tabview, LV_PCT(100));
    lv_obj_t *tabs = lv_tabview_get_content(tabview);
    lv_obj_set_style_bg_color(tabview, C_BG, 0);

    lv_obj_t *t_player   = lv_tabview_add_tab(tabview, "player");
    lv_obj_t *t_playlist = lv_tabview_add_tab(tabview, "playlist");
    lv_obj_t *t_radio    = lv_tabview_add_tab(tabview, "radio");
    lv_obj_t *t_search   = lv_tabview_add_tab(tabview, "search");

    build_player(t_player);
    build_playlist(t_playlist);
    build_radio(t_radio);
    build_search(t_search);

    lv_obj_add_event_cb(tabview, tabview_changed, LV_EVENT_VALUE_CHANGED, NULL);

    /* 底部导航栏 */
    lv_obj_t *nav = lv_obj_create(scr);
    lv_obj_set_size(nav, LV_PCT(100), 30);
    lv_obj_set_style_bg_color(nav, C_SURFACE, 0);
    lv_obj_set_style_radius(nav, 0, 0);
    lv_obj_set_style_border_opa(nav, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(nav, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_all(nav, 0, 0);

    const char *nav_icons[4] = { LV_SYMBOL_AUDIO, LV_SYMBOL_LIST, LV_SYMBOL_WIFI, LV_SYMBOL_IMAGE };
    const char *nav_text[4] = { "播放", "歌单", "电台", "搜索" };
    for (int i = 0; i < 4; i++) {
        lv_obj_t *item = lv_obj_create(nav);
        lv_obj_set_flex_grow(item, 1);
        lv_obj_set_style_bg_color(item, C_SURFACE, 0);
        lv_obj_set_style_border_opa(item, LV_OPA_TRANSP, 0);
        lv_obj_set_flex_flow(item, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(item, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(item, 4, 0);
        lv_obj_set_user_data(item, (void *)(intptr_t)i);
        lv_obj_t *ic = lv_label_create(item);
        lv_label_set_text(ic, nav_icons[i]);
        lv_obj_t *tx = lv_label_create(item);
        lv_label_set_text(tx, nav_text[i]);
        lv_obj_set_style_text_font(tx, font_cn_16, 0);
        lv_obj_add_event_cb(item, nav_handler, LV_EVENT_CLICKED, NULL);
        nav_items[i] = item;
    }
    update_nav_active(0);
}
