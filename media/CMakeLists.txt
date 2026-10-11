# AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
# https://ai-softphone.com
#
# SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
# SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of AI Softphone. There is NO WARRANTY, to the extent
# permitted by law. See the file LICENSE for the full terms.

include(FetchContent)

enable_language(C)

set(SOFTPHONE_LAME_VERSION "3.100")

FetchContent_Declare(lame
    URL
        https://downloads.sourceforge.net/project/lame/lame/3.100/lame-3.100.tar.gz
        http://deb.debian.org/debian/pool/main/l/lame/lame_3.100.orig.tar.gz
    URL_HASH SHA256=ddfe36cab873794038ae2c1210557ad34857a4b6bdc515785d1da9e175b1da1e)

FetchContent_MakeAvailable(lame)

configure_file(lame-config.h.in "${CMAKE_CURRENT_BINARY_DIR}/lame-config/config.h" @ONLY)

add_library(lame STATIC
    ${lame_SOURCE_DIR}/libmp3lame/bitstream.c
    ${lame_SOURCE_DIR}/libmp3lame/encoder.c
    ${lame_SOURCE_DIR}/libmp3lame/fft.c
    ${lame_SOURCE_DIR}/libmp3lame/gain_analysis.c
    ${lame_SOURCE_DIR}/libmp3lame/id3tag.c
    ${lame_SOURCE_DIR}/libmp3lame/lame.c
    ${lame_SOURCE_DIR}/libmp3lame/mpglib_interface.c
    ${lame_SOURCE_DIR}/libmp3lame/newmdct.c
    ${lame_SOURCE_DIR}/libmp3lame/presets.c
    ${lame_SOURCE_DIR}/libmp3lame/psymodel.c
    ${lame_SOURCE_DIR}/libmp3lame/quantize.c
    ${lame_SOURCE_DIR}/libmp3lame/quantize_pvt.c
    ${lame_SOURCE_DIR}/libmp3lame/reservoir.c
    ${lame_SOURCE_DIR}/libmp3lame/set_get.c
    ${lame_SOURCE_DIR}/libmp3lame/tables.c
    ${lame_SOURCE_DIR}/libmp3lame/takehiro.c
    ${lame_SOURCE_DIR}/libmp3lame/util.c
    ${lame_SOURCE_DIR}/libmp3lame/vbrquantize.c
    ${lame_SOURCE_DIR}/libmp3lame/VbrTag.c
    ${lame_SOURCE_DIR}/libmp3lame/version.c)

target_include_directories(lame
    SYSTEM PUBLIC ${lame_SOURCE_DIR}/include)

target_include_directories(lame
    PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}/lame-config"
        ${lame_SOURCE_DIR}/libmp3lame)

target_compile_definitions(lame PRIVATE HAVE_CONFIG_H)

if(NOT MSVC)
    target_compile_options(lame PRIVATE -w)
else()
    target_compile_options(lame PRIVATE /w)
endif()

set_target_properties(lame PROPERTIES POSITION_INDEPENDENT_CODE ON)

FetchContent_Declare(minimp3
    GIT_REPOSITORY https://github.com/lieff/minimp3.git
    GIT_TAG ea99364f61c14656440e8d77e9c233ccf3124633
    GIT_SHALLOW FALSE
    SYSTEM)

FetchContent_MakeAvailable(minimp3)

add_library(minimp3 INTERFACE)
target_include_directories(minimp3 SYSTEM INTERFACE ${minimp3_SOURCE_DIR})

add_library(softphone_media ${SOFTPHONE_LIBRARY_KIND}
    src/asr_audio.cpp
    src/mp3_decoder.cpp
    src/mp3_encoder.cpp
    src/mp3_stream.cpp
    src/sound_file.cpp
    src/time_stretch.cpp
    src/wav_decoder.cpp
    src/waveform.cpp)

add_library(softphone::media ALIAS softphone_media)

target_include_directories(softphone_media
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

target_link_libraries(softphone_media
    PUBLIC
        softphone::core
        softphone::coverage
    PRIVATE
        softphone::warnings
        lame
        minimp3)
