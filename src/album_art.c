#include "album_art.h"
#include "memory_cfg.h"
#include <zephyr/kernel.h>
#include <string.h>
#include <lvgl.h>

#ifdef HAS_TJPGDEC
#include "tjpgd.h"

#define TJPGD_WORK_SIZE 4096
static uint8_t s_work[TJPGD_WORK_SIZE];

/* Input callback: feed raw JPEG bytes to TJpgDec */
typedef struct { const uint8_t *data; uint32_t size; uint32_t pos; } jpeg_src_t;

static unsigned jpeg_in(JDEC *jd, uint8_t *buf, unsigned len) {
    jpeg_src_t *src = (jpeg_src_t *)jd->device;
    unsigned avail = src->size - src->pos;
    if (len > avail) len = avail;
    if (buf) memcpy(buf, src->data + src->pos, len);
    src->pos += len;
    return len;
}

/* Output callback: write one decoded MCU block into the pixel buffer */
static lv_color_t s_pixel_buf[ART_DISPLAY_SIZE * ART_DISPLAY_SIZE] MEM_SECTION;

static int jpeg_out(JDEC *jd, void *bitmap, JRECT *rect) {
    ARG_UNUSED(jd);
    const uint8_t *src = (const uint8_t *)bitmap;  /* RGB888 */
    uint16_t bw = rect->right  - rect->left + 1;
    uint16_t bh = rect->bottom - rect->top  + 1;

    for (uint16_t y = 0; y < bh; y++) {
        for (uint16_t x = 0; x < bw; x++) {
            uint32_t px = (uint32_t)(rect->top + y) * ART_DISPLAY_SIZE
                        + (rect->left + x);
            if (px >= (uint32_t)(ART_DISPLAY_SIZE * ART_DISPLAY_SIZE)) continue;
            const uint8_t *p = src + (y * bw + x) * 3;
            s_pixel_buf[px] = lv_color_make(p[0], p[1], p[2]);
        }
    }
    return 1; /* 1 = continue */
}

#else
/* Fallback when TJpgDec was not fetched */
static lv_color_t s_pixel_buf[ART_DISPLAY_SIZE * ART_DISPLAY_SIZE] MEM_SECTION;
#endif /* HAS_TJPGDEC */

static lv_img_dsc_t s_img_dsc;
static bool         s_decoded = false;

bool album_art_decode(const uint8_t *jpeg_data, uint32_t size) {
    s_decoded = false;
    if (!jpeg_data || size == 0) return false;

#ifdef HAS_TJPGDEC
    JDEC jd;
    jpeg_src_t src = { .data = jpeg_data, .size = size, .pos = 0 };

    /* Initialise decompressor */
    JRESULT rc = jd_prepare(&jd, jpeg_in, s_work, TJPGD_WORK_SIZE, &src);
    if (rc != JDR_OK) return false;

    /* Scale down to fit ART_DISPLAY_SIZE: pick largest 1/N that still fits */
    uint8_t scale = 0;
    while (scale < 3 &&
           (jd.width  >> (scale + 1)) >= ART_DISPLAY_SIZE &&
           (jd.height >> (scale + 1)) >= ART_DISPLAY_SIZE) {
        scale++;
    }

    memset(s_pixel_buf, 0, sizeof(s_pixel_buf));
    rc = jd_decomp(&jd, jpeg_out, scale);
    if (rc != JDR_OK) return false;
#else
    /* No JPEG decoder — solid grey placeholder */
    ARG_UNUSED(jpeg_data);
    ARG_UNUSED(size);
    lv_color_t grey = lv_color_make(0x44, 0x44, 0x44);
    for (int i = 0; i < ART_DISPLAY_SIZE * ART_DISPLAY_SIZE; i++) {
        s_pixel_buf[i] = grey;
    }
#endif

    s_img_dsc.header.magic  = LV_IMAGE_HEADER_MAGIC;
    s_img_dsc.header.cf     = LV_COLOR_FORMAT_NATIVE;
    s_img_dsc.header.w      = ART_DISPLAY_SIZE;
    s_img_dsc.header.h      = ART_DISPLAY_SIZE;
    s_img_dsc.header.stride = ART_DISPLAY_SIZE * sizeof(lv_color_t);
    s_img_dsc.data_size     = sizeof(s_pixel_buf);
    s_img_dsc.data          = (const uint8_t *)s_pixel_buf;
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

    bool ok = false;
    if (meta && meta->has_art) {
        ok = album_art_decode(meta->art_data, meta->art_size);
    }
    if (!ok) {
        /* Grey placeholder */
        lv_color_t grey = lv_color_make(0x33, 0x33, 0x33);
        for (int i = 0; i < ART_DISPLAY_SIZE * ART_DISPLAY_SIZE; i++) {
            s_pixel_buf[i] = grey;
        }
        s_img_dsc.header.magic  = LV_IMAGE_HEADER_MAGIC;
        s_img_dsc.header.cf     = LV_COLOR_FORMAT_NATIVE;
        s_img_dsc.header.w      = ART_DISPLAY_SIZE;
        s_img_dsc.header.h      = ART_DISPLAY_SIZE;
        s_img_dsc.header.stride = ART_DISPLAY_SIZE * sizeof(lv_color_t);
        s_img_dsc.data_size     = sizeof(s_pixel_buf);
        s_img_dsc.data          = (const uint8_t *)s_pixel_buf;
        s_decoded = true;
    }
    if (s_decoded) {
        lv_img_set_src(img_widget, &s_img_dsc);
    }
}
