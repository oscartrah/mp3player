cmake_minimum_required(VERSION 3.20)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(mp3player)

target_sources(app PRIVATE
    src/main.c
    src/player_state.c
    src/settings.c
    src/mp3_meta.c
    src/mp3_seek.c
    src/mp3_decoder.c
    src/playlist.c
    src/playlist_ui.c
    src/album_art.c
    src/ui.c
    src/ui_fonts.c
    src/ui_layout.c
    src/board_display.c
    src/eq_presets.c
    src/eq_curve.c
    src/eq_bands.c
    src/eq_ui.c
    src/audio_hal.c
    src/audio_hal_wm8994.c
    src/audio_hal_wm8960.c
    src/audio_hal_wm8904.c
    src/audio_hal_da7212.c
    src/biquad_eq.c
    src/button_input.c
)

target_include_directories(app PRIVATE src)
