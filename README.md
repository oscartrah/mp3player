# Zephyr MP3 Player

Vollständiger MP3-Player auf Basis von Zephyr RTOS mit LVGL-UI,
minimp3-Dekoder, ID3v2-Metadaten, Album-Art und 5-Band-Equalizer.

---

## Unterstützte Boards

| Board | Display | Auflösung | Codec | HW-EQ | Touch |
|---|---|---|---|---|---|
| STM32F769I-DISCO | DSI (OTM8009A) | 800 × 480 | WM8994 | ✓ | FT5336 |
| STM32F746G-DISCO | LTDC | 480 × 272 | WM8994 | ✓ | FT5336 |
| i.MX RT1060 EVK | Parallel LCD | 480 × 272 | WM8960 | SW | FT5336 |
| LPCXpresso54628 | Parallel LCD | 480 × 272 | WM8904 | ✗ | FT5336 |
| EK-RA6M3G | GLCDC | 480 × 272 | DA7212 | ✗ | GT911 |

HW-EQ = Hardware-Equalizer direkt im Codec-Chip. Boards ohne HW-EQ nutzen
den Software-Biquad-EQ (`biquad_eq.c`), der im Decoder-Thread läuft.

---

## Features

- MP3-Dekodierung via **minimp3** (MPEG 1/2/2.5, Layer I/II/III)
- **ID3v2.3/v2.4**-Parser: TIT2, TPE1, TALB, TRCK, APIC (Album-Art)
- **Xing/Info VBR-Header** → exakte Gesamtdauer + TOC-basiertes Seeking
- CBR-Schätzung als Fallback
- Seek über Progressbar (TOC bei VBR, Byte-Offset bei CBR)
- **Playlist-Browser** — rekursiver FAT32-Scan, bis 256 Tracks
- **Shuffle** (Fisher-Yates)
- **5-Band parametrischer EQ** — HW wo vorhanden, sonst SW-Biquad
- **6 EQ-Presets**: Flat · Bass Boost · Treble Boost · Vocal · Rock · Jazz + Custom
- Frequenzgang-Kurve (LVGL Canvas, live beim Bewegen der Slider)
- **NVS-Persistenz**: Volume, Shuffle, letzter Track, Position, EQ
- DPI-basiertes UI — `lv_dpx()`-Einheiten, automatische Schriftskalierung
- Zwei Layout-Klassen: **Normal** (800 × 480) und **Compact** (480 × 272)
- Album-Art-Anzeige (JPEG → LVGL Image Widget)

---

## Projektstruktur

```
mp3player/
├── CMakeLists.txt            # Quellenliste für alle Module
├── Kconfig                   # Audio-HAL-Auswahl (WM8994/WM8960/WM8904/DA7212)
├── prj.conf                  # Gemeinsame Zephyr-Optionen (I2S, FAT, NVS, LVGL)
├── west.yml                  # West-Manifest (Zephyr v3.7.0, minimp3, TJpgDec)
│
├── boards/                   # Board-spezifische Kconfig + Device-Tree-Overlays
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
│   └── minimp3/              # Wird per west update geklont
│       ├── CMakeLists.txt
│       └── Kconfig
│
└── src/
    ├── main.c                # Einstiegspunkt: Threads starten, Settings laden
    │
    │   ── Zustand & Steuerung ──────────────────────────────────────────
    ├── player_state.h/.c     # g_state, state_mutex, cmd_queue
    │
    │   ── Persistenz ──────────────────────────────────────────────────
    ├── settings.h/.c         # NVS: Volume, Shuffle, Track, Position, EQ
    │
    │   ── MP3-Verarbeitung ────────────────────────────────────────────
    ├── mp3_meta.h/.c         # ID3v2-Parser, Xing/TOC, APIC-Extraktion
    ├── mp3_seek.h/.c         # TOC- und Byte-Offset-basiertes Seeking
    ├── mp3_decoder.h/.c      # Decoder-Thread (minimp3), Player-Thread (I2S)
    ├── biquad_eq.h/.c        # SW-EQ: Low-Shelf, Peak, High-Shelf (Direct Form II)
    │
    │   ── Playlist ─────────────────────────────────────────────────────
    ├── playlist.h/.c         # FAT-Scan, qsort, next/prev, Fisher-Yates-Shuffle
    ├── playlist_ui.h/.c      # LVGL-Scroll-Liste, aktive Zeile hervorheben
    │
    │   ── Album-Art ────────────────────────────────────────────────────
    ├── album_art.h/.c        # JPEG-Decode → lv_img-Widget
    │
    │   ── Equalizer-UI ──────────────────────────────────────────────────
    ├── eq_presets.h/.c       # 6 Presets + Custom, eq_preset_apply, save/load
    ├── eq_curve.h/.c         # Frequenzgang-Canvas (log. Frequenzachse)
    ├── eq_bands.h/.c         # 5 vertikale Slider (± 12 dB)
    ├── eq_ui.h/.c            # EQ-Tab: Toggle, Preset-Dropdown, Kurve + Bands
    │
    │   ── Haupt-UI ──────────────────────────────────────────────────────
    ├── ui.h/.c               # Tabview (Now Playing / Playlist / EQ), ui_update
    ├── ui_theme.h            # Farben (UI_COL_*) und dp-Konstanten (UI_BTN_*)
    ├── ui_fonts.h/.c         # DPI-Tier: LOW (< 200 dpi) / HIGH (≥ 200 dpi)
    ├── ui_layout.h/.c        # Layout-Breakpoints: NORMAL vs. COMPACT
    ├── board_display.h/.c    # g_display: Breite, Höhe, DPI pro Board
    │
    │   ── Audio-HAL ────────────────────────────────────────────────────
    ├── audio_hal.h/.c        # Abstraktions-Interface + HAL-Auswahl zur Laufzeit
    ├── audio_hal_wm8994.c    # Wolfson WM8994 (STM32F769I + F746G)
    ├── audio_hal_wm8960.c    # Wolfson WM8960 (i.MX RT1060)
    ├── audio_hal_wm8904.c    # Wolfson WM8904 (LPCXpresso54628)
    ├── audio_hal_da7212.c    # Dialog DA7212 (EK-RA6M3G)
    │
    │   ── Board-Unterstützung ──────────────────────────────────────────
    ├── button_input.h/.c     # GPIO-Buttons für Boards ohne Touchscreen
    └── memory_cfg.h          # Board-abhängige Puffergrößen (ART_BUF_SIZE usw.)
```

---

## Voraussetzungen

### Zephyr SDK

Zephyr SDK 0.16 oder neuer wird benötigt.
Installationsanleitung: <https://docs.zephyrproject.org/latest/develop/toolchains/zephyr_sdk.html>

```bash
# Beispiel (Linux / macOS)
wget https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.16.8/zephyr-sdk-0.16.8_linux-x86_64.tar.xz
tar xf zephyr-sdk-0.16.8_linux-x86_64.tar.xz
cd zephyr-sdk-0.16.8
./setup.sh
```

Auf Windows wird das **Zephyr SDK für Windows** empfohlen (`.7z`-Archiv von der gleichen Release-Seite). Alternativ funktioniert WSL2 mit dem Linux-SDK.

### Python-Abhängigkeiten

```bash
pip install west
```

Weitere Python-Pakete installiert `west update` automatisch aus `zephyr/scripts/requirements.txt`.

---

## Setup

```bash
# 1. West-Workspace initialisieren (einmalig)
west init -l .
west update            # klont Zephyr v3.7.0, minimp3, TJpgDec

# 2. Python-Anforderungen von Zephyr installieren
pip install -r ../zephyr/scripts/requirements.txt
```

Nach `west update` liegt der Workspace so:

```
<workspace>/
├── mp3player/          ← dieses Repository (west manifest repo)
├── zephyr/             ← Zephyr v3.7.0
├── modules/
│   ├── minimp3/        ← lieff/minimp3
│   └── tjpgdec/        ← elm-chan/TJpgDec
└── ...                 ← weitere Zephyr-Module
```

---

## Build & Flash

Alle `west build`-Aufrufe werden aus dem `mp3player/`-Verzeichnis ausgeführt.
`-p always` erzwingt einen sauberen Build (empfohlen beim ersten Mal oder nach
Änderungen an `.conf`- oder `.overlay`-Dateien).

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

### Debug-Ausgabe

```bash
west build -b stm32f769i_disco -- -DCONFIG_LOG=y -DCONFIG_LOG_DEFAULT_LEVEL=3
west flash
west espresso   # oder: minicom / PuTTY auf dem Debug-UART
```

---

## SD-Karte vorbereiten

Formatierung: **FAT32**, Cluster-Größe 32 KB empfohlen.

```
SD:/
└── Music/
    ├── 01 - Titel.mp3
    ├── 02 - Titel.mp3
    └── Unterordner/
        └── 03 - Titel.mp3
```

- Bis zu **256 Tracks** werden erkannt (128 auf LPCXpresso54628).
- Unterordner werden **nicht** rekursiv durchsucht — alle MP3-Dateien
  müssen direkt unter `Music/` liegen (eine Ebene).
- ID3v2.3- und ID3v2.4-Tags werden ausgelesen; ohne Tags wird der
  Dateiname als Titel verwendet.

---

## Konfiguration

### Audio-HAL auswählen

Das richtige Backend wird über das Board-`conf` automatisch gesetzt.
Für manuelle Überschreibung:

```bash
west build -b mimxrt1060_evk -- -DCONFIG_AUDIO_HAL_WM8960=y
```

### Software-EQ erzwingen

```bash
west build -b stm32f769i_disco -- \
    -DCONFIG_AUDIO_HAL_SW_EQ_FALLBACK=y
```

### Speicher-Tuning (`memory_cfg.h`)

| Makro | Bedeutung |
|---|---|
| `ART_BUF_SIZE` | Maximale JPEG-Rohdatengröße (Album-Art) |
| `READ_BUF_SIZE` | Lesepuffer für den MP3-Dekoder |
| `PCM_QUEUE_DEPTH` | Tiefe der PCM-Msgqueue (= Latenz-Puffer) |
| `ART_DISPLAY_SIZE` | Pixel-Kantenlänge des angezeigten Cover-Quadrats |
| `PLAYLIST_MAX_TRACKS` | Maximale Playlist-Länge |

---

## Architektur

### Thread-Modell

| Thread | Priorität | Aufgabe |
|---|---|---|
| Player | 3 | PCM-Blöcke aus der Queue holen und an I2S liefern |
| Decoder | 5 | minimp3 dekodieren, optional SW-EQ anwenden, Queue befüllen |
| UI | 7 | LVGL-Handler + `ui_update()` alle 16 ms |
| Save | 9 | Position alle 5 s in NVS schreiben |

### Datenfluss

```
[SD-Karte]
    │  fs_read()
    ▼
[Decoder-Thread]  ──minimp3──►  PCM-Blöcke  ──k_msgq──►  [Player-Thread]
                                                               │ I2S/DMA
                                                               ▼
                                                          [Codec-Chip]
                                                          (WM8994 / WM8960 …)
```

### Audio-HAL

`audio_hal.h` definiert `audio_hal_api_t` — ein Tabelle aus Funktionszeigern.
`audio_hal.c` setzt `g_audio_hal` zur Build-Zeit per `#if CONFIG_AUDIO_HAL_*`.
Alle übrigen Module rufen ausschließlich `audio_set_volume()`, `audio_set_eq()`
und `audio_set_mute()` auf — sie sind codec-unabhängig.

### Equalizer

```
EQ-Konfiguration (eq_config_t)
    │
    ├─► audio_set_eq()  ──►  HW-EQ im Codec (WM8994)
    │
    └─► biquad_eq_configure()  ──►  biquad_eq_process()
                                    (im Decoder-Thread, auf PCM-Samples)
```

---

## Lizenz

MIT — siehe [LICENSE](LICENSE).

Eingebettete Drittanbieter-Bibliotheken:

| Bibliothek | Lizenz |
|---|---|
| [minimp3](https://github.com/lieff/minimp3) | CC0 1.0 |
| [TJpgDec](https://github.com/elm-chan/TJpgDec) | BSD 1-Clause |
| [Zephyr RTOS](https://github.com/zephyrproject-rtos/zephyr) | Apache 2.0 |
| [LVGL](https://github.com/lvgl/lvgl) | MIT |
