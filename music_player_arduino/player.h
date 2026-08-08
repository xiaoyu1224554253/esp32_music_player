#pragma once

#include <Arduino.h>

enum PlayMode {
    PLAY_MODE_ORDER,
    PLAY_MODE_RANDOM,
    PLAY_MODE_REPEAT
};

struct Song {
    const char* title;
    const char* subtitle;
    const char* source;
};

extern const Song playlist[];
extern const int playlist_count;
extern int cur_song;
extern bool playing;
extern PlayMode play_mode;
extern unsigned long song_elapsed_ms;
extern unsigned long song_duration_ms;

extern const char* lyric_lines[];
extern const int lyric_count;
extern int lyric_index;

extern const char* radio_categories[];
extern const int radio_category_count;
extern int cur_radio_cat;

extern const char* search_history[];
extern const int search_history_count;

extern const Song search_results[];
extern const int search_result_count;
