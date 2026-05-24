#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <zephyr/kernel.h>

#define EQ_BANDS 5

typedef enum {
    EQ_BAND_LOW_SHELF = 0,
    EQ_BAND_PEAK_1,
    EQ_BAND_PEAK_2,
    EQ_BAND_PEAK_3,
    EQ_BAND_HIGH_SHELF,
} eq_band_id_t;

typedef struct {
    uint16_t freq_hz;
    float    gain_db;
    float    q;
} eq_band_t;

typedef struct {
    eq_band_t bands[EQ_BANDS];
    bool      enabled;
} eq_config_t;

#define EQ_CONFIG_FLAT {                                          \
    .enabled = false,                                            \
    .bands = {                                                   \
        { .freq_hz =   80, .gain_db = 0.0f, .q = 0.0f },        \
        { .freq_hz =  300, .gain_db = 0.0f, .q = 1.0f },        \
        { .freq_hz = 1000, .gain_db = 0.0f, .q = 1.0f },        \
        { .freq_hz = 3200, .gain_db = 0.0f, .q = 1.0f },        \
        { .freq_hz = 9000, .gain_db = 0.0f, .q = 0.0f },        \
    }                                                            \
}

typedef struct {
    bool     enabled;
    float    threshold_db;
    float    ratio;
    uint32_t attack_ms;
    uint32_t release_ms;
} drc_config_t;

typedef struct audio_hal_api {
    const char *name;
    int  (*init)         (void);
    int  (*set_volume)   (uint8_t vol_pct);
    int  (*set_mute)     (bool mute);
    int  (*set_eq)       (const eq_config_t *eq);
    int  (*set_drc)      (const drc_config_t *drc);
    int  (*set_samplerate)(uint32_t hz);
    bool hw_eq;
    bool hw_drc;
    uint8_t eq_bands;
} audio_hal_api_t;

extern const audio_hal_api_t *g_audio_hal;

int audio_hal_init(void);

static inline int audio_set_volume(uint8_t v) {
    if (!g_audio_hal || !g_audio_hal->set_volume) return -ENODEV;
    return g_audio_hal->set_volume(v);
}
static inline int audio_set_eq(const eq_config_t *eq) {
    if (!g_audio_hal || !g_audio_hal->set_eq) return -ENODEV;
    return g_audio_hal->set_eq(eq);
}
static inline int audio_set_mute(bool m) {
    if (!g_audio_hal || !g_audio_hal->set_mute) return -ENODEV;
    return g_audio_hal->set_mute(m);
}

/* Extern HAL instances */
extern const audio_hal_api_t audio_hal_wm8994;
extern const audio_hal_api_t audio_hal_wm8960;
extern const audio_hal_api_t audio_hal_wm8904;
extern const audio_hal_api_t audio_hal_da7212;
