# Zephyr MP3 Player

Vollständiger MP3-Player auf Basis von Zephyr RTOS mit LVGL-UI,
minimp3-Dekoder, ID3v2-Metadaten, Album-Art und 5-Band-Equalizer.

## Unterstützte Boards

| Board                | Codec   | HW-EQ | Display      | Touch   |
|----------------------|---------|-------|--------------|---------|
| STM32F769I-DISCO     | WM8994  | ✓     | DSI 800×480  | FT6206  |
| STM32F746G-DISCO     | WM8994  | ✓     | LTDC 480×272 | FT5336  |
| i.MX RT1060 EVK      | WM8960  | SW    | Para 480×272 | FT5336  |
| LPCXpresso54628      | WM8904  | ✓     | Para 480×272 | FT5336  |
| EK-RA6M3G            | DA7212  | ✓     | GLCDC 480×272| GT911   |

## Features

- MP3-Dekodierung via minimp3 (MPEG 1/2/2.5, Layer I/II/III)
- ID3v2.3/v2.4 Parser (TIT2, TPE1, TALB, TRCK, TLEN, APIC)
- Album-Art via TJpgDec (JPEG → LVGL Canvas)
- Xing/Info VBR-Header → exakte Gesamtdauer
- CBR-Schätzung als Fallback
- Seek (Progressbar antippbar, TOC-basiert bei VBR)
- Playlist-Browser (rekursiver FAT-Scan, bis 256 Tracks)
- Shuffle (Fisher-Yates)
- 5-Band parametrischer EQ (HW wo verfügbar, sonst SW-Biquad)
- 6 EQ-Presets: Flat, Bass, Treble, Vocal, Rock, Jazz + Custom
- Frequenzgang-Kurve (LVGL Canvas, live)
- NVS-Persistenz: Volume, Shuffle, letzter Track, Position, EQ
- DPI-basiertes UI (dp-Einheiten, automatische Skalierung)
- Zwei Layout-Klassen: 800×480 (Normal) und 480×272 (Compact)

## Projektstruktur

```
mp3player/
├── CMakeLists.txt
├── Kconfig
├── prj.conf                    # gemeinsame Optionen
├── west.yml
├── boards/
│   ├── stm32f769i_disco.conf   # board-spezifische Kconfig
│   ├── stm32f769i_disco.overlay
│   ├── stm32f746g_disco.conf
│   ├── stm32f746g_disco.overlay
│   ├── mimxrt1060_evk.conf
│   ├── mimxrt1060_evk.overlay
│   ├── lpcxpresso54628.conf
│   ├── lpcxpresso54628.overlay
│   ├── ek_ra6m3g.conf
│   └── ek_ra6m3g.overlay
├── modules/
│   └── minimp3/                # via west.yml
└── src/
    ├── main.c
    ├── player_state.h/c        # Shared State + cmd_queue
    ├── settings.h/c            # NVS-Persistenz
    ├── mp3_meta.h/c            # ID3v2, Xing, APIC
    ├── mp3_seek.h/c            # Seek-Offset-Berechnung
    ├── mp3_decoder.h/c         # minimp3 + Decoder-Thread
    ├── playlist.h/c            # FAT-Scan, Sort, Shuffle
    ├── playlist_ui.h/c         # LVGL Scroll-Liste
    ├── album_art.h/c           # TJpgDec → LVGL
    ├── ui.h/c                  # Haupt-UI, Tabview
    ├── ui_theme.h              # Farben + dp-Konstanten
    ├── ui_fonts.h/c            # DPI-Klassen (HIGH/LOW)
    ├── ui_layout.h/c           # Breakpoints (NORMAL/COMPACT)
    ├── ui_widgets.h            # Widget-Factories
    ├── board_display.h/c       # DPI-Profile
    ├── eq_presets.h/c          # 6 Presets + CUSTOM
    ├── eq_curve.h/c            # Frequenzgang Canvas
    ├── eq_bands.h/c            # 5 vertikale Slider
    ├── eq_ui.h/c               # EQ-Screen
    ├── audio_hal.h/c           # generisches HAL-Interface
    ├── audio_hal_wm8994.c      # STM32F769I + F746G
    ├── audio_hal_wm8960.c      # i.MX RT1060
    ├── audio_hal_wm8904.c      # LPCXpresso54628
    ├── audio_hal_da7212.c      # EK-RA6M3G
    ├── biquad_eq.h/c           # SW-EQ Fallback
    ├── button_input.h/c        # GPIO-Buttons (LPC54628)
    └── memory_cfg.h            # Board-Puffergrößen
```

## Build

```bash
# Abhängigkeiten holen
west init -l .
west update

# STM32F769I-DISCO
west build -b stm32f769i_disco -p always

# STM32F746G-DISCO
west build -b stm32f746g_disco -p always

# i.MX RT1060 EVK
west build -b mimxrt1060_evk -p always

# LPCXpresso54628
west build -b lpcxpresso54628 -p always

# EK-RA6M3G
west build -b ek_ra6m3g -p always

# Flashen
west flash
```

## SD-Karte vorbereiten

```
SD:/
└── Music/
    ├── 01 - Titel.mp3
    ├── 02 - Titel.mp3
    └── Subfolder/
        └── 03 - Titel.mp3
```

FAT32 formatieren. Bis zu 256 Tracks werden erkannt (128 auf LPC54628).

## Thread-Prioritäten

| Thread   | Priorität | Begründung                          |
|----------|-----------|-------------------------------------|
| Player   | 3         | I2S-DMA darf nicht abreißen         |
| Decoder  | 5         | muss Player rechtzeitig versorgen   |
| UI       | 7         | darf kurz warten                    |
| Save     | 9         | niedrigste Priorität                |

## Lizenz

MIT
