#include "audio_hal.h"
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>

#define WM8904_ADDR 0x1A

static const struct device *s_i2c;

static int wm8904_write(uint8_t reg, uint16_t val) {
    uint8_t buf[3] = { reg, (uint8_t)(val >> 8), (uint8_t)(val & 0xFF) };
    return i2c_write(s_i2c, buf, 3, WM8904_ADDR);
}

static int wm8904_init(void) {
    s_i2c = DEVICE_DT_GET_ANY(wolfson_wm8904);
    if (!s_i2c) s_i2c = DEVICE_DT_GET_ANY(zephyr_i2c);
    if (!s_i2c || !device_is_ready(s_i2c)) return -ENODEV;

    /* SW reset */
    wm8904_write(0x00, 0x8904);
    k_msleep(10);

    /* Bias & VMID */
    wm8904_write(0x04, 0x0019); /* VMID_ENA, BIAS_ENA, VMID_BUF_ENA */
    k_msleep(5);
    wm8904_write(0x04, 0x0018); /* VMID normal */

    /* Clock: SYSCLK = MCLK */
    wm8904_write(0x15, 0x0845); /* MCLK div, SYSCLK_SRC=MCLK */

    /* Audio interface: I2S, 16-bit, slave */
    wm8904_write(0x18, 0x0002);

    /* DAC power */
    wm8904_write(0x03, 0x0003); /* DACL_ENA, DACR_ENA */
    wm8904_write(0x0E, 0x0003); /* HPL_PGA_ENA, HPR_PGA_ENA */

    /* DAC → headphone path */
    wm8904_write(0x43, 0x000F); /* Charge pump enable */
    wm8904_write(0x5A, 0x00FF); /* Analogue HP seq */

    /* Headphone volume: 0 dB */
    wm8904_write(0x39, 0x00C0); /* HPOUTL_VOL */
    wm8904_write(0x3A, 0x00C0); /* HPOUTR_VOL */

    return 0;
}

static int wm8904_set_volume(uint8_t vol_pct) {
    uint16_t vol = (uint16_t)(vol_pct * 0x3F / 100);
    wm8904_write(0x39, 0x0100 | vol); /* update bit */
    wm8904_write(0x3A, 0x0100 | vol);
    return 0;
}

static int wm8904_set_mute(bool mute) {
    wm8904_write(0x21, mute ? 0x0008 : 0x0000); /* DAC_MUTE */
    return 0;
}

static int wm8904_set_eq(const eq_config_t *eq) {
    ARG_UNUSED(eq);
    return 0;
}

static int wm8904_set_drc(const drc_config_t *drc) {
    if (!drc->enabled) {
        wm8904_write(0x28, 0x0000);
        return 0;
    }
    wm8904_write(0x28, 0x01AF); /* DRC enable with defaults */
    return 0;
}

static int wm8904_set_samplerate(uint32_t hz) {
    ARG_UNUSED(hz);
    return 0;
}

const audio_hal_api_t audio_hal_wm8904 = {
    .name           = "WM8904",
    .init           = wm8904_init,
    .set_volume     = wm8904_set_volume,
    .set_mute       = wm8904_set_mute,
    .set_eq         = wm8904_set_eq,
    .set_drc        = wm8904_set_drc,
    .set_samplerate = wm8904_set_samplerate,
    .hw_eq          = false,
    .hw_drc         = true,
    .eq_bands       = 0,
};
