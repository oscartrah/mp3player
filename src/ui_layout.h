#pragma once
#include <lvgl.h>
#include "board_display.h"

typedef enum {
    LAYOUT_COMPACT = 0,
    LAYOUT_NORMAL,
} ui_layout_class_t;

typedef struct {
    ui_layout_class_t cls;
    lv_coord_t art_x;
    lv_coord_t art_y;
    lv_coord_t info_x;
    lv_coord_t info_w;
    lv_coord_t progress_x;
    lv_coord_t progress_w;
    lv_coord_t btn_row_y;
    lv_coord_t vol_y;
    uint8_t    playlist_cols;
} ui_layout_t;

extern ui_layout_t g_layout;
void ui_layout_init(void);
