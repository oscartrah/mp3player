#include "eq_curve.h"
#include "ui_theme.h"
#include "memory_cfg.h"
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define CURVE_POINTS 64
#define DB_RANGE     12.0f

static lv_color_t s_cbuf[240 * 120] MEM_SECTION;

static float eval_db(const eq_config_t *eq, float freq, float fs) {
    float total = 0.0f;
    for (int i = 0; i < EQ_BANDS; i++) {
        float fc = (float)eq->bands[i].freq_hz;
        float g  = eq->bands[i].gain_db;
        float q  = eq->bands[i].q > 0 ? eq->bands[i].q : 0.707f;
        float w  = 2.0f * (float)M_PI * freq / fs;
        float wc = 2.0f * (float)M_PI * fc   / fs;

        if (i == EQ_BAND_LOW_SHELF || i == EQ_BAND_HIGH_SHELF) {
            /* Simplified: contribution proportional to proximity on log scale */
            float ratio = (i == EQ_BAND_LOW_SHELF) ? (wc / w) : (w / wc);
            total += g / (1.0f + ratio * ratio);
        } else {
            float dw = w - wc;
            float bw = wc / q;
            total += g / (1.0f + (dw * dw) / (bw * bw * 0.25f));
        }
    }
    return total;
}

lv_obj_t *eq_curve_create(lv_obj_t *parent, lv_coord_t w, lv_coord_t h) {
    lv_obj_t *canvas = lv_canvas_create(parent);
    lv_canvas_set_buffer(canvas, s_cbuf, w, h, LV_COLOR_FORMAT_NATIVE);
    lv_canvas_fill_bg(canvas, UI_COL_BG, LV_OPA_COVER);
    lv_obj_set_size(canvas, w, h);
    return canvas;
}

void eq_curve_update(lv_obj_t *canvas, const eq_config_t *eq) {
    lv_coord_t w = lv_obj_get_width(canvas);
    lv_coord_t h = lv_obj_get_height(canvas);

    lv_canvas_fill_bg(canvas, UI_COL_BG, LV_OPA_COVER);

    lv_layer_t layer;
    lv_canvas_init_layer(canvas, &layer);

    /* Draw 0 dB centre line */
    lv_draw_line_dsc_t ld;
    lv_draw_line_dsc_init(&ld);
    ld.color = UI_COL_SUBTEXT;
    ld.width = 1;
    ld.p1.x = 0;     ld.p1.y = h / 2;
    ld.p2.x = w - 1; ld.p2.y = h / 2;
    lv_draw_line(&layer, &ld);

    /* Draw EQ curve as connected segments */
    ld.color = UI_COL_ACCENT;
    ld.width = 2;

    float fs = 44100.0f;
    float f_lo = logf(20.0f);
    float f_hi = logf(20000.0f);

    lv_point_precise_t pts[CURVE_POINTS];
    for (int i = 0; i < CURVE_POINTS; i++) {
        float t    = (float)i / (CURVE_POINTS - 1);
        float freq = expf(f_lo + t * (f_hi - f_lo));
        float db   = eq->enabled ? eval_db(eq, freq, fs) : 0.0f;
        float norm = db / DB_RANGE;
        if (norm >  1.0f) norm =  1.0f;
        if (norm < -1.0f) norm = -1.0f;
        pts[i].x = (lv_value_precise_t)(int32_t)(t * (w - 1));
        pts[i].y = (lv_value_precise_t)(int32_t)((0.5f - norm * 0.5f) * (h - 1));
    }
    ld.points    = pts;
    ld.point_cnt = CURVE_POINTS;
    lv_draw_line(&layer, &ld);

    lv_canvas_finish_layer(canvas, &layer);
}
