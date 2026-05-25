#define MINIMP3_IMPLEMENTATION
#include <minimp3.h>
#include "mp3_decoder.h"
#include "memory_cfg.h"
#include "player_state.h"
#include "audio_hal.h"
#include "biquad_eq.h"
#include "settings.h"
#include "playlist.h"
#include <zephyr/fs/fs.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/kernel.h>
#include <string.h>

/* PCM queue: backing buffer in SDRAM to avoid internal RAM overflow */
static char pcm_queue_buf[sizeof(struct pcm_block) * PCM_QUEUE_DEPTH] MEM_SECTION;
struct k_msgq pcm_queue;

static int pcm_queue_init(void)
{
    k_msgq_init(&pcm_queue, pcm_queue_buf,
                sizeof(struct pcm_block), PCM_QUEUE_DEPTH);
    return 0;
}
SYS_INIT(pcm_queue_init, POST_KERNEL, 99);

/* ── Board-specific I2S device ──────────────────────────────── */
#if defined(CONFIG_BOARD_STM32F769I_DISCO)
#  define PLAYER_I2S_NODE  DT_NODELABEL(sai2_a)
#elif defined(CONFIG_BOARD_STM32F746G_DISCO)
#  define PLAYER_I2S_NODE  DT_NODELABEL(sai1_a)
#elif defined(CONFIG_BOARD_MIMXRT1060_EVK)
#  define PLAYER_I2S_NODE  DT_NODELABEL(sai1)
#elif defined(CONFIG_BOARD_LPCXPRESSO54628)
#  define PLAYER_I2S_NODE  DT_NODELABEL(flexcomm6)
#elif defined(CONFIG_BOARD_EK_RA6M3G)
#  define PLAYER_I2S_NODE  DT_NODELABEL(ssi0)
#endif

/* TX memory slab: PCM_QUEUE_DEPTH blocks × PCM_BLOCK_SIZE samples × 2 bytes */
K_MEM_SLAB_DEFINE(i2s_tx_slab, PCM_BLOCK_SIZE * 2U, PCM_QUEUE_DEPTH, 4);

static uint32_t s_i2s_rate = 0;
static uint8_t  s_i2s_ch   = 0;

static int i2s_tx_configure(const struct device *dev,
                             uint32_t rate, uint8_t channels)
{
    if (s_i2s_rate == rate && s_i2s_ch == channels) return 0;
    struct i2s_config cfg = {
        .word_size      = 16U,
        .channels       = channels,
        .format         = I2S_FMT_DATA_FORMAT_I2S,
        .options        = I2S_OPT_FRAME_CLK_TARGET | I2S_OPT_BIT_CLK_TARGET,
        .frame_clk_freq = rate,
        .mem_slab       = &i2s_tx_slab,
        .block_size     = PCM_BLOCK_SIZE * 2U,
        .timeout        = 2000,
    };
    int rc = i2s_configure(dev, I2S_DIR_TX, &cfg);
    if (rc == 0) { s_i2s_rate = rate; s_i2s_ch = channels; }
    return rc;
}

/* ── Decoder thread ─────────────────────────────────────────── */
static uint8_t    read_buf[READ_BUF_SIZE] MEM_SECTION;
static biquad_eq_t s_beq;

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
        int avail = READ_BUF_SIZE - buf_used;
        int got   = fs_read(&f, buf + buf_used, avail);
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

            k_mutex_lock(&state_mutex, K_FOREVER);
            g_state.sample_rate = info.hz;
            k_mutex_unlock(&state_mutex);

            k_msgq_put(&pcm_queue, &blk, K_FOREVER);
        }

        struct player_command cmd;
        if (k_msgq_get(&cmd_queue, &cmd, K_NO_WAIT) == 0) {
            if (cmd.cmd == CMD_STOP || cmd.cmd == CMD_NEXT ||
                cmd.cmd == CMD_PREV) {
                k_msgq_purge(&pcm_queue);
                break;
            }
        }
    }
    fs_close(&f);
}

/* ── Player thread ──────────────────────────────────────────── */
void player_thread(void *p1, void *p2, void *p3) {
    ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);

#ifdef PLAYER_I2S_NODE
    const struct device *i2s_dev = DEVICE_DT_GET(PLAYER_I2S_NODE);
    if (!device_is_ready(i2s_dev)) i2s_dev = NULL;
#else
    const struct device *i2s_dev = NULL;
#endif

    bool     running    = false;
    uint32_t elapsed_ms = 0;
    struct pcm_block blk;

    while (1) {
        /* Handle control commands */
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
                if (i2s_dev && running) {
                    i2s_trigger(i2s_dev, I2S_DIR_TX, I2S_TRIGGER_DRAIN);
                    running = false;
                }
                k_mutex_lock(&state_mutex, K_FOREVER);
                g_state.status = PLAYER_PAUSED;
                k_mutex_unlock(&state_mutex);
                audio_set_mute(true);
                /* Block until CMD_PLAY or CMD_STOP arrives */
                while (1) {
                    k_sleep(K_MSEC(50));
                    if (k_msgq_get(&cmd_queue, &cmd, K_NO_WAIT) == 0 &&
                        (cmd.cmd == CMD_PLAY || cmd.cmd == CMD_STOP)) break;
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

        if (k_msgq_get(&pcm_queue, &blk, K_MSEC(100)) != 0) continue;

        k_mutex_lock(&state_mutex, K_FOREVER);
        player_status_t st = g_state.status;
        k_mutex_unlock(&state_mutex);

        if (st == PLAYER_PLAYING && i2s_dev) {
            /* Reconfigure I2S if format changed (e.g. VBR rate switch) */
            if (blk.sample_rate != s_i2s_rate || blk.channels != s_i2s_ch) {
                if (running) {
                    i2s_trigger(i2s_dev, I2S_DIR_TX, I2S_TRIGGER_DROP);
                    running = false;
                }
                i2s_tx_configure(i2s_dev, blk.sample_rate, blk.channels);
            }

            /* Write PCM into I2S TX slab — driver returns block to slab after DMA */
            void *tx_buf;
            if (k_mem_slab_alloc(&i2s_tx_slab, &tx_buf, K_MSEC(100)) == 0) {
                size_t sz = blk.samples * sizeof(int16_t);
                memcpy(tx_buf, blk.data, sz);
                if (i2s_buf_write(i2s_dev, tx_buf, sz) == 0) {
                    if (!running) {
                        i2s_trigger(i2s_dev, I2S_DIR_TX, I2S_TRIGGER_START);
                        running = true;
                    }
                } else {
                    k_mem_slab_free(&i2s_tx_slab, &tx_buf);
                }
            }
        }

        /* Accumulate ms to avoid integer truncation (26 ms/frame at 44100 Hz) */
        if (blk.sample_rate > 0 && st == PLAYER_PLAYING) {
            elapsed_ms += (1152U * 1000U) / blk.sample_rate;
            if (elapsed_ms >= 1000U) {
                k_mutex_lock(&state_mutex, K_FOREVER);
                g_state.elapsed_sec += elapsed_ms / 1000U;
                k_mutex_unlock(&state_mutex);
                elapsed_ms %= 1000U;
            }
        }
    }
}
