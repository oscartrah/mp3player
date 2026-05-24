#include "audio_hal.h"
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>

#define WM8994_ADDR  0x1A

static const struct device *s_i2c;

static int wm8994_write(uint16_t reg, uint16_t val) {
    uint8_t buf[4] = {
        (uint8_t)(reg >> 8), (uint8_t)(reg & 0xFF),
        (uint8_t)(val >> 8), (uint8_t)(val & 0xFF)
    };
    return i2c_write(s_i2c, buf, 4, WM8994_ADDR);
}

static int wm8994_init(void) {
#if DT_NODE_EXISTS(DT_NODELABEL(wm8994))
    s_i2c = DEVICE_DT_GET(DT_BUS(DT_NODELABEL(wm8994)));
#else
    s_i2c = NULL;
#endif
    if (!s_i2c || !device_is_ready(s_i2c)) return -ENODEV;

    /* Software reset */
    wm8994_write(0x0000, 0x0000);
    k_msleep(10);

    /* Power management: enable VMID, bias, left/right DAC */
    wm8994_write(0x0001, 0x0003); /* VMID_SEL=01, BIAS_ENA=1 */
    wm8994_write(0x0002, 0x6000); /* DACL_ENA, DACR_ENA */
    wm8994_write(0x0003, 0x0300); /* MIXOUTL_ENA, MIXOUTR_ENA */
    wm8994_write(0x0004, 0x2002); /* AIF1ADC1L_ENA, ADCL_ENA */
    wm8994_write(0x0005, 0x0303); /* AIF1DAC1L_ENA, AIF1DAC1R_ENA */

    /* AIF1 format: I2S, 16-bit, BCLK slave */
    wm8994_write(0x0300, 0x4010); /* AIF1_FMT=10 (I2S), AIF1_WL=00 (16-bit) */
    wm8994_write(0x0302, 0x0000); /* AIF1 BCLK slave */

    /* Clocking: FLL not configured here — assumed external MCLK */
    wm8994_write(0x0200, 0x0001); /* AIF1CLK_ENA */
    wm8994_write(0x0208, 0x000A); /* DSP1CLK_ENA, SYSCLK_SRC=AIF1CLK */

    /* DAC → mixer → headphone output */
    wm8994_write(0x0601, 0x0001); /* DAC1L → MIXOUTL */
    wm8994_write(0x0602, 0x0001); /* DAC1R → MIXOUTR */
    wm8994_write(0x0060, 0x0022); /* HPOUT1L_ENA, HPOUT1R_ENA */

    /* Default volume: 0 dB on DAC, -12 dB on headphone */
    wm8994_write(0x0610, 0x01C0); /* DAC1L_VOL=0xC0 (0 dB) */
    wm8994_write(0x0611, 0x01C0); /* DAC1R_VOL=0xC0 */
    wm8994_write(0x001C, 0x016F); /* HPOUT1L_VOL */
    wm8994_write(0x001D, 0x016F); /* HPOUT1R_VOL */

    return 0;
}

static int wm8994_set_volume(uint8_t vol_pct) {
    /* Map 0–100% → 0x00–0xFF DAC digital volume */
    uint16_t dac_vol = (uint16_t)(vol_pct * 255 / 100);
    wm8994_write(0x0610, 0x0100 | dac_vol);
    wm8994_write(0x0611, 0x0100 | dac_vol);
    return 0;
}

static int wm8994_set_mute(bool mute) {
    wm8994_write(0x0420, mute ? 0x0200 : 0x0000);
    return 0;
}

static int wm8994_set_eq(const eq_config_t *eq) {
    /* WM8994 has a 5-band hardware EQ on the DAC path */
    if (!eq->enabled) {
        wm8994_write(0x0480, 0x0000);
        return 0;
    }
    /* EQ1 enable + coefficients — simplified fixed coefficients */
    wm8994_write(0x0480, 0x0001 |
        ((uint16_t)(eq->bands[0].gain_db > 0 ? 1 : 0) << 4));
    for (int i = 0; i < 5 && i < EQ_BANDS; i++) {
        /* Gain register: 0x481+i, format: gain_db + 12 in 4-bit field */
        int16_t g = (int16_t)(eq->bands[i].gain_db + 12.0f);
        if (g < 0) g = 0; if (g > 24) g = 24;
        wm8994_write((uint16_t)(0x0481 + i), (uint16_t)g);
    }
    return 0;
}

static int wm8994_set_drc(const drc_config_t *drc) {
    if (!drc->enabled) {
        wm8994_write(0x0440, 0x0000);
        return 0;
    }
    wm8994_write(0x0440, 0x03C0); /* DRC enable with default thresholds */
    return 0;
}

static int wm8994_set_samplerate(uint32_t hz) {
    uint16_t sr;
    switch (hz) {
    case 8000:  sr = 0x0003; break;
    case 11025: sr = 0x0013; break;
    case 16000: sr = 0x0007; break;
    case 22050: sr = 0x0017; break;
    case 32000: sr = 0x000B; break;
    case 44100: sr = 0x001B; break;
    case 48000: sr = 0x000F; break;
    default:    sr = 0x001B; break;
    }
    wm8994_write(0x0210, sr);
    return 0;
}

const audio_hal_api_t audio_hal_wm8994 = {
    .name           = "WM8994",
    .init           = wm8994_init,
    .set_volume     = wm8994_set_volume,
    .set_mute       = wm8994_set_mute,
    .set_eq         = wm8994_set_eq,
    .set_drc        = wm8994_set_drc,
    .set_samplerate = wm8994_set_samplerate,
    .hw_eq          = true,
    .hw_drc         = true,
    .eq_bands       = 5,
};
