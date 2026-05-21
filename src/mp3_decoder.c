#define MINIMP3_IMPLEMENTATION
#include <minimp3.h>
#include "mp3_decoder.h"
#include "player_state.h"
#include "audio_hal.h"
#include "biquad_eq.h"
#include "settings.h"
#include "playlist.h"
#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>
#include <string.h>

K_MSGQ_DEFINE(pcm_queue, sizeof(struct pcm_block), PCM_QUEUE_DEPTH, 4);

static uint8_t read_buf[READ_BUF_SIZE];
static biquad_eq_t s_beq;

static bool load_settings_eq(void) {
    app_settings_t cfg;
    if (settings_load(&cfg) == 0 && cfg.eq.enabled) {
        return true;
    }
    return false;
}

void decoder_thread(void *path_arg, void *p2, void *p3) {
    ARG_UNUSED(p2); ARG_UNUSED(p3);
    const char *path = (const char *)path_arg;

    struct fs_file_t f;
    fs_file_t_init(&f);
    if (fs_open(&f, path, FS_O_READ) < 0) return;

    mp3dec_t dec;
    mp3dec_init(&dec);

    app_settings_t cfg;
    if (settings_load(&cfg) == 0) {
        biquad_eq_configure(&s_beq, &cfg.eq, 44100);
        s_beq.enabled = cfg.eq.enabled;
    }

    uint8_t *buf = read_buf;
    int buf_used = 0;

    while (1) {
        /* Refill read buffer */
        int avail = READ_BUF_SIZE - buf_used;
        int got = fs_read(&f, buf + buf_used, avail);
        if (got <= 0 && buf_used == 0) break;
        if (got > 0) buf_used += got;

        mp3dec_frame_info_t info;
        struct pcm_block blk;
        int samples = mp3dec_decode_frame(&dec, buf, buf_used,
                                          blk.data, &info);
        if (info.frame_bytes == 0) break;

        buf_used -= info.frame_bytes;
        memmove(buf, buf + info.frame_bytes, buf_used);

        if (samples > 0) {
            blk.samples     = (uint16_t)(samples * info.channels);
            blk.channels    = (uint8_t)info.channels;
            blk.sample_rate = (uint32_t)info.hz;

            if (s_beq.enabled) {
                biquad_eq_process(&s_beq, blk.data, samples, blk.channels);
            }

            /* Update state sample_rate */
            k_mutex_lock(&state_mutex, K_FOREVER);
            g_state.sample_rate = info.hz;
            k_mutex_unlock(&state_mutex);

            k_msgq_put(&pcm_queue, &blk, K_FOREVER);
        }

        /* Check for stop command */
        struct player_command cmd;
        if (k_msgq_get(&cmd_queue, &cmd, K_NO_WAIT) == 0) {
            if (cmd.cmd == CMD_STOP || cmd.cmd == CMD_NEXT || cmd.cmd == CMD_PREV) {
                k_msgq_purge(&pcm_queue);
                break;
            }
        }
    }
    fs_close(&f);
}

void player_thread(void *p1, void *p2, void *p3) {
    ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);

    const struct device *i2s = DEVICE_DT_GET_ANY(zephyr_audio_i2s);

    struct pcm_block blk;

    while (1) {
        struct player_command cmd;
        if (k_msgq_get(&cmd_queue, &cmd, K_NO_WAIT) == 0) {
            switch (cmd.cmd) {
            case CMD_VOLUME:
                audio_set_volume((uint8_t)cmd.value);
                k_mutex_lock(&state_mutex, K_FOREVER);
                g_state.volume = (uint8_t)cmd.value;
                k_mutex_unlock(&state_mutex);
                break;
            case CMD_PAUSE:
                k_mutex_lock(&state_mutex, K_FOREVER);
                g_state.status = PLAYER_PAUSED;
                k_mutex_unlock(&state_mutex);
                audio_set_mute(true);
                /* Wait until resumed */
                while (1) {
                    k_sleep(K_MSEC(50));
                    if (k_msgq_get(&cmd_queue, &cmd, K_NO_WAIT) == 0) {
                        if (cmd.cmd == CMD_PLAY || cmd.cmd == CMD_STOP) break;
                    }
                }
                if (cmd.cmd == CMD_PLAY) {
                    audio_set_mute(false);
                    k_mutex_lock(&state_mutex, K_FOREVER);
                    g_state.status = PLAYER_PLAYING;
                    k_mutex_unlock(&state_mutex);
                }
                break;
            default:
                break;
            }
        }

        if (k_msgq_get(&pcm_queue, &blk, K_MSEC(100)) == 0) {
            k_mutex_lock(&state_mutex, K_FOREVER);
            player_status_t st = g_state.status;
            k_mutex_unlock(&state_mutex);

            if (st == PLAYER_PLAYING && i2s) {
                /* Ship samples to I2S — board-specific details handled by Zephyr driver */
                const struct device *i2s_dev = i2s;
                ARG_UNUSED(i2s_dev);
                /* In a real implementation: i2s_buf_write(i2s_dev, blk.data, blk.samples * 2) */
            }

            /* Update elapsed time (1152 samples per MP3 frame, stereo) */
            if (blk.sample_rate > 0) {
                uint32_t frame_ms = (1152 * 1000) / blk.sample_rate;
                k_mutex_lock(&state_mutex, K_FOREVER);
                g_state.elapsed_sec += frame_ms / 1000;
                k_mutex_unlock(&state_mutex);
            }
        }
    }
}
