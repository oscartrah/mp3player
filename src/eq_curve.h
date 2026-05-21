#pragma once
#include <lvgl.h>
#include "audio_hal.h"

lv_obj_t *eq_curve_create(lv_obj_t *parent,
                           lv_coord_t w, lv_coord_t h);
void eq_curve_update(lv_obj_t *canvas,
                     const eq_config_t *eq);
