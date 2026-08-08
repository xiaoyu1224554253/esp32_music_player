#include "player.h"

const Song playlist[] = {
    {"夜曲", "周杰伦 - 十一月的萧邦", "网络"},
    {"晴天", "周杰伦 - 叶惠美", "网络"},
    {"七里香", "周杰伦 - 七里香", "网络"},
    {"稻香", "周杰伦 - 魔杰座", "网络"},
    {"青花瓷", "周杰伦 - 我很忙", "SD卡"},
};
const int playlist_count = sizeof(playlist) / sizeof(playlist[0]);
int cur_song = 0;
bool playing = false;
PlayMode play_mode = PLAY_MODE_ORDER;
unsigned long song_elapsed_ms = 84 * 1000;  // 1:24
unsigned long song_duration_ms = 226 * 1000; // 3:46

const char* lyric_lines[] = {
    "一盏离愁 孤单伫立在窗口",
    "我在门后 假装你人还没走",
    "旧地如重游 月圆更寂寞",
    "夜半清醒的烛火 不忍苛责我",
    "一壶漂泊 浪迹天涯难入喉",
};
const int lyric_count = sizeof(lyric_lines) / sizeof(lyric_lines[0]);
int lyric_index = 2; // 当前高亮"旧地如重游"

const char* radio_categories[] = {"全部", "流行", "古典", "爵士", "电子", "民谣", "新闻"};
const int radio_category_count = sizeof(radio_categories) / sizeof(radio_categories[0]);
int cur_radio_cat = 0;

const char* search_history[] = {"周杰伦", "林俊杰", "陈奕迅", "夜曲"};
const int search_history_count = sizeof(search_history) / sizeof(search_history[0]);

const Song search_results[] = {
    {"夜曲 - 周杰伦", "网络 · 03:46", "网络"},
    {"晴天 - 周杰伦", "网络 · 04:29", "网络"},
};
const int search_result_count = sizeof(search_results) / sizeof(search_results[0]);
