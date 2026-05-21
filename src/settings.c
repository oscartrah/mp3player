#include "settings.h"
#include <zephyr/fs/nvs.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/kernel.h>
#include <string.h>

#define NVS_PARTITION        storage_partition
#define NVS_PARTITION_DEVICE FIXED_PARTITION_DEVICE(NVS_PARTITION)
#define NVS_PARTITION_OFFSET FIXED_PARTITION_OFFSET(NVS_PARTITION)

static struct nvs_fs s_nvs;
static bool s_ready = false;

int settings_init(void) {
    struct flash_pages_info info;
    s_nvs.flash_device = NVS_PARTITION_DEVICE;
    if (!device_is_ready(s_nvs.flash_device)) return -ENODEV;

    s_nvs.offset = NVS_PARTITION_OFFSET;
    int rc = flash_get_page_info_by_offs(s_nvs.flash_device,
                                         s_nvs.offset, &info);
    if (rc) return rc;
    s_nvs.sector_size  = info.size;
    s_nvs.sector_count = 4U;
    rc = nvs_mount(&s_nvs);
    if (rc == 0) s_ready = true;
    return rc;
}

int settings_load(app_settings_t *out) {
    app_settings_t def = SETTINGS_DEFAULT;
    *out = def;
    if (!s_ready) return -ENODEV;

    nvs_read(&s_nvs, NVS_KEY_VOLUME,     &out->volume,       sizeof(out->volume));
    nvs_read(&s_nvs, NVS_KEY_SHUFFLE,    &out->shuffle,      sizeof(out->shuffle));
    nvs_read(&s_nvs, NVS_KEY_LAST_TRACK, &out->last_track,   sizeof(out->last_track));
    nvs_read(&s_nvs, NVS_KEY_LAST_POS,   &out->last_pos_sec, sizeof(out->last_pos_sec));
    nvs_read(&s_nvs, NVS_KEY_EQ,         &out->eq,           sizeof(out->eq));
    return 0;
}

int settings_save(const app_settings_t *s) {
    if (!s_ready) return -ENODEV;
    nvs_write(&s_nvs, NVS_KEY_VOLUME,     &s->volume,       sizeof(s->volume));
    nvs_write(&s_nvs, NVS_KEY_SHUFFLE,    &s->shuffle,      sizeof(s->shuffle));
    nvs_write(&s_nvs, NVS_KEY_LAST_TRACK, &s->last_track,   sizeof(s->last_track));
    nvs_write(&s_nvs, NVS_KEY_LAST_POS,   &s->last_pos_sec, sizeof(s->last_pos_sec));
    nvs_write(&s_nvs, NVS_KEY_EQ,         &s->eq,           sizeof(s->eq));
    return 0;
}

int settings_save_volume(uint8_t vol) {
    if (!s_ready) return -ENODEV;
    return nvs_write(&s_nvs, NVS_KEY_VOLUME, &vol, sizeof(vol)) > 0 ? 0 : -EIO;
}

int settings_save_shuffle(bool on) {
    if (!s_ready) return -ENODEV;
    return nvs_write(&s_nvs, NVS_KEY_SHUFFLE, &on, sizeof(on)) > 0 ? 0 : -EIO;
}

int settings_save_position(uint16_t track, uint32_t pos_sec) {
    if (!s_ready) return -ENODEV;
    nvs_write(&s_nvs, NVS_KEY_LAST_TRACK, &track,   sizeof(track));
    nvs_write(&s_nvs, NVS_KEY_LAST_POS,   &pos_sec, sizeof(pos_sec));
    return 0;
}

int settings_save_eq(const eq_config_t *eq) {
    if (!s_ready) return -ENODEV;
    return nvs_write(&s_nvs, NVS_KEY_EQ, eq, sizeof(*eq)) > 0 ? 0 : -EIO;
}
