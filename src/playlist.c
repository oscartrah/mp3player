#include "playlist.h"
#include "mp3_meta.h"
#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>
#include <string.h>
#include <stdlib.h>

playlist_t g_playlist;

static void build_path(char *dst, const char *root, const char *name, size_t max) {
    size_t rlen = strlen(root);
    size_t nlen = strlen(name);
    if (rlen + nlen + 2 > max) return;
    memcpy(dst, root, rlen);
    if (root[rlen - 1] != '/') dst[rlen++] = '/';
    memcpy(dst + rlen, name, nlen + 1);
}

static bool is_mp3(const char *name) {
    size_t n = strlen(name);
    return n > 4 && (name[n-4] == '.' &&
           (name[n-3] == 'm' || name[n-3] == 'M') &&
           (name[n-2] == 'p' || name[n-2] == 'P') &&
           (name[n-1] == '3'));
}

int playlist_scan(const char *root, playlist_t *pl) {
    memset(pl, 0, sizeof(*pl));
    pl->current = -1;

    struct fs_dir_t dir;
    fs_dir_t_init(&dir);
    if (fs_opendir(&dir, root) < 0) return -ENOENT;

    struct fs_dirent ent;
    while (fs_readdir(&dir, &ent) == 0 && ent.name[0] != '\0') {
        if (pl->count >= PLAYLIST_MAX_TRACKS) break;
        if (ent.type != FS_DIR_ENTRY_FILE) continue;
        if (!is_mp3(ent.name)) continue;

        playlist_entry_t *e = &pl->entries[pl->count];
        build_path(e->path, root, ent.name, PLAYLIST_PATH_MAX);

        mp3_meta_t meta;
        if (mp3_meta_read(e->path, &meta) == 0) {
            strncpy(e->title,  meta.title[0]  ? meta.title  : ent.name, META_STR_MAX - 1);
            strncpy(e->artist, meta.artist[0] ? meta.artist : "Unknown", META_STR_MAX - 1);
            e->duration_sec = meta.duration_sec;
            e->track_nr     = meta.track_nr;
        } else {
            strncpy(e->title, ent.name, META_STR_MAX - 1);
        }
        pl->count++;
    }
    fs_closedir(&dir);

    playlist_sort(pl);
    if (pl->count > 0) pl->current = 0;
    return (int)pl->count;
}

static int cmp_entry(const void *a, const void *b) {
    const playlist_entry_t *ea = (const playlist_entry_t *)a;
    const playlist_entry_t *eb = (const playlist_entry_t *)b;
    if (ea->track_nr && eb->track_nr) return (int)ea->track_nr - (int)eb->track_nr;
    return strcmp(ea->title, eb->title);
}

void playlist_sort(playlist_t *pl) {
    if (pl->count > 1) {
        qsort(pl->entries, pl->count, sizeof(playlist_entry_t), cmp_entry);
    }
}

int16_t playlist_next(const playlist_t *pl) {
    if (pl->count == 0) return -1;
    if (pl->shuffle) {
        int16_t cur = pl->current;
        for (uint16_t i = 0; i < pl->count; i++) {
            if (pl->shuffle_order[i] == cur) {
                return (i + 1 < pl->count) ? pl->shuffle_order[i + 1] : pl->shuffle_order[0];
            }
        }
    }
    int16_t n = pl->current + 1;
    return (n >= (int16_t)pl->count) ? 0 : n;
}

int16_t playlist_prev(const playlist_t *pl) {
    if (pl->count == 0) return -1;
    if (pl->shuffle) {
        int16_t cur = pl->current;
        for (uint16_t i = 0; i < pl->count; i++) {
            if (pl->shuffle_order[i] == cur) {
                return (i > 0) ? pl->shuffle_order[i - 1]
                               : pl->shuffle_order[pl->count - 1];
            }
        }
    }
    int16_t n = pl->current - 1;
    return (n < 0) ? (int16_t)(pl->count - 1) : n;
}

static void fisher_yates(playlist_t *pl) {
    for (uint16_t i = 0; i < pl->count; i++) pl->shuffle_order[i] = i;
    for (uint16_t i = pl->count - 1; i > 0; i--) {
        uint16_t j = (uint16_t)(sys_rand32_get() % (i + 1));
        uint16_t tmp = pl->shuffle_order[i];
        pl->shuffle_order[i] = pl->shuffle_order[j];
        pl->shuffle_order[j] = tmp;
    }
}

void playlist_shuffle_enable(playlist_t *pl) {
    pl->shuffle = true;
    fisher_yates(pl);
}

void playlist_shuffle_disable(playlist_t *pl) {
    pl->shuffle = false;
}

void playlist_toggle_shuffle(playlist_t *pl) {
    if (pl->shuffle) playlist_shuffle_disable(pl);
    else             playlist_shuffle_enable(pl);
}
