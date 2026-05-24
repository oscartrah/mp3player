#include "playlist_ui.h"
#include "player_state.h"
#include "ui_theme.h"
#include "ui_fonts.h"
#include <string.h>

static lv_obj_t  *s_list  = NULL;
static playlist_t *s_pl   = NULL;
static uint16_t   s_active = UINT16_MAX;

static void row_click_cb(lv_event_t *e) {
    uint16_t idx = (uint16_t)(uintptr_t)lv_event_get_user_data(e);
    if (!s_pl || idx >= s_pl->count) return;
    s_pl->current = (int16_t)idx;
    playlist_ui_set_active(idx);

    struct player_command cmd = {0};
    cmd.cmd = CMD_PLAY_PATH;
    strncpy(cmd.path, s_pl->entries[idx].path, sizeof(cmd.path) - 1);
    k_msgq_put(&cmd_queue, &cmd, K_NO_WAIT);
}

void playlist_ui_init(lv_obj_t *parent, playlist_t *pl) {
    s_pl = pl;

    s_list = lv_obj_create(parent);
    lv_obj_set_size(s_list, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(s_list, UI_COL_BG, 0);
    lv_obj_set_style_border_width(s_list, 0, 0);
    lv_obj_set_style_pad_all(s_list, 0, 0);
    lv_obj_set_flex_flow(s_list, LV_FLEX_FLOW_COLUMN);

    for (uint16_t i = 0; i < pl->count; i++) {
        lv_obj_t *row = lv_obj_create(s_list);
        lv_obj_set_size(row, lv_pct(100), UI_PLAYLIST_ROW_H);
        lv_obj_set_style_bg_color(row,
            (i & 1) ? UI_COL_ROW_ODD : UI_COL_ROW_EVEN, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_radius(row, 0, 0);
        lv_obj_set_style_pad_hor(row, UI_PAD_MD, 0);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *lbl = lv_label_create(row);
        lv_obj_set_style_text_font(lbl, UI_FONT(UI_FONT_SM), 0);
        lv_obj_set_style_text_color(lbl, UI_COL_TEXT, 0);
        lv_label_set_long_mode(lbl, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_width(lbl, lv_pct(80));
        lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 0, 0);

        char buf[META_STR_MAX * 2];
        snprintf(buf, sizeof(buf), "%s — %s",
                 pl->entries[i].title, pl->entries[i].artist);
        lv_label_set_text(lbl, buf);

        lv_obj_add_event_cb(row, row_click_cb, LV_EVENT_CLICKED,
                            (void *)(uintptr_t)i);
    }
}

void playlist_ui_set_active(uint16_t idx) {
    if (!s_list) return;
    uint32_t cnt = lv_obj_get_child_cnt(s_list);

    if (s_active < cnt) {
        lv_obj_t *old = lv_obj_get_child(s_list, s_active);
        lv_obj_set_style_bg_color(old,
            (s_active & 1) ? UI_COL_ROW_ODD : UI_COL_ROW_EVEN, 0);
    }
    if (idx < cnt) {
        lv_obj_t *row = lv_obj_get_child(s_list, idx);
        lv_obj_set_style_bg_color(row, UI_COL_ACCENT, 0);
        lv_obj_scroll_to_view(row, LV_ANIM_ON);
    }
    s_active = idx;
}
