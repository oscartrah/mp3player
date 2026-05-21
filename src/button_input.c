#include "button_input.h"
#include "player_state.h"
#include "playlist.h"
#include <zephyr/kernel.h>

#if defined(CONFIG_BOARD_LPCXPRESSO54628)
#include <zephyr/drivers/gpio.h>

#define BTN_DEBOUNCE_MS 50

static const struct gpio_dt_spec btns[] = {
    GPIO_DT_SPEC_GET(DT_NODELABEL(btn_play_pause), gpios),
    GPIO_DT_SPEC_GET(DT_NODELABEL(btn_next),       gpios),
    GPIO_DT_SPEC_GET(DT_NODELABEL(btn_prev),       gpios),
    GPIO_DT_SPEC_GET(DT_NODELABEL(btn_vol_up),     gpios),
    GPIO_DT_SPEC_GET(DT_NODELABEL(btn_vol_down),   gpios),
};

static struct gpio_callback btn_cb[ARRAY_SIZE(btns)];

static void on_btn_press(const struct device *dev,
                          struct gpio_callback *cb,
                          uint32_t pins)
{
    static int64_t last_press[ARRAY_SIZE(btns)] = {0};
    int64_t now = k_uptime_get();
    struct player_command cmd = {0};

    for (uint8_t i = 0; i < ARRAY_SIZE(btns); i++) {
        if (!(pins & BIT(btns[i].pin))) continue;
        if (now - last_press[i] < BTN_DEBOUNCE_MS) continue;
        last_press[i] = now;

        switch (i) {
        case 0: {
            k_mutex_lock(&state_mutex, K_FOREVER);
            bool playing = (g_state.status == PLAYER_PLAYING);
            k_mutex_unlock(&state_mutex);
            cmd.cmd = playing ? CMD_PAUSE : CMD_PLAY;
            break;
        }
        case 1: cmd.cmd = CMD_NEXT; break;
        case 2: cmd.cmd = CMD_PREV; break;
        case 3: {
            k_mutex_lock(&state_mutex, K_FOREVER);
            cmd.cmd   = CMD_VOLUME;
            cmd.value = MIN(100, g_state.volume + 5);
            k_mutex_unlock(&state_mutex);
            break;
        }
        case 4: {
            k_mutex_lock(&state_mutex, K_FOREVER);
            cmd.cmd   = CMD_VOLUME;
            cmd.value = MAX(0, (int)g_state.volume - 5);
            k_mutex_unlock(&state_mutex);
            break;
        }
        }
        k_msgq_put(&cmd_queue, &cmd, K_NO_WAIT);
    }
}

int button_input_init(void) {
    for (uint8_t i = 0; i < ARRAY_SIZE(btns); i++) {
        gpio_pin_configure_dt(&btns[i], GPIO_INPUT);
        gpio_init_callback(&btn_cb[i], on_btn_press,
                           BIT(btns[i].pin));
        gpio_add_callback(btns[i].port, &btn_cb[i]);
        gpio_pin_interrupt_configure_dt(&btns[i],
                                        GPIO_INT_EDGE_TO_ACTIVE);
    }
    return 0;
}

#else

/* Kein Button-Input auf anderen Boards */
int button_input_init(void) { return 0; }

#endif /* CONFIG_BOARD_LPCXPRESSO54628 */
