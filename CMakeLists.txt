cmake_minimum_required(VERSION 3.16)

set(target_name floeslide)
option(EMULATOR "Save progress to H:/ (HostFS) instead of B:/" ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(BIN_DIR "${CMAKE_SOURCE_DIR}/bin" CACHE PATH "Where the .bin goes")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BIN_DIR})

set(ZOS_TOOLCHAIN sdcc)
include($ENV{ZOS_PATH}/cmake/zos_init.cmake)

zos_use(zvb)
include($ENV{ZGDK_PATH}/cmake/zgdk_init.cmake)

project(floeslide C ASM)

add_executable(${target_name}
    src/controls.c
    src/main.c
    src/play.c
    src/levels.c
    src/hud.c
    src/sfx.c
    src/music.c
    src/assets.c
)
target_compile_options(${target_name} PRIVATE --opt-code-size)
if(EMULATOR)
    target_compile_definitions(${target_name} PRIVATE EMULATOR=1)
endif()

# For recording videos: a bot plays the par solutions (src/autopilot.c), from any floe
option(AUTOPILOT "Bot plays the game (video recording)" OFF)
if(AUTOPILOT)
    target_sources(${target_name} PRIVATE src/autopilot.c)
    target_compile_definitions(${target_name} PRIVATE AUTOPILOT=1)
endif()
set(START_FLOE 0 CACHE STRING "Autopilot: first floe, 0-based")
target_compile_definitions(${target_name} PRIVATE START_FLOE=${START_FLOE})

find_package(ZVB_SDK REQUIRED CONFIG PATHS "$ENV{ZVB_SDK_PATH}/cmake" NO_DEFAULT_PATH)
find_package(ZGDK REQUIRED CONFIG PATHS "$ENV{ZGDK_PATH}/cmake" NO_DEFAULT_PATH)
target_link_libraries(${target_name} PUBLIC zgdk zvb_gfx zvb_sound)

# Assets go in floeslide.zar next to the .bin, read when needed (src/assets.c): the tileset (LZ, in
# chunks the music pattern buffers can hold), palette, music and the floes' cells. tileset.gif comes
# from tools/make_assets.py, the .zmt tracks from tools/make_music.py, floes.bin from
# tools/make_levels.py (all committed, so the build doesn't need Pillow).
include(cmake/assets.cmake)

zos_add_outputs(${target_name})

set_property(TARGET ${target_name} APPEND PROPERTY LINK_DEPENDS "$ENV{ZGDK_PATH}/lib/zgdk.lib")
