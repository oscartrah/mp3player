#pragma once
#include <lvgl.h>

typedef enum {
    UI_FONT_XS = 0,
    UI_FONT_SM,
    UI_FONT_MD,
    UI_FONT_LG,
    UI_FONT_XL,
    UI_FONT_ICON,
    UI_FONT_COUNT,
} ui_font_id_t;

extern const lv_font_t **g_ui_fonts;

void ui_fonts_init(uint16_t dpi);

#define UI_FONT(id)  (g_ui_fonts[(id)])
