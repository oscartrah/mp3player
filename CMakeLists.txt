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

# minimp3 — single-header MP3 decoder
set(MINIMP3_DIR ${CMAKE_CURRENT_SOURCE_DIR}/modules/minimp3)
if(EXISTS ${MINIMP3_DIR}/minimp3.h)
    target_include_directories(app PRIVATE ${MINIMP3_DIR})
endif()

# TJpgDec — fetched by west into modules/tjpgdec (cmumford/TJpgDec mirror)
set(TJPGDEC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../modules/tjpgdec)
if(EXISTS ${TJPGDEC_DIR}/tjpgd.c)
    target_sources(app PRIVATE ${TJPGDEC_DIR}/tjpgd.c)
    target_include_directories(app PRIVATE ${TJPGDEC_DIR})
    target_compile_definitions(app PRIVATE HAS_TJPGDEC=1)
endif()
