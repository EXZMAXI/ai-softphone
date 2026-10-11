# AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
# https://ai-softphone.com
#
# SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
# SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of AI Softphone. There is NO WARRANTY, to the extent
# permitted by law. See the file LICENSE for the full terms.

set(SOFTPHONE_CAPTURE_SOURCES
    src/applications.cpp
    src/capture_session.cpp
    src/detector.cpp
    src/devices.cpp
    src/echo_encoder.cpp
    src/fake_capture.cpp
    src/signal_trail.cpp
    src/system_capture.cpp)

if(TARGET softphone_pjproject AND SOFTPHONE_BUILD_SIP)
    set(SOFTPHONE_CAPTURE_ECHO "pjmedia")
    set(SOFTPHONE_CAPTURE_ECHO_SOURCE src/echo_pjmedia.cpp)
else()
    set(SOFTPHONE_CAPTURE_ECHO "none")
    set(SOFTPHONE_CAPTURE_ECHO_SOURCE src/echo_none.cpp)
endif()

set(SOFTPHONE_CAPTURE_BACKEND "none")
set(SOFTPHONE_CAPTURE_BACKEND_SOURCE src/backend_none.cpp)

set(SOFTPHONE_CAPTURE_DETECTOR "none")
set(SOFTPHONE_CAPTURE_DETECTOR_SOURCE src/detector_none.cpp)

if(WIN32)
    set(SOFTPHONE_CAPTURE_BACKEND "wasapi")
    set(SOFTPHONE_CAPTURE_BACKEND_SOURCE src/backend_wasapi.cpp)
    set(SOFTPHONE_CAPTURE_DETECTOR "wasapi")
    set(SOFTPHONE_CAPTURE_DETECTOR_SOURCE src/detector_wasapi.cpp)
endif()

if(UNIX AND NOT APPLE)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(SOFTPHONE_PULSE QUIET libpulse)
    endif()
    if(SOFTPHONE_PULSE_FOUND)
        set(SOFTPHONE_CAPTURE_BACKEND "pulse ${SOFTPHONE_PULSE_VERSION}")
        set(SOFTPHONE_CAPTURE_BACKEND_SOURCE src/backend_pulse.cpp)
        set(SOFTPHONE_CAPTURE_DETECTOR "pulse")
        set(SOFTPHONE_CAPTURE_DETECTOR_SOURCE src/detector_pulse.cpp)
    endif()
endif()

if(APPLE)
    set(SOFTPHONE_CAPTURE_BACKEND "core audio")
    set(SOFTPHONE_CAPTURE_BACKEND_SOURCE src/backend_coreaudio.mm)
    set(SOFTPHONE_CAPTURE_DETECTOR "core audio")
    set(SOFTPHONE_CAPTURE_DETECTOR_SOURCE src/detector_coreaudio.mm)
endif()

add_library(softphone_capture ${SOFTPHONE_LIBRARY_KIND}
    ${SOFTPHONE_CAPTURE_SOURCES}
    ${SOFTPHONE_CAPTURE_BACKEND_SOURCE}
    ${SOFTPHONE_CAPTURE_DETECTOR_SOURCE}
    ${SOFTPHONE_CAPTURE_ECHO_SOURCE})

add_library(softphone::capture ALIAS softphone_capture)

target_include_directories(softphone_capture
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

target_link_libraries(softphone_capture
    PUBLIC
        softphone::core
        softphone::coverage
    PRIVATE
        softphone::warnings)

if(SOFTPHONE_CAPTURE_ECHO STREQUAL "pjmedia")
    target_link_libraries(softphone_capture PRIVATE softphone::pjproject)
endif()

if(APPLE)
    set_source_files_properties(
        src/backend_coreaudio.mm src/detector_coreaudio.mm
        PROPERTIES COMPILE_OPTIONS "-Wno-old-style-cast")
endif()

if(APPLE)
    target_link_libraries(softphone_capture PRIVATE
        "-framework CoreAudio" "-framework AudioToolbox" "-framework Foundation")
endif()

if(SOFTPHONE_PULSE_FOUND)
    target_include_directories(softphone_capture SYSTEM PRIVATE ${SOFTPHONE_PULSE_INCLUDE_DIRS})
    target_link_libraries(softphone_capture PRIVATE ${SOFTPHONE_PULSE_LIBRARIES})
    target_link_directories(softphone_capture PRIVATE ${SOFTPHONE_PULSE_LIBRARY_DIRS})
endif()

if(SOFTPHONE_CAPTURE_BACKEND STREQUAL "wasapi")
    target_link_libraries(softphone_capture PRIVATE ole32 avrt)
endif()

message(STATUS "Third-party capture backend: ${SOFTPHONE_CAPTURE_BACKEND}, detector: ${SOFTPHONE_CAPTURE_DETECTOR}, echo canceller: ${SOFTPHONE_CAPTURE_ECHO}")
