#pragma once
#include <stdint.h>
#include "audio_hal.h"

typedef struct {
    float b0, b1, b2;
    float a1, a2;
    float z1, z2;
} biquad_t;

typedef struct {
    biquad_t bands[EQ_BANDS][2];
    bool     enabled;
} biquad_eq_t;

void biquad_eq_configure(biquad_eq_t *beq,
                         const eq_config_t *cfg,
                         uint32_t sample_rate);

void biquad_eq_process(biquad_eq_t *beq,
                       int16_t *pcm,
                       uint16_t frames,
                       uint8_t channels);
