#include "mp3_seek.h"

uint32_t mp3_seek_offset(const mp3_meta_t *meta,
                          uint32_t target_sec)
{
    if (meta->duration_sec == 0) return meta->audio_offset;

    float pct = (float)target_sec / (float)meta->duration_sec;
    if (pct > 1.0f) pct = 1.0f;

    if (meta->has_toc && meta->audio_bytes > 0) {
        float toc_idx = pct * 99.0f;
        uint8_t i  = (uint8_t)toc_idx;
        float   f  = toc_idx - i;
        float   fa = meta->toc[i];
        float   fb = (i < 99) ? meta->toc[i + 1] : 256.0f;
        float   byte_pct = (fa + f * (fb - fa)) / 256.0f;
        return meta->audio_offset
             + (uint32_t)(byte_pct * (float)meta->audio_bytes);
    }

    return meta->audio_offset
         + (uint32_t)(pct * (float)meta->audio_bytes);
}
