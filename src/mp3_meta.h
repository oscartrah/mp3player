#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "memory_cfg.h"

#define META_STR_MAX 64

typedef struct {
    char     title[META_STR_MAX];
    char     artist[META_STR_MAX];
    char     album[META_STR_MAX];
    uint8_t  track_nr;
    uint32_t total_frames;
    uint32_t sample_rate;
    uint32_t duration_sec;
    uint32_t audio_bytes;
    bool     is_vbr;
    uint8_t  toc[100];
    bool     has_toc;
    uint32_t audio_offset;
    uint8_t  *art_data;
    uint32_t  art_size;
    char      art_mime[16];
    bool      has_art;
} mp3_meta_t;

extern uint8_t art_buf[ART_BUF_SIZE] MEM_SECTION;

int mp3_meta_read(const char *path, mp3_meta_t *meta);
