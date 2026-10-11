# AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
# https://ai-softphone.com
#
# SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
# SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of AI Softphone. There is NO WARRANTY, to the extent
# permitted by law. See the file LICENSE for the full terms.

include(Pjproject)

if(WIN32)

    set(SOFTPHONE_DEVICE_NAMES src/device_descriptions_mmdevice.cpp)
elseif(UNIX AND NOT APPLE)
    set(SOFTPHONE_DEVICE_NAMES src/device_descriptions_alsa.cpp)
else()
    set(SOFTPHONE_DEVICE_NAMES src/device_descriptions_plain.cpp)
endif()

if(UNIX AND NOT APPLE)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(ALSA alsa)
    pkg_check_modules(PULSE libpulse)
endif()

if(PULSE_FOUND)
    set(SOFTPHONE_AUDIO_DEVICE_BACKEND src/pulse_audio_device.cpp)
else()
    set(SOFTPHONE_AUDIO_DEVICE_BACKEND src/pulse_audio_device_none.cpp)
endif()

if(APPLE)
    set(SOFTPHONE_DEFAULT_DEVICE src/system_default_device_apple.cpp)
elseif(WIN32)
    set(SOFTPHONE_DEFAULT_DEVICE src/system_default_device_mmdevice.cpp)
elseif(PULSE_FOUND)
    set(SOFTPHONE_DEFAULT_DEVICE src/system_default_device_pulse.cpp)
else()
    set(SOFTPHONE_DEFAULT_DEVICE src/system_default_device_none.cpp)
endif()

add_library(softphone_sip ${SOFTPHONE_LIBRARY_KIND}
    src/device_descriptions.cpp
    src/dialog_info.cpp
    src/from_inside.cpp
    src/system_default_device.cpp
    ${SOFTPHONE_DEFAULT_DEVICE}
    ${SOFTPHONE_DEVICE_NAMES}
    ${SOFTPHONE_AUDIO_DEVICE_BACKEND}
    src/tls_roots.cpp
    src/pjsip_backend.cpp)

add_library(softphone::sip ALIAS softphone_sip)

target_include_directories(softphone_sip
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

target_link_libraries(softphone_sip
    PUBLIC
        softphone::core
        softphone::coverage
    PRIVATE
        softphone::warnings
    PUBLIC
        softphone::pjproject)

if(WIN32)
    target_link_libraries(softphone_sip PRIVATE ole32)
endif()

if(APPLE)
    target_link_libraries(softphone_sip PRIVATE "-framework CoreAudio" "-framework CoreFoundation")
endif()

if(APPLE)
    target_link_libraries(softphone_sip PRIVATE "-framework Security")
elseif(UNIX)
    find_package(OpenSSL REQUIRED COMPONENTS Crypto)
    target_link_libraries(softphone_sip PRIVATE OpenSSL::Crypto)
endif()

if(UNIX AND NOT APPLE)
    if(ALSA_FOUND)
        target_compile_definitions(softphone_sip PRIVATE SOFTPHONE_HAS_ALSA)
        target_include_directories(softphone_sip SYSTEM PRIVATE ${ALSA_INCLUDE_DIRS})
        target_link_libraries(softphone_sip PRIVATE ${ALSA_LIBRARIES})
        message(STATUS "ALSA ${ALSA_VERSION}: sound devices will carry their own names")
    else()
        message(STATUS "No ALSA: sound devices will be shown by their identifiers")
    endif()

    if(PULSE_FOUND)
        target_compile_definitions(softphone_sip PRIVATE SOFTPHONE_HAS_PULSE)
        target_include_directories(softphone_sip SYSTEM PRIVATE ${PULSE_INCLUDE_DIRS})
        target_link_libraries(softphone_sip PRIVATE ${PULSE_LIBRARIES})
        message(STATUS
            "libpulse ${PULSE_VERSION}: sound devices will carry the words the desktop uses")
    else()
        message(STATUS "No libpulse: sound devices will carry ALSA's words")
    endif()
endif()
