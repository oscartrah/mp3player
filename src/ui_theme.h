#pragma once
#include <lvgl.h>

/* Farben */
#define UI_COL_BG        lv_color_hex(0x1A1A2E)
#define UI_COL_ROW_EVEN  lv_color_hex(0x16213E)
#define UI_COL_ROW_ODD   lv_color_hex(0x0F3460)
#define UI_COL_ACCENT    lv_color_hex(0x00B4D8)
#define UI_COL_TEXT      lv_color_hex(0xE0E0E0)
#define UI_COL_SUBTEXT   lv_color_hex(0x9090B0)
#define UI_COL_HEADER    lv_color_hex(0x533483)
#define UI_COL_BTN_IDLE  lv_color_hex(0x333355)

/* Abstände (dp) */
#define UI_PAD_SM           lv_dpx(4)
#define UI_PAD_MD           lv_dpx(8)
#define UI_PAD_LG           lv_dpx(16)
#define UI_PAD_XL           lv_dpx(24)

/* Komponenten (dp) */
#define UI_TABBAR_H         lv_dpx(40)
#define UI_HEADER_H         lv_dpx(36)
#define UI_BTN_H_SM         lv_dpx(40)
#define UI_BTN_H_MD         lv_dpx(52)
#define UI_BTN_H_LG         lv_dpx(64)
#define UI_BTN_W_SM         lv_dpx(48)
#define UI_BTN_W_MD         lv_dpx(72)
#define UI_BTN_W_LG         lv_dpx(88)
#define UI_SLIDER_H         lv_dpx(10)
#define UI_PROGRESSBAR_H    lv_dpx(10)
#define UI_PLAYLIST_ROW_H   lv_dpx(52)
#define UI_ART_SIZE         lv_dpx(140)
#define UI_RADIUS_SM        lv_dpx(4)
#define UI_RADIUS_MD        lv_dpx(8)
#define UI_RADIUS_CIRCLE    LV_RADIUS_CIRCLE
#define UI_TOUCH_TARGET_MIN lv_dpx(48)
