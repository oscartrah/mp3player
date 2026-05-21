#include <zephyr/kernel.h>
#include <zephyr/drivers/display.h>
#include <lvgl.h>

#include "player_state.h"
#include "settings.h"
#include "playlist.h"
#include "mp3_decoder.h"
#include "audio_hal.h"
#include "ui.h"
#include "button_input.h"

/* ── Stack-Definitionen ───────────────────────── */
#define DECODER_STACK  16384
#define PLAYER_STACK    4096
#define UI_STACK        8192
#define SAVE_STACK      2048

K_THREAD_STACK_DEFINE(decoder_stack, DECODER_STACK);
K_THREAD_STACK_DEFINE(player_stack,  PLAYER_STACK);
K_THREAD_STACK_DEFINE(ui_stack,      UI_STACK);
K_THREAD_STACK_DEFINE(save_stack,    SAVE_STACK);

static struct k_thread decoder_tid;
static struct k_thread player_tid;
static struct k_thread ui_tid;
static struct k_thread save_tid;

/* ── Save-Thread: Position alle 5 s persistieren  */
void save_thread(void *p1, void *p2, void *p3) {
    ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);
    uint32_t last_saved = 0;
    while (1) {
        k_sleep(K_SECONDS(5));
        k_mutex_lock(&state_mutex, K_FOREVER);
        uint32_t pos   = g_state.elapsed_sec;
        uint16_t track = (uint16_t)g_playlist.current;
        k_mutex_unlock(&state_mutex);
        if (pos != last_saved) {
            settings_save_position(track, pos);
            last_saved = pos;
        }
    }
}

/* ── UI-Thread ────────────────────────────────── */
void ui_thread(void *p1, void *p2, void *p3) {
    ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);
    const struct device *display =
        DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
    display_blanking_off(display);
    lv_init();
    ui_init();
    while (1) {
        ui_update();
        lv_task_handler();
        k_sleep(K_MSEC(16));
    }
}

int main(void) {
    /* Einstellungen laden */
    settings_init();
    app_settings_t cfg;
    settings_load(&cfg);

    /* Audio HAL initialisieren */
    audio_hal_init();
    audio_set_volume(cfg.volume);

    /* State befüllen */
    k_mutex_lock(&state_mutex, K_FOREVER);
    g_state.volume = cfg.volume;
    k_mutex_unlock(&state_mutex);

    /* Playlist scannen */
    playlist_scan("/SD:/Music", &g_playlist);
    if (cfg.last_track < g_playlist.count) {
        g_playlist.current = (int16_t)cfg.last_track;
    }
    if (cfg.shuffle) {
        playlist_shuffle_enable(&g_playlist);
    }

    /* Button-Input (LPC54628 ohne Touch) */
    button_input_init();

    /* Threads starten */
    /* Priorität 3: Player (I2S darf nicht abreißen)  */
    k_thread_create(&player_tid, player_stack, PLAYER_STACK,
                    player_thread, NULL, NULL, NULL,
                    K_PRIO_PREEMPT(3), 0, K_NO_WAIT);

    /* Priorität 5: Decoder */
    if (g_playlist.count > 0 && g_playlist.current >= 0) {
        k_thread_create(&decoder_tid, decoder_stack, DECODER_STACK,
                        decoder_thread,
                        g_playlist.entries[g_playlist.current].path,
                        NULL, NULL,
                        K_PRIO_PREEMPT(5), 0, K_NO_WAIT);
    }

    /* Priorität 7: UI */
    k_thread_create(&ui_tid, ui_stack, UI_STACK,
                    ui_thread, NULL, NULL, NULL,
                    K_PRIO_PREEMPT(7), 0, K_NO_WAIT);

    /* Priorität 9: Save */
    k_thread_create(&save_tid, save_stack, SAVE_STACK,
                    save_thread, NULL, NULL, NULL,
                    K_PRIO_PREEMPT(9), 0, K_NO_WAIT);

    return 0;
}
