#include "audio_hal.h"
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>

#define WM8960_ADDR 0x1A

static const struct device *s_i2c;

/* WM8960 uses 9-bit registers packed as [reg<<1 | bit8_of_val, val[7:0]] */
static int wm8960_write(uint8_t reg, uint16_t val) {
    uint8_t buf[2] = {
        (uint8_t)((reg << 1) | ((val >> 8) & 0x01)),
        (uint8_t)(val & 0xFF)
    };
    return i2c_write(s_i2c, buf, 2, WM8960_ADDR);
}

static int wm8960_init(void) {
#if DT_NODE_EXISTS(DT_NODELABEL(wm8960))
    s_i2c = DEVICE_DT_GET(DT_BUS(DT_NODELABEL(wm8960)));
#else
    s_i2c = NULL;
#endif
    if (!s_i2c || !device_is_ready(s_i2c)) return -ENODEV;

    /* Reset */
    wm8960_write(0x0F, 0x000);
    k_msleep(10);

    /* Power: VMID, VREF, AINL, AINR, ADCL, ADCR */
    wm8960_write(0x19, 0x1FC);
    wm8960_write(0x1A, 0x1F8); /* DAC power + headphone driver */
    wm8960_write(0x2F, 0x00C); /* LOUT1_PU, ROUT1_PU */

    /* Clocking: use MCLK directly, SR=44100 */
    wm8960_write(0x04, 0x000); /* SYSCLK = MCLK */
    wm8960_write(0x08, 0x000); /* SR=44100 assumed from MCLK */

    /* DAC format: I2S, 16-bit, slave */
    wm8960_write(0x07, 0x002); /* I2S format, 16-bit */

    /* DAC → headphone output */
    wm8960_write(0x22, 0x100); /* DACL → left mixer */
    wm8960_write(0x25, 0x100); /* DACR → right mixer */

    /* Default headphone volume */
    wm8960_write(0x02, 0x179); /* LOUT1VOL with update bit */
    wm8960_write(0x03, 0x179); /* ROUT1VOL with update bit */

    /* Unmute DAC */
    wm8960_write(0x05, 0x000);

    return 0;
}

static int wm8960_set_volume(uint8_t vol_pct) {
    /* 0–100% → 0x30–0x7F headphone volume register */
    uint16_t vol = 0x30 + (uint16_t)(vol_pct * 0x4F / 100);
    wm8960_write(0x02, 0x100 | vol);
    wm8960_write(0x03, 0x100 | vol);
    return 0;
}

static int wm8960_set_mute(bool mute) {
    wm8960_write(0x05, mute ? 0x008 : 0x000);
    return 0;
}

static int wm8960_set_eq(const eq_config_t *eq) {
    /* WM8960 has no hardware EQ — SW EQ applied in biquad_eq */
    ARG_UNUSED(eq);
    return 0;
}

static int wm8960_set_drc(const drc_config_t *drc) {
    if (!drc->enabled) {
        wm8960_write(0x10, 0x000);
        return 0;
    }
    wm8960_write(0x10, 0x1C0); /* ALC enable, both channels */
    return 0;
}

static int wm8960_set_samplerate(uint32_t hz) {
    ARG_UNUSED(hz);
    /* Full PLL programming omitted — MCLK assumed matching */
    return 0;
}

const audio_hal_api_t audio_hal_wm8960 = {
    .name           = "WM8960",
    .init           = wm8960_init,
    .set_volume     = wm8960_set_volume,
    .set_mute       = wm8960_set_mute,
    .set_eq         = wm8960_set_eq,
    .set_drc        = wm8960_set_drc,
    .set_samplerate = wm8960_set_samplerate,
    .hw_eq          = false,
    .hw_drc         = false,
    .eq_bands       = 0,
};
