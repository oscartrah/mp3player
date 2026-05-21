#include "player_state.h"

K_MUTEX_DEFINE(state_mutex);
K_MSGQ_DEFINE(cmd_queue, sizeof(struct player_command), 8, 4);

struct player_state g_state = {
    .track_name  = "No Track",
    .artist      = "Unknown",
    .elapsed_sec = 0,
    .total_sec   = 0,
    .sample_rate = 44100,
    .volume      = 75,
    .status      = PLAYER_STOPPED,
};
