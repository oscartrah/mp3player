#pragma once
#include "audio_hal.h"

typedef enum {
    EQ_PRESET_IDX_FLAT = 0,
    EQ_PRESET_IDX_BASS_BOOST,
    EQ_PRESET_IDX_TREBLE_BOOST,
    EQ_PRESET_IDX_VOCAL,
    EQ_PRESET_IDX_ROCK,
    EQ_PRESET_IDX_JAZZ,
    EQ_PRESET_IDX_CUSTOM,
    EQ_PRESET_COUNT,
} eq_preset_idx_t;

typedef struct {
    const char  *name;
    eq_config_t  config;
} eq_preset_t;

extern const eq_preset_t  g_eq_presets[EQ_PRESET_COUNT];
extern eq_preset_idx_t    g_eq_preset_active;
extern eq_config_t        g_eq_current;

void eq_preset_apply(eq_preset_idx_t idx);
void eq_band_set(uint8_t band, float gain_db);
void eq_save(void);
void eq_load(void);
