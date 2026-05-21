#include "ui_fonts.h"

/* Built-in LVGL fonts used at two DPI tiers */
static const lv_font_t *fonts_lo[UI_FONT_COUNT] = {
    [UI_FONT_XS]   = &lv_font_montserrat_10,
    [UI_FONT_SM]   = &lv_font_montserrat_12,
    [UI_FONT_MD]   = &lv_font_montserrat_14,
    [UI_FONT_LG]   = &lv_font_montserrat_18,
    [UI_FONT_XL]   = &lv_font_montserrat_22,
    [UI_FONT_ICON] = &lv_font_montserrat_16,
};

static const lv_font_t *fonts_hi[UI_FONT_COUNT] = {
    [UI_FONT_XS]   = &lv_font_montserrat_14,
    [UI_FONT_SM]   = &lv_font_montserrat_16,
    [UI_FONT_MD]   = &lv_font_montserrat_20,
    [UI_FONT_LG]   = &lv_font_montserrat_24,
    [UI_FONT_XL]   = &lv_font_montserrat_28,
    [UI_FONT_ICON] = &lv_font_montserrat_20,
};

const lv_font_t **g_ui_fonts = fonts_lo;

void ui_fonts_init(uint16_t dpi) {
    g_ui_fonts = (dpi >= 200) ? fonts_hi : fonts_lo;
}
