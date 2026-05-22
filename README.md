# Zephyr MP3 Player

A complete MP3 player built on Zephyr RTOS with an LVGL UI,
minimp3 decoder, ID3v2 metadata, album art, and a 5-band equalizer.

---

## Supported Boards

| Board | Display | Resolution | Codec | HW-EQ | Touch |
|---|---|---|---|---|---|
| STM32F769I-DISCO | DSI (OTM8009A) | 800 × 480 | WM8994 | ✓ | FT5336 |
| STM32F746G-DISCO | LTDC | 480 × 272 | WM8994 | ✓ | FT5336 |
| i.MX RT1060 EVK | Parallel LCD | 480 × 272 | WM8960 | SW | FT5336 |
| LPCXpresso54628 | Parallel LCD | 480 × 272 | WM8904 | ✗ | FT5336 |
| EK-RA6M3G | GLCDC | 480 × 272 | DA7212 | ✗ | GT911 |

HW-EQ = hardware equalizer built into the codec chip. Boards without HW-EQ
use the software biquad EQ (`biquad_eq.c`) running in the decoder thread.

---

## Features

- MP3 decoding via **minimp3** (MPEG 1/2/2.5, Layer I/II/III)
- **ID3v2.3/v2.4** parser: TIT2, TPE1, TALB, TRCK, APIC (album art)
- **Xing/Info VBR header** → exact total duration + TOC-based seeking
- CBR byte-offset seeking as fallback
- Seek via progress bar (TOC for VBR, byte offset for CBR)
- **Playlist browser** — FAT32 directory scan, up to 256 tracks
- **Shuffle** (Fisher-Yates)
- **5-band parametric EQ** — hardware where available, otherwise SW biquad
- **6 EQ presets**: Flat · Bass Boost · Treble Boost · Vocal · Rock · Jazz + Custom
- Live frequency response curve (LVGL canvas, updates while moving sliders)
- **NVS persistence**: volume, shuffle, last track, playback position, EQ
- DPI-aware UI — `lv_dpx()` units, automatic font scaling
- Two layout classes: **Normal** (800 × 480) and **Compact** (480 × 272)
- Album art display (JPEG → LVGL image widget)

---

## Project Structure

```
mp3player/
├── CMakeLists.txt            # Source list for all modules
├── Kconfig                   # Audio HAL selection (WM8994/WM8960/WM8904/DA7212)
├── prj.conf                  # Shared Zephyr options (I2S, FAT, NVS, LVGL)
├── west.yml                  # West manifest (Zephyr v3.7.0, minimp3, TJpgDec)
│
├── boards/                   # Board-specific Kconfig fragments + DT overlays
│   ├── stm32f769i_disco.conf
│   ├── stm32f769i_disco.overlay
│   ├── stm32f746g_disco.conf
│   ├── stm32f746g_disco.overlay
│   ├── mimxrt1060_evk.conf
│   ├── mimxrt1060_evk.overlay
│   ├── lpcxpresso54628.conf
│   ├── lpcxpresso54628.overlay
│   ├── ek_ra6m3g.conf
│   └── ek_ra6m3g.overlay
│
├── modules/
│   └── minimp3/              # Cloned by west update
│       ├── CMakeLists.txt
│       └── Kconfig
│
└── src/
    ├── main.c                # Entry point: start threads, load settings
    │
    │   ── State & Control ──────────────────────────────────────────────
    ├── player_state.h/.c     # g_state, state_mutex, cmd_queue
    │
    │   ── Persistence ──────────────────────────────────────────────────
    ├── settings.h/.c         # NVS: volume, shuffle, track, position, EQ
    │
    │   ── MP3 Processing ────────────────────────────────────────────────
    ├── mp3_meta.h/.c         # ID3v2 parser, Xing/TOC, APIC extraction
    ├── mp3_seek.h/.c         # TOC- and byte-offset-based seeking
    ├── mp3_decoder.h/.c      # Decoder thread (minimp3), player thread (I2S)
    ├── biquad_eq.h/.c        # SW EQ: low-shelf, peak, high-shelf (Direct Form II)
    │
    │   ── Playlist ──────────────────────────────────────────────────────
    ├── playlist.h/.c         # FAT scan, qsort, next/prev, Fisher-Yates shuffle
    ├── playlist_ui.h/.c      # LVGL scroll list, highlight active row
    │
    │   ── Album Art ─────────────────────────────────────────────────────
    ├── album_art.h/.c        # JPEG decode → lv_img widget
    │
    │   ── Equalizer UI ───────────────────────────────────────────────────
    ├── eq_presets.h/.c       # 6 presets + Custom, eq_preset_apply, save/load
    ├── eq_curve.h/.c         # Frequency response canvas (log frequency axis)
    ├── eq_bands.h/.c         # 5 vertical sliders (± 12 dB)
    ├── eq_ui.h/.c            # EQ tab: toggle switch, preset dropdown, curve + bands
    │
    │   ── Main UI ────────────────────────────────────────────────────────
    ├── ui.h/.c               # Tabview (Now Playing / Playlist / EQ), ui_update
    ├── ui_theme.h            # Colours (UI_COL_*) and dp constants (UI_BTN_*)
    ├── ui_fonts.h/.c         # DPI tier: LOW (< 200 dpi) / HIGH (≥ 200 dpi)
    ├── ui_layout.h/.c        # Layout breakpoints: NORMAL vs. COMPACT
    ├── board_display.h/.c    # g_display: width, height, DPI per board
    │
    │   ── Audio HAL ──────────────────────────────────────────────────────
    ├── audio_hal.h/.c        # Abstraction interface + HAL selection at build time
    ├── audio_hal_wm8994.c    # Wolfson WM8994 (STM32F769I + F746G)
    ├── audio_hal_wm8960.c    # Wolfson WM8960 (i.MX RT1060)
    ├── audio_hal_wm8904.c    # Wolfson WM8904 (LPCXpresso54628)
    ├── audio_hal_da7212.c    # Dialog DA7212 (EK-RA6M3G)
    │
    │   ── Board Support ──────────────────────────────────────────────────
    ├── button_input.h/.c     # GPIO buttons for boards without a touchscreen
    └── memory_cfg.h          # Board-dependent buffer sizes (ART_BUF_SIZE etc.)
```

---

## Prerequisites

### Zephyr SDK

Zephyr SDK 0.16 or later is required.
Installation guide: <https://docs.zephyrproject.org/latest/develop/toolchains/zephyr_sdk.html>

```bash
# Example (Linux / macOS)
wget https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.16.8/zephyr-sdk-0.16.8_linux-x86_64.tar.xz
tar xf zephyr-sdk-0.16.8_linux-x86_64.tar.xz
cd zephyr-sdk-0.16.8
./setup.sh
```

On Windows use the **Zephyr SDK for Windows** (`.7z` archive from the same release
page). WSL2 with the Linux SDK also works.

### Python

```bash
pip install west
```

Additional Python packages are installed automatically by `west update` from
`zephyr/scripts/requirements.txt`.

---

## Setup

```bash
# 1. Initialise the west workspace (once)
west init -l .
west update            # clones Zephyr v3.7.0, minimp3, TJpgDec

# 2. Install Zephyr Python requirements
pip install -r ../zephyr/scripts/requirements.txt
```

After `west update` the workspace looks like this:

```
<workspace>/
├── mp3player/          ← this repository (west manifest repo)
├── zephyr/             ← Zephyr v3.7.0
├── modules/
│   ├── minimp3/        ← lieff/minimp3
│   └── tjpgdec/        ← elm-chan/TJpgDec
└── ...                 ← additional Zephyr modules
```

---

## Build & Flash

All `west build` commands are run from the `mp3player/` directory.
`-p always` forces a clean build — recommended on the first run or after
changing `.conf` or `.overlay` files.

```bash
# STM32F769I-DISCO  (800×480, WM8994, HW-EQ)
west build -b stm32f769i_disco -p always
west flash

# STM32F746G-DISCO  (480×272, WM8994, HW-EQ)
west build -b stm32f746g_disco -p always
west flash

# i.MX RT1060 EVK   (480×272, WM8960, SW-EQ)
west build -b mimxrt1060_evk -p always
west flash

# LPCXpresso54628   (480×272, WM8904, SW-EQ)
west build -b lpcxpresso54628 -p always
west flash

# EK-RA6M3G         (480×272, DA7212, SW-EQ)
west build -b ek_ra6m3g -p always
west flash
```

### Debug output

```bash
west build -b stm32f769i_disco -- -DCONFIG_LOG=y -DCONFIG_LOG_DEFAULT_LEVEL=3
west flash
# then open the debug UART with minicom, PuTTY, or west espresso
```

---

## SD Card Setup

Format: **FAT32**, 32 KB cluster size recommended.

```
SD:/
└── Music/
    ├── 01 - Track.mp3
    ├── 02 - Track.mp3
    └── Subfolder/
        └── 03 - Track.mp3
```

- Up to **256 tracks** are detected (128 on LPCXpresso54628).
- Subdirectories are **not** scanned recursively — all MP3 files must sit
  directly under `Music/` (one level deep).
- ID3v2.3 and ID3v2.4 tags are read; the filename is used as the title when
  no tags are present.

---

## Configuration

### Selecting the audio HAL

The correct backend is set automatically by each board's `.conf` file.
To override manually:

```bash
west build -b mimxrt1060_evk -- -DCONFIG_AUDIO_HAL_WM8960=y
```

### Forcing the software EQ

```bash
west build -b stm32f769i_disco -- -DCONFIG_AUDIO_HAL_SW_EQ_FALLBACK=y
```

### Memory tuning (`memory_cfg.h`)

| Macro | Purpose |
|---|---|
| `ART_BUF_SIZE` | Maximum raw JPEG size for album art |
| `READ_BUF_SIZE` | Read buffer for the MP3 decoder |
| `PCM_QUEUE_DEPTH` | PCM message-queue depth (latency buffer) |
| `ART_DISPLAY_SIZE` | Pixel side length of the displayed cover square |
| `PLAYLIST_MAX_TRACKS` | Maximum playlist length |

---

## Architecture

### Thread model

| Thread | Priority | Role |
|---|---|---|
| Player | 3 | Drain PCM queue and feed samples to I2S |
| Decoder | 5 | Run minimp3, optionally apply SW EQ, fill PCM queue |
| UI | 7 | LVGL handler + `ui_update()` every 16 ms |
| Save | 9 | Write playback position to NVS every 5 s |

### Data flow

```
[SD card]
    │  fs_read()
    ▼
[Decoder thread]  ──minimp3──►  PCM blocks  ──k_msgq──►  [Player thread]
                                                               │  I2S / DMA
                                                               ▼
                                                          [Codec chip]
                                                     (WM8994 / WM8960 / …)
```

### Audio HAL

`audio_hal.h` defines `audio_hal_api_t` — a table of function pointers.
`audio_hal.c` assigns `g_audio_hal` at build time via `#if CONFIG_AUDIO_HAL_*`.
All other modules call only `audio_set_volume()`, `audio_set_eq()`, and
`audio_set_mute()` — they are codec-agnostic.

### Equalizer

```
EQ configuration (eq_config_t)
    │
    ├─► audio_set_eq()  ──►  HW EQ in codec (WM8994)
    │
    └─► biquad_eq_configure()  ──►  biquad_eq_process()
                                    (in decoder thread, applied to PCM samples)
```

---

## License

MIT — see [LICENSE](LICENSE).

Embedded third-party libraries:

| Library | License |
|---|---|
| [minimp3](https://github.com/lieff/minimp3) | CC0 1.0 |
| [TJpgDec](https://github.com/elm-chan/TJpgDec) | BSD 1-Clause |
| [Zephyr RTOS](https://github.com/zephyrproject-rtos/zephyr) | Apache 2.0 |
| [LVGL](https://github.com/lvgl/lvgl) | MIT |
