#pragma once
#include <lvgl.h>

typedef struct {
    uint16_t    width;
    uint16_t    height;
    uint16_t    dpi;
    const char *name;
} display_profile_t;

#if defined(CONFIG_BOARD_STM32F769I_DISCO)
    #define DISPLAY_PROFILE { .width=800, .height=480, .dpi=233, .name="F769" }
#elif defined(CONFIG_BOARD_STM32F746G_DISCO)
    #define DISPLAY_PROFILE { .width=480, .height=272, .dpi=128, .name="F746" }
#elif defined(CONFIG_BOARD_MIMXRT1060_EVK)
    #define DISPLAY_PROFILE { .width=480, .height=272, .dpi=128, .name="RT1060" }
#elif defined(CONFIG_BOARD_LPCXPRESSO54628)
    #define DISPLAY_PROFILE { .width=480, .height=272, .dpi=128, .name="LPC54628" }
#elif defined(CONFIG_BOARD_EK_RA6M3G)
    #define DISPLAY_PROFILE { .width=480, .height=272, .dpi=128, .name="RA6M3G" }
#else
    #define DISPLAY_PROFILE { .width=480, .height=272, .dpi=128, .name="Unknown" }
#endif

extern display_profile_t g_display;
void display_profile_init(void);
