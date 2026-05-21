#pragma once
#include <lvgl.h>
#include "audio_hal.h"

typedef void (*eq_bands_changed_cb_t)(void);

lv_obj_t *eq_bands_create(lv_obj_t *parent,
                           lv_coord_t w,
                           lv_coord_t h,
                           eq_bands_changed_cb_t on_change);
void eq_bands_sync(const eq_config_t *eq);
