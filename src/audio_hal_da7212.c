#include "audio_hal.h"
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>

#define DA7212_ADDR 0x1A

static const struct device *s_i2c;

static int da7212_write(uint8_t reg, uint8_t val) {
    uint8_t buf[2] = { reg, val };
    return i2c_write(s_i2c, buf, 2, DA7212_ADDR);
}

static int da7212_init(void) {
#if DT_NODE_EXISTS(DT_NODELABEL(da7212))
    s_i2c = DEVICE_DT_GET(DT_BUS(DT_NODELABEL(da7212)));
#else
    s_i2c = NULL;
#endif
    if (!s_i2c || !device_is_ready(s_i2c)) return -ENODEV;

    /* Keep-alive / reset */
    da7212_write(0xFD, 0x01); /* CIF_CTRL: soft reset */
    k_msleep(10);

    /* System clock: MCLK → system */
    da7212_write(0x22, 0x01); /* PLL_CTRL: PLL off, MCLK direct */

    /* Audio interface: I2S, 16-bit, slave */
    da7212_write(0x27, 0x08); /* DAI_CTRL: I2S, 16-bit */
    da7212_write(0x28, 0x00); /* DAI_CLK_MODE: slave */

    /* Power up DAC & HP amplifier */
    da7212_write(0x43, 0x80); /* DAC_L_CTRL: DAC_L_EN */
    da7212_write(0x44, 0x80); /* DAC_R_CTRL: DAC_R_EN */
    da7212_write(0x45, 0x00); /* Unmute */

    /* Mixer: DAC → HP */
    da7212_write(0x69, 0x80); /* HP_L_CTRL: HP_L_AMP_EN */
    da7212_write(0x6A, 0x80); /* HP_R_CTRL: HP_R_AMP_EN */

    /* Default HP volume */
    da7212_write(0x6B, 0x37); /* HP_L_GAIN */
    da7212_write(0x6C, 0x37); /* HP_R_GAIN */

    /* System power */
    da7212_write(0xFD, 0x00); /* normal operation */

    return 0;
}

static int da7212_set_volume(uint8_t vol_pct) {
    /* 0–100% → 0x00–0x3F HP gain */
    uint8_t gain = (uint8_t)(vol_pct * 0x3F / 100);
    da7212_write(0x6B, gain);
    da7212_write(0x6C, gain);
    return 0;
}

static int da7212_set_mute(bool mute) {
    da7212_write(0x45, mute ? 0x08 : 0x00); /* DAC_FILTERS1: MUTE bit */
    return 0;
}

static int da7212_set_eq(const eq_config_t *eq) {
    ARG_UNUSED(eq);
    return 0;
}

static int da7212_set_drc(const drc_config_t *drc) {
    if (!drc->enabled) {
        da7212_write(0x6E, 0x00); /* DRC_CTRL: disable */
        return 0;
    }
    da7212_write(0x6E, 0x28); /* DRC enable, default threshold */
    return 0;
}

static int da7212_set_samplerate(uint32_t hz) {
    uint8_t sr;
    switch (hz) {
    case 8000:  sr = 0x01; break;
    case 16000: sr = 0x03; break;
    case 32000: sr = 0x07; break;
    case 44100: sr = 0x0A; break;
    case 48000: sr = 0x0B; break;
    default:    sr = 0x0A; break;
    }
    da7212_write(0x22, sr);
    return 0;
}

const audio_hal_api_t audio_hal_da7212 = {
    .name           = "DA7212",
    .init           = da7212_init,
    .set_volume     = da7212_set_volume,
    .set_mute       = da7212_set_mute,
    .set_eq         = da7212_set_eq,
    .set_drc        = da7212_set_drc,
    .set_samplerate = da7212_set_samplerate,
    .hw_eq          = false,
    .hw_drc         = true,
    .eq_bands       = 0,
};
