#include "biquad_eq.h"
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static void calc_low_shelf(biquad_t *b, float freq, float gain_db, float q,
                            float fs) {
    float A  = powf(10.0f, gain_db / 40.0f);
    float w0 = 2.0f * (float)M_PI * freq / fs;
    float cw = cosf(w0);
    float sw = sinf(w0);
    float alpha = sw / 2.0f * sqrtf((A + 1.0f / A) * (1.0f / 1.0f - 1.0f) + 2.0f);
    ARG_UNUSED(q);
    float sqA = sqrtf(A);

    float b0 =  A * ((A + 1.0f) - (A - 1.0f) * cw + 2.0f * sqA * alpha);
    float b1 =  2.0f * A * ((A - 1.0f) - (A + 1.0f) * cw);
    float b2 =  A * ((A + 1.0f) - (A - 1.0f) * cw - 2.0f * sqA * alpha);
    float a0 =       (A + 1.0f) + (A - 1.0f) * cw + 2.0f * sqA * alpha;
    float a1 = -2.0f * ((A - 1.0f) + (A + 1.0f) * cw);
    float a2 =       (A + 1.0f) + (A - 1.0f) * cw - 2.0f * sqA * alpha;

    b->b0 = b0 / a0; b->b1 = b1 / a0; b->b2 = b2 / a0;
    b->a1 = a1 / a0; b->a2 = a2 / a0;
    b->z1 = b->z2 = 0.0f;
}

static void calc_high_shelf(biquad_t *b, float freq, float gain_db, float q,
                             float fs) {
    float A  = powf(10.0f, gain_db / 40.0f);
    float w0 = 2.0f * (float)M_PI * freq / fs;
    float cw = cosf(w0);
    float sw = sinf(w0);
    float alpha = sw / 2.0f * sqrtf((A + 1.0f / A) * (1.0f / 1.0f - 1.0f) + 2.0f);
    ARG_UNUSED(q);
    float sqA = sqrtf(A);

    float b0 =  A * ((A + 1.0f) + (A - 1.0f) * cw + 2.0f * sqA * alpha);
    float b1 = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cw);
    float b2 =  A * ((A + 1.0f) + (A - 1.0f) * cw - 2.0f * sqA * alpha);
    float a0 =       (A + 1.0f) - (A - 1.0f) * cw + 2.0f * sqA * alpha;
    float a1 =  2.0f * ((A - 1.0f) - (A + 1.0f) * cw);
    float a2 =       (A + 1.0f) - (A - 1.0f) * cw - 2.0f * sqA * alpha;

    b->b0 = b0 / a0; b->b1 = b1 / a0; b->b2 = b2 / a0;
    b->a1 = a1 / a0; b->a2 = a2 / a0;
    b->z1 = b->z2 = 0.0f;
}

static void calc_peak(biquad_t *b, float freq, float gain_db, float q,
                       float fs) {
    float A  = powf(10.0f, gain_db / 40.0f);
    float w0 = 2.0f * (float)M_PI * freq / fs;
    float cw = cosf(w0);
    float sw = sinf(w0);
    float alpha = sw / (2.0f * q);

    float b0 = 1.0f + alpha * A;
    float b1 = -2.0f * cw;
    float b2 = 1.0f - alpha * A;
    float a0 = 1.0f + alpha / A;
    float a1 = -2.0f * cw;
    float a2 = 1.0f - alpha / A;

    b->b0 = b0 / a0; b->b1 = b1 / a0; b->b2 = b2 / a0;
    b->a1 = a1 / a0; b->a2 = a2 / a0;
    b->z1 = b->z2 = 0.0f;
}

void biquad_eq_configure(biquad_eq_t *beq, const eq_config_t *cfg,
                         uint32_t sample_rate) {
    memset(beq, 0, sizeof(*beq));
    beq->enabled = cfg->enabled;
    float fs = (float)sample_rate;

    for (int i = 0; i < EQ_BANDS; i++) {
        float freq = (float)cfg->bands[i].freq_hz;
        float gain = cfg->bands[i].gain_db;
        float q    = cfg->bands[i].q > 0.0f ? cfg->bands[i].q : 0.707f;

        for (int ch = 0; ch < 2; ch++) {
            if (i == EQ_BAND_LOW_SHELF) {
                calc_low_shelf(&beq->bands[i][ch], freq, gain, q, fs);
            } else if (i == EQ_BAND_HIGH_SHELF) {
                calc_high_shelf(&beq->bands[i][ch], freq, gain, q, fs);
            } else {
                calc_peak(&beq->bands[i][ch], freq, gain, q, fs);
            }
        }
    }
}

static inline float biquad_tick(biquad_t *b, float x) {
    float y = b->b0 * x + b->z1;
    b->z1   = b->b1 * x - b->a1 * y + b->z2;
    b->z2   = b->b2 * x - b->a2 * y;
    return y;
}

void biquad_eq_process(biquad_eq_t *beq, int16_t *pcm, uint16_t frames,
                       uint8_t channels) {
    if (!beq->enabled) return;

    for (uint16_t i = 0; i < frames; i++) {
        for (uint8_t ch = 0; ch < channels && ch < 2; ch++) {
            float s = (float)pcm[i * channels + ch];
            for (int b = 0; b < EQ_BANDS; b++) {
                s = biquad_tick(&beq->bands[b][ch], s);
            }
            /* Clamp */
            if (s >  32767.0f) s =  32767.0f;
            if (s < -32768.0f) s = -32768.0f;
            pcm[i * channels + ch] = (int16_t)s;
        }
    }
}
