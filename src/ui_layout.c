#include "ui_layout.h"
#include "board_display.h"
#include "memory_cfg.h"

ui_layout_t g_layout;

void ui_layout_init(void) {
    uint16_t w = g_display.width;
    uint16_t h = g_display.height;

    if (w < 480 || h < 320) {
        /* Compact layout: 320×240 or smaller */
        g_layout.cls         = LAYOUT_COMPACT;
        g_layout.art_x       = 4;
        g_layout.art_y       = 4;
        g_layout.info_x      = 100;
        g_layout.info_w      = (lv_coord_t)(w - 104);
        g_layout.progress_x  = 4;
        g_layout.progress_w  = (lv_coord_t)(w - 8);
        g_layout.btn_row_y   = (lv_coord_t)(h - 56);
        g_layout.vol_y       = (lv_coord_t)(h - 28);
        g_layout.playlist_cols = 1;
    } else {
        /* Normal layout: 480×272 and above */
        g_layout.cls         = LAYOUT_NORMAL;
        g_layout.art_x       = 8;
        g_layout.art_y       = 8;
        g_layout.info_x      = (lv_coord_t)(ART_DISPLAY_SIZE + 16);
        g_layout.info_w      = (lv_coord_t)(w - ART_DISPLAY_SIZE - 24);
        g_layout.progress_x  = 8;
        g_layout.progress_w  = (lv_coord_t)(w - 16);
        g_layout.btn_row_y   = (lv_coord_t)(h - 64);
        g_layout.vol_y       = (lv_coord_t)(h - 32);
        g_layout.playlist_cols = 1;
    }
}
