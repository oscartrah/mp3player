#pragma once
#include <zephyr/kernel.h>
#include <stdint.h>
#include <stdbool.h>

#define TRACK_NAME_MAX   64
#define PLAYLIST_PATH_MAX 128

typedef enum {
    PLAYER_STOPPED,
    PLAYER_PLAYING,
    PLAYER_PAUSED,
} player_status_t;

typedef enum {
    CMD_PLAY,
    CMD_PAUSE,
    CMD_STOP,
    CMD_NEXT,
    CMD_PREV,
    CMD_VOLUME,
    CMD_SEEK,
    CMD_PLAY_PATH,
} player_cmd_t;

struct player_command {
    player_cmd_t cmd;
    int32_t      value;
    char         path[PLAYLIST_PATH_MAX];
};

struct player_state {
    char            track_name[TRACK_NAME_MAX];
    char            artist[TRACK_NAME_MAX];
    uint32_t        elapsed_sec;
    uint32_t        total_sec;
    uint32_t        sample_rate;
    uint8_t         volume;
    player_status_t status;
};

extern struct k_mutex      state_mutex;
extern struct player_state g_state;
extern struct k_msgq       cmd_queue;
