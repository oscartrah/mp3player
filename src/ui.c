#include "ui.h"
#include "ui_theme.h"
#include "ui_fonts.h"
#include "ui_layout.h"
#include "board_display.h"
#include "album_art.h"
#include "playlist_ui.h"
#include "eq_ui.h"
#include "eq_presets.h"
#include "player_state.h"
#include "playlist.h"
#include "settings.h"
#include <lvgl.h>
#include <stdio.h>
#include <string.h>

/* ── Widgets ──────────────────────────────────── */
static lv_obj_t *s_lbl_title;
static lv_obj_t *s_lbl_artist;
static lv_obj_t *s_lbl_time;
static lv_obj_t *s_bar_progress;
static lv_obj_t *s_btn_play;
static lv_obj_t *s_lbl_play;
static lv_obj_t *s_slider_vol;
static lv_obj_t *s_img_art;
static lv_obj_t *s_tabview;

/* ── Button callbacks ─────────────────────────── */
static void btn_play_cb(lv_event_t *e) {
    ARG_UNUSED(e);
    struct player_command cmd;
    k_mutex_lock(&state_mutex, K_FOREVER);
    player_status_t st = g_state.status;
    k_mutex_unlock(&state_mutex);
    cmd.cmd   = (st == PLAYER_PLAYING) ? CMD_PAUSE : CMD_PLAY;
    cmd.value = 0;
    k_msgq_put(&cmd_queue, &cmd, K_NO_WAIT);
}

static void btn_next_cb(lv_event_t *e) {
    ARG_UNUSED(e);
    struct player_command cmd = { .cmd = CMD_NEXT };
    k_msgq_put(&cmd_queue, &cmd, K_NO_WAIT);
}

static void btn_prev_cb(lv_event_t *e) {
    ARG_UNUSED(e);
    struct player_command cmd = { .cmd = CMD_PREV };
    k_msgq_put(&cmd_queue, &cmd, K_NO_WAIT);
}

static void slider_vol_cb(lv_event_t *e) {
    ARG_UNUSED(e);
    int32_t val = lv_slider_get_value(s_slider_vol);
    struct player_command cmd = { .cmd = CMD_VOLUME, .value = val };
    k_msgq_put(&cmd_queue, &cmd, K_NO_WAIT);
    settings_save_volume((uint8_t)val);
}

/* ── Tab: Now Playing ─────────────────────────── */
static void build_player_tab(lv_obj_t *tab) {
    lv_obj_set_style_bg_color(tab, UI_COL_BG, 0);
    lv_obj_set_style_pad_all(tab, 0, 0);

    /* Album art */
    s_img_art = album_art_create_widget(tab, g_layout.art_x, g_layout.art_y);

    /* Title */
    s_lbl_title = lv_label_create(tab);
    lv_obj_set_style_text_font(s_lbl_title, UI_FONT(UI_FONT_LG), 0);
    lv_obj_set_style_text_color(s_lbl_title, UI_COL_TEXT, 0);
    lv_label_set_long_mode(s_lbl_title, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(s_lbl_title, g_layout.info_w);
    lv_obj_set_pos(s_lbl_title, g_layout.info_x, g_layout.art_y);
    lv_label_set_text(s_lbl_title, "No Track");

    /* Artist */
    s_lbl_artist = lv_label_create(tab);
    lv_obj_set_style_text_font(s_lbl_artist, UI_FONT(UI_FONT_MD), 0);
    lv_obj_set_style_text_color(s_lbl_artist, UI_COL_SUBTEXT, 0);
    lv_label_set_long_mode(s_lbl_artist, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(s_lbl_artist, g_layout.info_w);
    lv_obj_align_to(s_lbl_artist, s_lbl_title, LV_ALIGN_OUT_BOTTOM_LEFT,
                    0, UI_PAD_SM);
    lv_label_set_text(s_lbl_artist, "Unknown");

    /* Progress bar */
    s_bar_progress = lv_bar_create(tab);
    lv_obj_set_size(s_bar_progress, g_layout.progress_w, UI_PROGRESSBAR_H);
    lv_obj_set_pos(s_bar_progress, g_layout.progress_x,
                   g_layout.btn_row_y - lv_dpx(28));
    lv_obj_set_style_bg_color(s_bar_progress, UI_COL_BTN_IDLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_bar_progress, UI_COL_ACCENT, LV_PART_INDICATOR);
    lv_bar_set_range(s_bar_progress, 0, 1000);

    /* Time label */
    s_lbl_time = lv_label_create(tab);
    lv_obj_set_style_text_font(s_lbl_time, UI_FONT(UI_FONT_XS), 0);
    lv_obj_set_style_text_color(s_lbl_time, UI_COL_SUBTEXT, 0);
    lv_obj_align_to(s_lbl_time, s_bar_progress, LV_ALIGN_OUT_BOTTOM_RIGHT,
                    0, UI_PAD_SM);
    lv_label_set_text(s_lbl_time, "0:00 / 0:00");

    /* Control buttons */
    lv_coord_t btn_y = g_layout.btn_row_y;

    lv_obj_t *btn_prev = lv_btn_create(tab);
    lv_obj_set_size(btn_prev, UI_BTN_W_MD, UI_BTN_H_MD);
    lv_obj_set_pos(btn_prev, g_layout.progress_x, btn_y);
    lv_obj_set_style_bg_color(btn_prev, UI_COL_BTN_IDLE, 0);
    lv_obj_set_style_radius(btn_prev, UI_RADIUS_CIRCLE, 0);
    lv_obj_add_event_cb(btn_prev, btn_prev_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_prev = lv_label_create(btn_prev);
    lv_label_set_text(lbl_prev, LV_SYMBOL_PREV);
    lv_obj_center(lbl_prev);

    s_btn_play = lv_btn_create(tab);
    lv_obj_set_size(s_btn_play, UI_BTN_W_LG, UI_BTN_H_LG);
    lv_obj_set_pos(s_btn_play,
                   g_layout.progress_x + UI_BTN_W_MD + UI_PAD_MD, btn_y);
    lv_obj_set_style_bg_color(s_btn_play, UI_COL_ACCENT, 0);
    lv_obj_set_style_radius(s_btn_play, UI_RADIUS_CIRCLE, 0);
    lv_obj_add_event_cb(s_btn_play, btn_play_cb, LV_EVENT_CLICKED, NULL);
    s_lbl_play = lv_label_create(s_btn_play);
    lv_label_set_text(s_lbl_play, LV_SYMBOL_PLAY);
    lv_obj_center(s_lbl_play);

    lv_obj_t *btn_next = lv_btn_create(tab);
    lv_obj_set_size(btn_next, UI_BTN_W_MD, UI_BTN_H_MD);
    lv_obj_set_pos(btn_next,
                   g_layout.progress_x + UI_BTN_W_MD + UI_PAD_MD +
                   UI_BTN_W_LG + UI_PAD_MD, btn_y);
    lv_obj_set_style_bg_color(btn_next, UI_COL_BTN_IDLE, 0);
    lv_obj_set_style_radius(btn_next, UI_RADIUS_CIRCLE, 0);
    lv_obj_add_event_cb(btn_next, btn_next_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_next = lv_label_create(btn_next);
    lv_label_set_text(lbl_next, LV_SYMBOL_NEXT);
    lv_obj_center(lbl_next);

    /* Volume slider */
    s_slider_vol = lv_slider_create(tab);
    lv_obj_set_size(s_slider_vol, g_layout.progress_w, UI_SLIDER_H);
    lv_obj_set_pos(s_slider_vol, g_layout.progress_x, g_layout.vol_y);
    lv_slider_set_range(s_slider_vol, 0, 100);
    lv_slider_set_value(s_slider_vol, 75, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(s_slider_vol, UI_COL_ACCENT, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(s_slider_vol, UI_COL_BTN_IDLE, LV_PART_MAIN);
    lv_obj_add_event_cb(s_slider_vol, slider_vol_cb, LV_EVENT_VALUE_CHANGED,
                        NULL);
}

/* ── ui_init ──────────────────────────────────── */
void ui_init(void) {
    display_profile_init();
    ui_fonts_init(g_display.dpi);
    ui_layout_init();
    eq_load();

    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, UI_COL_BG, 0);

    /* Tab view */
    s_tabview = lv_tabview_create(scr);
    lv_tabview_set_tab_bar_position(s_tabview, LV_DIR_TOP);
    lv_tabview_set_tab_bar_size(s_tabview, UI_TABBAR_H);
    lv_obj_set_style_bg_color(s_tabview, UI_COL_BG, 0);
    lv_obj_set_size(s_tabview, g_display.width, g_display.height);

    lv_obj_t *tab_player   = lv_tabview_add_tab(s_tabview, LV_SYMBOL_AUDIO " Now Playing");
    lv_obj_t *tab_playlist = lv_tabview_add_tab(s_tabview, LV_SYMBOL_LIST  " Playlist");
    lv_obj_t *tab_eq       = lv_tabview_add_tab(s_tabview, "EQ");

    build_player_tab(tab_player);
    playlist_ui_init(tab_playlist, &g_playlist);
    eq_ui_init(tab_eq);

    /* Initial volume from state */
    k_mutex_lock(&state_mutex, K_FOREVER);
    lv_slider_set_value(s_slider_vol, g_state.volume, LV_ANIM_OFF);
    k_mutex_unlock(&state_mutex);
}

/* ── ui_update (called every 16 ms from UI thread) */
void ui_update(void) {
    static player_status_t last_status = PLAYER_STOPPED;
    static uint32_t last_elapsed = UINT32_MAX;

    k_mutex_lock(&state_mutex, K_FOREVER);
    struct player_state st = g_state;
    k_mutex_unlock(&state_mutex);

    /* Update play/pause icon */
    if (st.status != last_status) {
        lv_label_set_text(s_lbl_play,
            st.status == PLAYER_PLAYING ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
        last_status = st.status;
    }

    /* Update track info (cheap string compare) */
    if (strcmp(lv_label_get_text(s_lbl_title), st.track_name)) {
        lv_label_set_text(s_lbl_title, st.track_name);
        lv_label_set_text(s_lbl_artist, st.artist);
    }

    /* Update progress + time */
    if (st.elapsed_sec != last_elapsed) {
        last_elapsed = st.elapsed_sec;
        if (st.total_sec > 0) {
            lv_bar_set_value(s_bar_progress,
                (int32_t)((st.elapsed_sec * 1000) / st.total_sec), LV_ANIM_OFF);
        }
        char tbuf[24];
        snprintf(tbuf, sizeof(tbuf), "%u:%02u / %u:%02u",
                 st.elapsed_sec / 60, st.elapsed_sec % 60,
                 st.total_sec   / 60, st.total_sec   % 60);
        lv_label_set_text(s_lbl_time, tbuf);
    }
}
