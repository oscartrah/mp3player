#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "mp3_meta.h"
#include "memory_cfg.h"

typedef struct {
    char       path[PLAYLIST_PATH_MAX];
    char       title[META_STR_MAX];
    char       artist[META_STR_MAX];
    uint32_t   duration_sec;
    uint8_t    track_nr;
} playlist_entry_t;

typedef struct {
    playlist_entry_t entries[PLAYLIST_MAX_TRACKS];
    uint16_t         count;
    int16_t          current;
    bool             shuffle;
    uint16_t         shuffle_order[PLAYLIST_MAX_TRACKS];
} playlist_t;

extern playlist_t g_playlist;

int     playlist_scan   (const char *root, playlist_t *pl);
void    playlist_sort   (playlist_t *pl);
int16_t playlist_next   (const playlist_t *pl);
int16_t playlist_prev   (const playlist_t *pl);

void playlist_shuffle_enable (playlist_t *pl);
void playlist_shuffle_disable(playlist_t *pl);
void playlist_toggle_shuffle (playlist_t *pl);
