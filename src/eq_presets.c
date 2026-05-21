#include "eq_presets.h"
#include "settings.h"
#include "biquad_eq.h"
#include "player_state.h"

const eq_preset_t g_eq_presets[EQ_PRESET_COUNT] = {
    [EQ_PRESET_IDX_FLAT] = {
        .name   = "Flat",
        .config = EQ_CONFIG_FLAT,
    },
    [EQ_PRESET_IDX_BASS_BOOST] = {
        .name = "Bass Boost",
        .config = {
            .enabled = true,
            .bands = {
                { .freq_hz =   80, .gain_db =  8.0f, .q = 0.0f },
                { .freq_hz =  300, .gain_db =  3.0f, .q = 1.0f },
                { .freq_hz = 1000, .gain_db =  0.0f, .q = 1.0f },
                { .freq_hz = 3200, .gain_db = -1.0f, .q = 1.0f },
                { .freq_hz = 9000, .gain_db =  0.0f, .q = 0.0f },
            }
        }
    },
    [EQ_PRESET_IDX_TREBLE_BOOST] = {
        .name = "Treble Boost",
        .config = {
            .enabled = true,
            .bands = {
                { .freq_hz =   80, .gain_db =  0.0f, .q = 0.0f },
                { .freq_hz =  300, .gain_db =  0.0f, .q = 1.0f },
                { .freq_hz = 1000, .gain_db =  0.0f, .q = 1.0f },
                { .freq_hz = 3200, .gain_db =  3.0f, .q = 1.0f },
                { .freq_hz = 9000, .gain_db =  7.0f, .q = 0.0f },
            }
        }
    },
    [EQ_PRESET_IDX_VOCAL] = {
        .name = "Vocal",
        .config = {
            .enabled = true,
            .bands = {
                { .freq_hz =   80, .gain_db = -2.0f, .q = 0.0f },
                { .freq_hz =  300, .gain_db =  1.0f, .q = 1.2f },
                { .freq_hz = 1000, .gain_db =  5.0f, .q = 1.5f },
                { .freq_hz = 3200, .gain_db =  4.0f, .q = 1.0f },
                { .freq_hz = 9000, .gain_db =  2.0f, .q = 0.0f },
            }
        }
    },
    [EQ_PRESET_IDX_ROCK] = {
        .name = "Rock",
        .config = {
            .enabled = true,
            .bands = {
                { .freq_hz =   80, .gain_db =  6.0f, .q = 0.0f },
                { .freq_hz =  300, .gain_db =  2.0f, .q = 1.0f },
                { .freq_hz = 1000, .gain_db = -1.0f, .q = 1.0f },
                { .freq_hz = 3200, .gain_db =  3.0f, .q = 1.0f },
                { .freq_hz = 9000, .gain_db =  5.0f, .q = 0.0f },
            }
        }
    },
    [EQ_PRESET_IDX_JAZZ] = {
        .name = "Jazz",
        .config = {
            .enabled = true,
            .bands = {
                { .freq_hz =   80, .gain_db =  3.0f, .q = 0.0f },
                { .freq_hz =  300, .gain_db =  0.0f, .q = 1.0f },
                { .freq_hz = 1000, .gain_db =  0.0f, .q = 1.0f },
                { .freq_hz = 3200, .gain_db =  2.0f, .q = 1.0f },
                { .freq_hz = 9000, .gain_db =  4.0f, .q = 0.0f },
            }
        }
    },
    [EQ_PRESET_IDX_CUSTOM] = {
        .name   = "Custom",
        .config = EQ_CONFIG_FLAT,
    },
};

eq_preset_idx_t g_eq_preset_active = EQ_PRESET_IDX_FLAT;
eq_config_t     g_eq_current       = EQ_CONFIG_FLAT;

extern struct k_mutex state_mutex;

void eq_preset_apply(eq_preset_idx_t idx) {
    if (idx >= EQ_PRESET_COUNT) return;
    g_eq_preset_active = idx;
    g_eq_current = g_eq_presets[idx].config;
    audio_set_eq(&g_eq_current);
}

void eq_band_set(uint8_t band, float gain_db) {
    if (band >= EQ_BANDS) return;
    g_eq_current.enabled = true;
    g_eq_current.bands[band].gain_db = gain_db;
    g_eq_preset_active = EQ_PRESET_IDX_CUSTOM;
    audio_set_eq(&g_eq_current);
}

void eq_save(void) {
    settings_save_eq(&g_eq_current);
}

void eq_load(void) {
    app_settings_t cfg;
    if (settings_load(&cfg) == 0) {
        g_eq_current = cfg.eq;
        audio_set_eq(&g_eq_current);
    }
}
