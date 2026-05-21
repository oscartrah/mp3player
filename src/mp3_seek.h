#pragma once
#include "mp3_meta.h"

uint32_t mp3_seek_offset(const mp3_meta_t *meta,
                         uint32_t target_sec);
