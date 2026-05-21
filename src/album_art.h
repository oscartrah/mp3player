#pragma once
#include <lvgl.h>
#include "mp3_meta.h"

bool      album_art_decode(const uint8_t *jpeg_data, uint32_t size);
lv_obj_t *album_art_create_widget(lv_obj_t *parent,
                                   lv_coord_t x, lv_coord_t y);
void      album_art_update(lv_obj_t *img_widget,
                            const mp3_meta_t *meta);
