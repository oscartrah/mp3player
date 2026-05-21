#pragma once

#if defined(CONFIG_BOARD_STM32F769I_DISCO)
    #define ART_BUF_SIZE         (64 * 1024)
    #define READ_BUF_SIZE        (16 * 1024)
    #define PCM_QUEUE_DEPTH      8
    #define ART_DISPLAY_SIZE     200
    #define PLAYLIST_MAX_TRACKS  256
    #define MEM_SECTION          __attribute__((section(".sdram")))

#elif defined(CONFIG_BOARD_STM32F746G_DISCO)
    #define ART_BUF_SIZE         (48 * 1024)
    #define READ_BUF_SIZE        (16 * 1024)
    #define PCM_QUEUE_DEPTH      8
    #define ART_DISPLAY_SIZE     160
    #define PLAYLIST_MAX_TRACKS  256
    #define MEM_SECTION          __attribute__((section(".sdram")))

#elif defined(CONFIG_BOARD_MIMXRT1060_EVK)
    #define ART_BUF_SIZE         (64 * 1024)
    #define READ_BUF_SIZE        (16 * 1024)
    #define PCM_QUEUE_DEPTH      8
    #define ART_DISPLAY_SIZE     160
    #define PLAYLIST_MAX_TRACKS  256
    #define MEM_SECTION          __attribute__((section(".sdram")))

#elif defined(CONFIG_BOARD_LPCXPRESSO54628)
    #define ART_BUF_SIZE         (16 * 1024)
    #define READ_BUF_SIZE        (4  * 1024)
    #define PCM_QUEUE_DEPTH      4
    #define ART_DISPLAY_SIZE     96
    #define PLAYLIST_MAX_TRACKS  128
    #define MEM_SECTION

#elif defined(CONFIG_BOARD_EK_RA6M3G)
    #define ART_BUF_SIZE         (64 * 1024)
    #define READ_BUF_SIZE        (16 * 1024)
    #define PCM_QUEUE_DEPTH      8
    #define ART_DISPLAY_SIZE     200
    #define PLAYLIST_MAX_TRACKS  256
    #define MEM_SECTION          __attribute__((section(".sdram")))

#else
    #error "Unbekanntes Board — memory_cfg.h anpassen"
#endif
