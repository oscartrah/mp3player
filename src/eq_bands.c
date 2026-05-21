#include "eq_bands.h"
#include "eq_presets.h"
#include "ui_theme.h"
#include "ui_fonts.h"
#include <stdio.h>

static lv_obj_t           *s_sliders[EQ_BANDS];
static eq_bands_changed_cb_t s_cb = NULL;

static const char *band_labels[EQ_BANDS] = {
    "80", "300", "1k", "3.2k", "9k"
};

static void slider_cb(lv_event_t *e) {
    uint8_t band = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
    lv_obj_t *sl = lv_event_get_target(e);
    int32_t val  = lv_slider_get_value(sl);
    float db     = (float)val / 10.0f;
    eq_band_set(band, db);
    if (s_cb) s_cb();
}

lv_obj_t *eq_bands_create(lv_obj_t *parent, lv_coord_t w, lv_coord_t h,
                           eq_bands_changed_cb_t on_change) {
    s_cb = on_change;

    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, w, h);
    lv_obj_set_style_bg_color(cont, UI_COL_BG, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_pad_all(cont, UI_PAD_SM, 0);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_SPACE_EVENLY,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_coord_t sl_h = h - lv_dpx(24);

    for (int i = 0; i < EQ_BANDS; i++) {
        lv_obj_t *col = lv_obj_create(cont);
        lv_obj_set_size(col, w / EQ_BANDS - UI_PAD_SM, h);
        lv_obj_set_style_bg_opa(col, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(col, 0, 0);
        lv_obj_set_style_pad_all(col, 0, 0);
        lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(col, LV_FLEX_ALIGN_CENTER,
                              LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        lv_obj_t *sl = lv_slider_create(col);
        lv_obj_set_size(sl, UI_SLIDER_H, sl_h);
        lv_slider_set_range(sl, -120, 120);  /* ±12 dB in tenths */
        lv_slider_set_value(sl, 0, LV_ANIM_OFF);
        lv_obj_set_style_bg_color(sl, UI_COL_ACCENT, LV_PART_INDICATOR);
        lv_obj_set_style_bg_color(sl, UI_COL_BTN_IDLE, LV_PART_MAIN);
        lv_obj_add_event_cb(sl, slider_cb, LV_EVENT_VALUE_CHANGED,
                            (void *)(uintptr_t)i);
        s_sliders[i] = sl;

        lv_obj_t *lbl = lv_label_create(col);
        lv_obj_set_style_text_font(lbl, UI_FONT(UI_FONT_XS), 0);
        lv_obj_set_style_text_color(lbl, UI_COL_SUBTEXT, 0);
        lv_label_set_text(lbl, band_labels[i]);
    }
    return cont;
}

void eq_bands_sync(const eq_config_t *eq) {
    for (int i = 0; i < EQ_BANDS; i++) {
        if (s_sliders[i]) {
            int32_t val = (int32_t)(eq->bands[i].gain_db * 10.0f);
            lv_slider_set_value(s_sliders[i], val, LV_ANIM_OFF);
        }
    }
}
