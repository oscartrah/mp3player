#include "eq_ui.h"
#include "eq_bands.h"
#include "eq_curve.h"
#include "eq_presets.h"
#include "ui_theme.h"
#include "ui_fonts.h"
#include "board_display.h"

static lv_obj_t *s_curve  = NULL;
static lv_obj_t *s_dd     = NULL;

static void on_bands_changed(void) {
    if (s_curve) eq_curve_update(s_curve, &g_eq_current);
    eq_save();
}

static void preset_dd_cb(lv_event_t *e) {
    ARG_UNUSED(e);
    uint16_t sel = lv_dropdown_get_selected(s_dd);
    eq_preset_apply((eq_preset_idx_t)sel);
    if (s_curve) eq_curve_update(s_curve, &g_eq_current);
    /* Sync band sliders via companion widget */
}

static void enabled_sw_cb(lv_event_t *e) {
    lv_obj_t *sw = lv_event_get_target(e);
    g_eq_current.enabled = lv_obj_has_state(sw, LV_STATE_CHECKED);
    audio_set_eq(&g_eq_current);
    eq_save();
    if (s_curve) eq_curve_update(s_curve, &g_eq_current);
}

void eq_ui_init(lv_obj_t *tab) {
    lv_coord_t w = g_display.width;
    lv_coord_t h = g_display.height - lv_dpx(40) - lv_dpx(40);

    /* Header row: EQ enabled toggle + preset dropdown */
    lv_obj_t *hdr = lv_obj_create(tab);
    lv_obj_set_size(hdr, w, lv_dpx(40));
    lv_obj_set_style_bg_color(hdr, UI_COL_HEADER, 0);
    lv_obj_set_style_border_width(hdr, 0, 0);
    lv_obj_set_style_radius(hdr, 0, 0);
    lv_obj_set_style_pad_hor(hdr, UI_PAD_MD, 0);
    lv_obj_clear_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *sw = lv_switch_create(hdr);
    lv_obj_align(sw, LV_ALIGN_LEFT_MID, 0, 0);
    if (g_eq_current.enabled) lv_obj_add_state(sw, LV_STATE_CHECKED);
    lv_obj_add_event_cb(sw, enabled_sw_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t *lbl = lv_label_create(hdr);
    lv_label_set_text(lbl, "EQ");
    lv_obj_set_style_text_font(lbl, UI_FONT(UI_FONT_MD), 0);
    lv_obj_set_style_text_color(lbl, UI_COL_TEXT, 0);
    lv_obj_align_to(lbl, sw, LV_ALIGN_OUT_RIGHT_MID, UI_PAD_SM, 0);

    /* Preset dropdown */
    s_dd = lv_dropdown_create(hdr);
    lv_dropdown_set_options(s_dd,
        "Flat\nBass Boost\nTreble Boost\nVocal\nRock\nJazz\nCustom");
    lv_dropdown_set_selected(s_dd, (uint16_t)g_eq_preset_active);
    lv_obj_set_width(s_dd, lv_dpx(120));
    lv_obj_align(s_dd, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_add_event_cb(s_dd, preset_dd_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* Frequency response curve */
    lv_coord_t curve_h = h / 3;
    s_curve = eq_curve_create(tab, w, curve_h);
    lv_obj_set_pos(s_curve, 0, lv_dpx(40));
    eq_curve_update(s_curve, &g_eq_current);

    /* Band sliders */
    lv_obj_t *bands = eq_bands_create(tab, w, h - curve_h, on_bands_changed);
    lv_obj_set_pos(bands, 0, lv_dpx(40) + curve_h);
    eq_bands_sync(&g_eq_current);
}
