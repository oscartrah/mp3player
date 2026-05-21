#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "audio_hal.h"

#define NVS_KEY_VOLUME      1
#define NVS_KEY_SHUFFLE     2
#define NVS_KEY_LAST_TRACK  3
#define NVS_KEY_LAST_POS    4
#define NVS_KEY_EQ          5

typedef struct {
    uint8_t     volume;
    bool        shuffle;
    uint16_t    last_track;
    uint32_t    last_pos_sec;
    eq_config_t eq;
} app_settings_t;

#define SETTINGS_DEFAULT { \
    .volume       = 75,    \
    .shuffle      = false, \
    .last_track   = 0,     \
    .last_pos_sec = 0,     \
    .eq           = EQ_CONFIG_FLAT, \
}

int settings_init(void);
int settings_load(app_settings_t *out);
int settings_save(const app_settings_t *s);
int settings_save_volume(uint8_t vol);
int settings_save_shuffle(bool on);
int settings_save_position(uint16_t track, uint32_t pos_sec);
int settings_save_eq(const eq_config_t *eq);
