#include "mp3_meta.h"
#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

uint8_t art_buf[ART_BUF_SIZE] MEM_SECTION;

static uint32_t read_syncsafe(const uint8_t *b) {
    return ((uint32_t)(b[0] & 0x7F) << 21) |
           ((uint32_t)(b[1] & 0x7F) << 14) |
           ((uint32_t)(b[2] & 0x7F) <<  7) |
           ((uint32_t)(b[3] & 0x7F));
}

static uint32_t read_be32(const uint8_t *b) {
    return ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) |
           ((uint32_t)b[2] << 8)  |  (uint32_t)b[3];
}

static void parse_latin1(char *dst, const uint8_t *src, uint32_t len, uint32_t max) {
    uint32_t n = (len < max - 1) ? len : max - 1;
    memcpy(dst, src, n);
    dst[n] = '\0';
}

static void parse_utf16(char *dst, const uint8_t *src, uint32_t len, uint32_t max) {
    /* Simple BOM-aware UTF-16 → ASCII strip (embedded boards, ASCII subset only) */
    uint32_t i = 0, o = 0;
    if (len >= 2 && ((src[0] == 0xFF && src[1] == 0xFE) ||
                     (src[0] == 0xFE && src[1] == 0xFF))) {
        i = 2;
    }
    while (i + 1 < len && o < max - 1) {
        uint16_t ch = (src[i]) | ((uint16_t)src[i + 1] << 8);
        dst[o++] = (ch < 0x80) ? (char)ch : '?';
        i += 2;
    }
    dst[o] = '\0';
}

static void parse_id3_string(char *dst, const uint8_t *data, uint32_t len, uint32_t max) {
    if (len == 0) { dst[0] = '\0'; return; }
    uint8_t enc = data[0];
    if (enc == 0x01 || enc == 0x02) {
        parse_utf16(dst, data + 1, len - 1, max);
    } else {
        parse_latin1(dst, data + 1, len - 1, max);
    }
}

static bool parse_id3v2(struct fs_file_t *f, mp3_meta_t *meta) {
    uint8_t hdr[10];
    if (fs_read(f, hdr, 10) != 10) return false;
    if (hdr[0] != 'I' || hdr[1] != 'D' || hdr[2] != '3') return false;

    uint32_t tag_size = read_syncsafe(hdr + 6);
    meta->audio_offset = tag_size + 10;

    uint8_t frame_hdr[10];
    uint32_t pos = 0;
    uint8_t *tmp = art_buf;

    while (pos + 10 < tag_size) {
        if (fs_read(f, frame_hdr, 10) != 10) break;
        pos += 10;

        if (frame_hdr[0] == 0) break;

        uint32_t fsize = read_be32(frame_hdr + 4);
        if (fsize == 0 || pos + fsize > tag_size) break;

        char fid[5] = {0};
        memcpy(fid, frame_hdr, 4);

        uint8_t *fbuf = malloc(fsize);
        if (!fbuf) { fs_seek(f, fsize, FS_SEEK_CUR); pos += fsize; continue; }
        fs_read(f, fbuf, fsize);
        pos += fsize;

        if (!strcmp(fid, "TIT2")) {
            parse_id3_string(meta->title,  fbuf, fsize, META_STR_MAX);
        } else if (!strcmp(fid, "TPE1")) {
            parse_id3_string(meta->artist, fbuf, fsize, META_STR_MAX);
        } else if (!strcmp(fid, "TALB")) {
            parse_id3_string(meta->album,  fbuf, fsize, META_STR_MAX);
        } else if (!strcmp(fid, "TRCK")) {
            char tmp_str[8] = {0};
            parse_id3_string(tmp_str, fbuf, fsize, sizeof(tmp_str));
            meta->track_nr = (uint8_t)atoi(tmp_str);
        } else if (!strcmp(fid, "APIC") && !meta->has_art) {
            /* encoding(1) + mime(\0) + pic_type(1) + desc(\0) + data */
            uint32_t off = 1;
            /* mime string */
            uint32_t ms = 0;
            while (off + ms < fsize && fbuf[off + ms]) ms++;
            memcpy(meta->art_mime, fbuf + off,
                   ms < sizeof(meta->art_mime) - 1 ? ms : sizeof(meta->art_mime) - 1);
            off += ms + 1 + 1; /* skip mime NUL + pic_type */
            /* skip description */
            while (off < fsize && fbuf[off]) off++;
            off++;
            uint32_t art_len = fsize - off;
            if (art_len <= ART_BUF_SIZE) {
                memcpy(art_buf, fbuf + off, art_len);
                meta->art_data = art_buf;
                meta->art_size = art_len;
                meta->has_art  = true;
            }
        } else if (!strcmp(fid, "XING") || !strcmp(fid, "Info")) {
            /* VBR TOC */
            if (fsize >= 8) {
                uint32_t flags = read_be32(fbuf);
                uint32_t o2 = 4;
                if (flags & 0x01) { meta->total_frames = read_be32(fbuf + o2); o2 += 4; }
                if (flags & 0x02) { meta->audio_bytes  = read_be32(fbuf + o2); o2 += 4; }
                if (flags & 0x04 && o2 + 100 <= fsize) {
                    memcpy(meta->toc, fbuf + o2, 100);
                    meta->has_toc = true;
                    o2 += 100;
                }
                meta->is_vbr = (fid[0] == 'X');
            }
        }
        free(fbuf);
        ARG_UNUSED(tmp);
    }
    return true;
}

int mp3_meta_read(const char *path, mp3_meta_t *meta) {
    memset(meta, 0, sizeof(*meta));
    strncpy(meta->title,  path, META_STR_MAX - 1);
    strncpy(meta->artist, "Unknown", META_STR_MAX - 1);

    struct fs_file_t f;
    fs_file_t_init(&f);
    if (fs_open(&f, path, FS_O_READ) < 0) return -EIO;

    parse_id3v2(&f, meta);

    /* Estimate duration from file size if no Xing frame */
    struct fs_dirent ent;
    if (meta->total_frames == 0 && fs_stat(path, &ent) == 0) {
        uint32_t audio_bytes = ent.size > meta->audio_offset
                               ? ent.size - meta->audio_offset : ent.size;
        /* ~4 bytes/sample @ 128kbps, 1152 frames @ 44100 → ~208 frames/s */
        meta->duration_sec = audio_bytes / (128 * 1000 / 8);
    } else {
        meta->sample_rate  = meta->sample_rate ? meta->sample_rate : 44100;
        meta->duration_sec = meta->total_frames * 1152 / meta->sample_rate;
    }

    fs_close(&f);
    return 0;
}
