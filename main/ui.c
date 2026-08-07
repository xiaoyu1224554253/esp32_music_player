/*
 * 音乐播放器 UI (LVGL 9.5) - 复刻 music_player_ui_prototype.html
 * 灵镜 AI 音响 · 320x240 深色主题
 */
#include "ui.h"
#include "lvgl.h"
#include <string.h>

/* ===== 设计令牌 (来自原型 CSS) ===== */
#define C_BG         lv_color_hex(0x0d0d15)
#define C_SURFACE    lv_color_hex(0x151520)
#define C_BORDER     lv_color_hex(0x2a2a3e)
#define C_TEXT       lv_color_hex(0xffffff)
#define C_TEXT2      lv_color_hex(0xaaaaaa)
#define C_TEXTM      lv_color_hex(0x666666)
#define C_PRIMARY    lv_color_hex(0x7c6cff)
#define C_ACCENT     lv_color_hex(0xa855f7)

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

/* 字体 (全中文字库, 在 ui_fonts.c 中定义) */
LV_FONT_DECLARE(font_cn_16);
LV_FONT_DECLARE(font_cn_22);

/* ===== 控件引用 ===== */
static lv_obj_t *lbl_song, *lbl_artist, *lbl_album, *lbl_time_end;
static lv_obj_t *lbl_lyric;
static lv_obj_t *play_btn;
static lv_obj_t *prog_fill, *prog_dot;
static lv_obj_t *lbl_playlist_items[6];
static lv_obj_t *lbl_source_badge;

/* ===== 播放页 ===== */
static void build_player(lv_obj_t *parent)
{
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(parent, 6, 0);

    /* 顶部: 封面 + 信息 */
    lv_obj_t *top = lv_obj_create(parent);
    lv_obj_set_size(top, LV_PCT(100), 76);
    lv_obj_set_flex_flow(top, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_grow(top, 0);
    lv_obj_set_style_bg_opa(top, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(top, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(top, 0, 0);
    lv_obj_set_style_pad_column(top, 8, 0);

    lv_obj_t *cover = lv_obj_create(top);
    lv_obj_set_size(cover, 70, 70);
    lv_obj_set_style_radius(cover, 8, 0);
    lv_obj_set_style_bg_grad_dir(cover, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_grad_color(cover, C_ACCENT, 0);
    lv_obj_set_style_bg_color(cover, C_PRIMARY, 0);
    lv_obj_set_style_shadow_color(cover, C_PRIMARY, 0);
    lv_obj_set_style_shadow_opa(cover, 60, 0);
    lv_obj_set_style_shadow_width(cover, 12, 0);
    lv_obj_t *cover_sym = lv_label_create(cover);
    lv_label_set_text(cover_sym, LV_SYMBOL_AUDIO);
    lv_obj_set_style_text_font(cover_sym, &font_cn_22, 0);
    lv_obj_center(cover_sym);

    lv_obj_t *info = lv_obj_create(top);
    lv_obj_set_flex_grow(info, 1);
    lv_obj_set_style_bg_opa(info, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(info, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(info, 0, 0);
    lv_obj_set_flex_flow(info, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_top(info, 4, 0);

    lbl_song = lv_label_create(info);
    lv_label_set_text(lbl_song, playlist[cur_song].name);
    lv_obj_set_style_text_font(lbl_song, &font_cn_16, 0);
    lv_obj_set_style_text_color(lbl_song, C_TEXT, 0);

    lbl_artist = lv_label_create(info);
    lv_label_set_text(lbl_artist, playlist[cur_song].artist);
    lv_obj_set_style_text_font(lbl_artist, &font_cn_16, 0);
    lv_obj_set_style_text_color(lbl_artist, C_TEXT2, 0);

    lbl_album = lv_label_create(info);
    lv_label_set_text(lbl_album, playlist[cur_song].album);
    lv_obj_set_style_text_font(lbl_album, &font_cn_16, 0);
    lv_obj_set_style_text_color(lbl_album, C_TEXTM, 0);

    lbl_source_badge = lv_label_create(info);
    lv_label_set_text(lbl_source_badge, "🌐 网络搜索");
    lv_obj_set_style_text_font(lbl_source_badge, &font_cn_16, 0);
    lv_obj_set_style_text_color(lbl_source_badge, C_PRIMARY, 0);
    lv_obj_set_style_bg_color(lbl_source_badge, lv_color_hex(0x1f1b3a), 0);
    lv_obj_set_style_bg_opa(lbl_source_badge, 255, 0);
    lv_obj_set_style_pad_hor(lbl_source_badge, 6, 0);
    lv_obj_set_style_pad_ver(lbl_source_badge, 2, 0);
    lv_obj_set_style_radius(lbl_source_badge, 10, 0);
    lv_obj_set_width(lbl_source_badge, LV_SIZE_CONTENT);

    /* 歌词区 */
    lv_obj_t *lybox = lv_obj_create(parent);
    lv_obj_set_size(lybox, LV_PCT(100), 34);
    lv_obj_set_style_bg_color(lybox, lv_color_hex(0x1a1a28), 0);
    lv_obj_set_style_bg_opa(lybox, 255, 0);
    lv_obj_set_style_radius(lybox, 8, 0);
    lv_obj_set_style_border_opa(lybox, LV_OPA_TRANSP, 0);
    lv_obj_center(lybox);
    lbl_lyric = lv_label_create(lybox);
    lv_label_set_text(lbl_lyric, lyrics[lyric_idx]);
    lv_obj_set_style_text_font(lbl_lyric, &font_cn_16, 0);
    lv_obj_set_style_text_color(lbl_lyric, C_TEXT, 0);
    lv_obj_set_style_text_align(lbl_lyric, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(lbl_lyric);

    /* 进度条 */
    lv_obj_t *prog = lv_obj_create(parent);
    lv_obj_set_size(prog, LV_PCT(100), 18);
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
    lv_obj_t *t1 = lv_label_create(timerow);
    lv_label_set_text(t1, "01:24");
    lv_obj_set_style_text_font(t1, &font_cn_16, 0);
    lv_obj_set_style_text_color(t1, C_TEXTM, 0);
    lbl_time_end = lv_label_create(timerow);
    lv_label_set_text(lbl_time_end, playlist[cur_song].time);
    lv_obj_set_flex_grow(lbl_time_end, 1);
    lv_obj_set_style_text_align(lbl_time_end, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_text_font(lbl_time_end, &font_cn_16, 0);
    lv_obj_set_style_text_color(lbl_time_end, C_TEXTM, 0);

    lv_obj_t *bar = lv_bar_create(prog);
    lv_obj_set_size(bar, LV_PCT(100), 5);
    lv_bar_set_value(bar, 35, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar, C_BORDER, 0);
    lv_obj_set_style_radius(bar, 3, 0);
    lv_obj_set_style_bg_color(bar, C_PRIMARY, LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar, 3, LV_PART_INDICATOR);

    /* 控制按钮 */
    lv_obj_t *ctrls = lv_obj_create(parent);
    lv_obj_set_size(ctrls, LV_PCT(100), 38);
    lv_obj_set_style_bg_opa(ctrls, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(ctrls, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(ctrls, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(ctrls, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(ctrls, 0, 0);
    lv_obj_set_style_gap(ctrls, 16, 0);

    lv_obj_t *prev = lv_btn_create(ctrls);
    lv_obj_set_size(prev, 34, 34);
    lv_obj_t *prev_l = lv_label_create(prev);
    lv_label_set_text(prev_l, LV_SYMBOL_PREV);
    lv_obj_set_style_text_font(prev_l, &font_cn_22, 0);
    lv_obj_set_style_radius(prev, 17, 0);
    lv_obj_set_style_bg_color(prev, lv_color_hex(0x1a1a28), 0);

    play_btn = lv_btn_create(ctrls);
    lv_obj_set_size(play_btn, 38, 38);
    lv_obj_t *play_l = lv_label_create(play_btn);
    lv_label_set_text(play_l, LV_SYMBOL_PLAY);
    lv_obj_set_style_text_font(play_l, &font_cn_22, 0);
    lv_obj_set_style_radius(play_btn, 19, 0);
    lv_obj_set_style_bg_grad_dir(play_btn, LV_GRAD_DIR_MAIN, 0);
    lv_obj_set_style_bg_grad_color(play_btn, C_ACCENT, 0);
    lv_obj_set_style_bg_color(play_btn, C_PRIMARY, 0);

    lv_obj_t *next = lv_btn_create(ctrls);
    lv_obj_set_size(next, 34, 34);
    lv_obj_t *next_l = lv_label_create(next);
    lv_label_set_text(next_l, LV_SYMBOL_NEXT);
    lv_obj_set_style_text_font(next_l, &font_cn_22, 0);
    lv_obj_set_style_radius(next, 17, 0);
    lv_obj_set_style_bg_color(next, lv_color_hex(0x1a1a28), 0);
}

/* ===== 歌单页 ===== */
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
    lv_obj_t *h = lv_label_create(hdr);
    lv_label_set_text(h, "我的歌单");
    lv_obj_set_style_text_font(h, &font_cn_16, 0);
    lv_obj_set_style_text_color(h, lv_color_hex(0xe0e0ff), 0);
    lv_obj_t *c = lv_label_create(hdr);
    lv_obj_set_flex_grow(c, 1);
    lv_label_set_text(c, "SD 卡 · 12 首");
    lv_obj_set_style_text_align(c, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_text_font(c, &font_cn_16, 0);
    lv_obj_set_style_text_color(c, C_TEXTM, 0);

    lv_obj_t *list = lv_list_create(parent);
    lv_obj_set_flex_grow(list, 1);
    lv_obj_set_size(list, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(list, 0, 0);

    for (int i = 0; i < N_SONGS; i++) {
        /* LVGL 9: lv_list_add_button 返回 lv_button, label 是其首个 child */
        lv_obj_t *item = lv_list_add_button(list, NULL, playlist[i].name);
        lbl_playlist_items[i] = lv_obj_get_child(item, 0);
        lv_obj_set_style_text_font(lbl_playlist_items[i], &font_cn_16, 0);
        lv_obj_set_style_bg_opa(item, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_color(item, C_BORDER, 0);
        lv_obj_set_style_border_width(item, 1, 0);
        lv_obj_set_style_border_opa(item, 40, 0);
    }
}

/* ===== 电台页 ===== */
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
    lv_obj_t *h = lv_label_create(hdr);
    lv_label_set_text(h, "电台");
    lv_obj_set_style_text_font(h, &font_cn_16, 0);
    lv_obj_set_style_text_color(h, lv_color_hex(0xe0e0ff), 0);
    lv_obj_t *c = lv_label_create(hdr);
    lv_obj_set_flex_grow(c, 1);
    lv_label_set_text(c, "8 个频道在线");
    lv_obj_set_style_text_align(c, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_text_font(c, &font_cn_16, 0);
    lv_obj_set_style_text_color(c, C_TEXTM, 0);

    const char *stations[8] = {
        "华语流行 FM", "古典音乐厅", "爵士咖啡馆", "电音浪潮",
        "民谣时光", "新闻资讯台", "ACG 动漫电台", "深夜故事会",
    };
    lv_obj_t *list = lv_list_create(parent);
    lv_obj_set_flex_grow(list, 1);
    lv_obj_set_size(list, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(list, 0, 0);
    for (int i = 0; i < 8; i++) {
        lv_obj_t *item = lv_list_add_button(list, LV_SYMBOL_AUDIO, stations[i]);
        lv_obj_t *lbl = lv_obj_get_child(item, 0);
        lv_obj_set_style_text_font(lbl, &font_cn_16, 0);
        lv_obj_set_style_bg_opa(item, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_color(item, C_BORDER, 0);
        lv_obj_set_style_border_width(item, 1, 0);
        lv_obj_set_style_border_opa(item, 40, 0);
    }
}

/* ===== 搜索页 ===== */
static void build_search(lv_obj_t *parent)
{
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(parent, 6, 0);

    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "搜索");
    lv_obj_set_style_text_font(title, &font_cn_16, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xe0e0ff), 0);

    lv_obj_t *box = lv_obj_create(parent);
    lv_obj_set_size(box, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(box, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(box, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(box, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_all(box, 0, 0);
    lv_obj_set_style_gap(box, 6, 0);

    lv_obj_t *input = lv_textarea_create(box);
    lv_obj_set_flex_grow(input, 1);
    lv_obj_set_height(input, 30);
    lv_textarea_set_placeholder_text(input, "搜索歌曲、歌手、专辑...");
    lv_obj_set_style_text_font(input, &font_cn_16, 0);
    lv_obj_set_style_bg_color(input, lv_color_hex(0x1a1a28), 0);
    lv_obj_set_style_radius(input, 15, 0);

    lv_obj_t *voice = lv_btn_create(box);
    lv_obj_set_size(voice, 30, 30);
    lv_obj_t *v = lv_label_create(voice);
    lv_label_set_text(v, LV_SYMBOL_VOLUME_MAX);
    lv_obj_set_style_radius(voice, 15, 0);
    lv_obj_set_style_bg_color(voice, lv_color_hex(0x241f44), 0);

    lv_obj_t *hist_title = lv_label_create(parent);
    lv_label_set_text(hist_title, "搜索历史");
    lv_obj_set_style_text_font(hist_title, &font_cn_16, 0);
    lv_obj_set_style_text_color(hist_title, C_TEXTM, 0);

    lv_obj_t *tags = lv_obj_create(parent);
    lv_obj_set_size(tags, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(tags, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(tags, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(tags, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_all(tags, 0, 0);
    lv_obj_set_style_gap(tags, 5, 0);
    const char *hist[] = { "周杰伦", "林俊杰", "陈奕迅", "夜曲" };
    for (int i = 0; i < 4; i++) {
        lv_obj_t *t = lv_btn_create(tags);
        lv_obj_set_height(t, 24);
        lv_obj_t *tl = lv_label_create(t);
        lv_label_set_text(tl, hist[i]);
        lv_obj_set_style_text_font(tl, &font_cn_16, 0);
        lv_obj_set_style_text_color(tl, C_TEXT2, 0);
        lv_obj_set_style_bg_color(t, lv_color_hex(0x1a1a28), 0);
        lv_obj_set_style_radius(t, 10, 0);
    }
}

/* ===== 导航栏 ===== */
static void nav_handler(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *tv = (lv_obj_t *)lv_event_get_user_data(e);
    int tab = (int)(intptr_t)lv_obj_get_user_data(btn);
    lv_tabview_set_act(tv, tab, LV_ANIM_ON);
}

void ui_init(void)
{
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, C_BG, 0);

    /* Tabview: 去掉默认标签栏, 用自定义底部导航 (LVGL 9: 单参数) */
    lv_obj_t *tv = lv_tabview_create(scr);
    lv_obj_set_size(tv, LV_PCT(100), LV_PCT(100));
    lv_obj_t *tabs = lv_tabview_get_content(tv);
    lv_obj_set_style_bg_color(tv, C_BG, 0);

    lv_obj_t *t_player   = lv_tabview_add_tab(tv, "player");
    lv_obj_t *t_playlist = lv_tabview_add_tab(tv, "playlist");
    lv_obj_t *t_radio    = lv_tabview_add_tab(tv, "radio");
    lv_obj_t *t_search   = lv_tabview_add_tab(tv, "search");

    build_player(t_player);
    build_playlist(t_playlist);
    build_radio(t_radio);
    build_search(t_search);

    /* 底部导航栏 */
    lv_obj_t *nav = lv_obj_create(scr);
    lv_obj_set_size(nav, LV_PCT(100), 30);
    lv_obj_align(nav, LV_ALIGN_BOTTOM_MID, 0, 0);
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
        lv_obj_set_style_bg_opa(item, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_opa(item, LV_OPA_TRANSP, 0);
        lv_obj_set_flex_flow(item, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(item, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_gap(item, 4, 0);
        lv_obj_set_user_data(item, (void *)(intptr_t)i);
        lv_obj_t *ic = lv_label_create(item);
        lv_label_set_text(ic, nav_icons[i]);
        lv_obj_t *tx = lv_label_create(item);
        lv_label_set_text(tx, nav_text[i]);
        lv_obj_set_style_text_font(tx, &font_cn_16, 0);
        lv_obj_add_event_cb(item, nav_handler, LV_EVENT_CLICKED, tv);
    }
}
