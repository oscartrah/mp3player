#pragma once
#include <zephyr/kernel.h>
#include <stdint.h>
#include "memory_cfg.h"

#define PCM_BLOCK_SIZE  2304   /* MINIMP3_MAX_SAMPLES_PER_FRAME * 2ch */

struct pcm_block {
    int16_t  data[PCM_BLOCK_SIZE];
    uint16_t samples;
    uint8_t  channels;
    uint32_t sample_rate;
};

extern struct k_msgq pcm_queue;

void decoder_thread(void *path, void *p2, void *p3);
void player_thread (void *p1,   void *p2, void *p3);
