#pragma once
#include <lvgl.h>
#include "playlist.h"

void playlist_ui_init(lv_obj_t *parent, playlist_t *pl);
void playlist_ui_set_active(uint16_t idx);
