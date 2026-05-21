#include "audio_hal.h"
#include <zephyr/kernel.h>

const audio_hal_api_t *g_audio_hal = NULL;

int audio_hal_init(void) {
#if defined(CONFIG_AUDIO_HAL_WM8994)
    g_audio_hal = &audio_hal_wm8994;
#elif defined(CONFIG_AUDIO_HAL_WM8960)
    g_audio_hal = &audio_hal_wm8960;
#elif defined(CONFIG_AUDIO_HAL_WM8904)
    g_audio_hal = &audio_hal_wm8904;
#elif defined(CONFIG_AUDIO_HAL_DA7212)
    g_audio_hal = &audio_hal_da7212;
#else
    #error "Kein Audio HAL konfiguriert — prj.conf prüfen"
#endif
    return g_audio_hal->init ? g_audio_hal->init() : 0;
}
