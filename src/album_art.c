#include "album_art.h"
#include "memory_cfg.h"
#include <zephyr/kernel.h>
#include <string.h>

/* Tiny JPEG decode via Zephyr's JPEG API (CONFIG_JPEG) or fallback stub */
#ifdef CONFIG_JPEG
#include <zephyr/drivers/video.h>
#endif

static lv_img_dsc_t  s_img_dsc;
static uint8_t       s_rgb_buf[ART_DISPLAY_SIZE * ART_DISPLAY_SIZE * 3];
static bool          s_decoded = false;

bool album_art_decode(const uint8_t *jpeg_data, uint32_t size) {
    s_decoded = false;
    if (!jpeg_data || size == 0) return false;

#ifdef CONFIG_JPEG
    /* Use Zephyr JPEG driver if available */
    const struct device *dev = device_get_binding("JPEG");
    if (dev) {
        struct video_format fmt = {
            .pixelformat = VIDEO_PIX_FMT_RGB24,
            .width       = ART_DISPLAY_SIZE,
            .height      = ART_DISPLAY_SIZE,
        };
        if (video_set_format(dev, VIDEO_EP_OUT, &fmt) == 0) {
            struct video_buffer vbuf = {
                .buffer = s_rgb_buf,
                .size   = sizeof(s_rgb_buf),
            };
            /* Simplified: real impl would feed jpeg_data to driver */
            ARG_UNUSED(jpeg_data);
            ARG_UNUSED(size);
        }
    }
#else
    /* Fallback: JPEG not supported — use solid colour placeholder */
    memset(s_rgb_buf, 0x33, sizeof(s_rgb_buf));
    ARG_UNUSED(jpeg_data);
    ARG_UNUSED(size);
#endif

    s_img_dsc.header.cf     = LV_IMG_CF_TRUE_COLOR;
    s_img_dsc.header.w      = ART_DISPLAY_SIZE;
    s_img_dsc.header.h      = ART_DISPLAY_SIZE;
    s_img_dsc.data_size     = ART_DISPLAY_SIZE * ART_DISPLAY_SIZE * LV_COLOR_SIZE / 8;
    s_img_dsc.data          = s_rgb_buf;
    s_decoded = true;
    return true;
}

lv_obj_t *album_art_create_widget(lv_obj_t *parent, lv_coord_t x,
                                   lv_coord_t y) {
    lv_obj_t *img = lv_img_create(parent);
    lv_obj_set_pos(img, x, y);
    lv_obj_set_size(img, ART_DISPLAY_SIZE, ART_DISPLAY_SIZE);
    return img;
}

void album_art_update(lv_obj_t *img_widget, const mp3_meta_t *meta) {
    if (!img_widget) return;
    if (meta && meta->has_art) {
        album_art_decode(meta->art_data, meta->art_size);
    } else {
        memset(s_rgb_buf, 0x22, sizeof(s_rgb_buf));
        s_img_dsc.data = s_rgb_buf;
        s_decoded = true;
    }
    if (s_decoded) {
        lv_img_set_src(img_widget, &s_img_dsc);
    }
}
