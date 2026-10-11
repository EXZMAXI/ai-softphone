# AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
# https://ai-softphone.com
#
# SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
# SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of AI Softphone. There is NO WARRANTY, to the extent
# permitted by law. See the file LICENSE for the full terms.

find_package(Threads REQUIRED)

include(FetchContent)

FetchContent_Declare(nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW TRUE
    SYSTEM)

set(JSON_BuildTests OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(nlohmann_json)

file(GLOB SOFTPHONE_CONTENT_FILES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/content/shipped-*.json)
list(SORT SOFTPHONE_CONTENT_FILES)

set(SOFTPHONE_CONTENT_LANGUAGES "")
foreach(one ${SOFTPHONE_CONTENT_FILES})
    get_filename_component(name ${one} NAME_WE)
    string(REPLACE "shipped-" "" name ${name})
    list(APPEND SOFTPHONE_CONTENT_LANGUAGES ${name})
endforeach()

file(GLOB SOFTPHONE_COUNTRY_FILES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/content/countries-*.json)
list(SORT SOFTPHONE_COUNTRY_FILES)

foreach(one ${SOFTPHONE_COUNTRY_FILES})
    get_filename_component(name ${one} NAME_WE)
    list(APPEND SOFTPHONE_CONTENT_FILES ${one})
    list(APPEND SOFTPHONE_CONTENT_LANGUAGES ${name})
endforeach()

if(NOT "countries-en" IN_LIST SOFTPHONE_CONTENT_LANGUAGES)
    message(FATAL_ERROR "core/content/countries-en.json is the list every other "
                        "language falls back to and it is not there")
endif()

if(NOT "en" IN_LIST SOFTPHONE_CONTENT_LANGUAGES)
    message(FATAL_ERROR "core/content/shipped-en.json is the language every "
                        "other one falls back to and it is not there")
endif()

set(SOFTPHONE_CONTENT_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/shipped_content_data.cpp)
add_custom_command(
    OUTPUT ${SOFTPHONE_CONTENT_SOURCE}
    COMMAND ${CMAKE_COMMAND}
            -DOUT=${SOFTPHONE_CONTENT_SOURCE}
            "-DFILES=${SOFTPHONE_CONTENT_FILES}"
            "-DNAMES=${SOFTPHONE_CONTENT_LANGUAGES}"
            -P ${CMAKE_SOURCE_DIR}/cmake/EmbedContent.cmake
    DEPENDS ${SOFTPHONE_CONTENT_FILES} ${CMAKE_SOURCE_DIR}/cmake/EmbedContent.cmake
    COMMENT "Embedding the shipped content"
    VERBATIM)

if(WIN32)
    set(SOFTPHONE_PATHS src/paths_win32.cpp)
elseif(APPLE)
    set(SOFTPHONE_PATHS src/paths_apple.cpp)
else()
    set(SOFTPHONE_PATHS src/paths_xdg.cpp)
endif()

if(WIN32)
    set(SOFTPHONE_UPDATE_INSTALLER src/updates_installer_win32.cpp)
else()
    set(SOFTPHONE_UPDATE_INSTALLER src/updates_installer_none.cpp)
endif()

if(WIN32)
    set(SOFTPHONE_CRASH src/crash_win32.cpp)
elseif(UNIX)
    set(SOFTPHONE_CRASH src/crash_posix.cpp)
else()
    set(SOFTPHONE_CRASH src/crash_none.cpp)
endif()

add_library(softphone_core ${SOFTPHONE_LIBRARY_KIND}
    src/account_config.cpp
    src/api.cpp
    src/answers.cpp
    src/artifacts.cpp
    src/automations.cpp
    src/asr.cpp
    src/asr_plan.cpp
    src/audio_encoder.cpp
    src/api_json.cpp
    src/audio_devices.cpp
    src/audio_signal.cpp
    src/backoff.cpp
    src/busy_lamp.cpp
    src/buttons.cpp
    src/auto_answer.cpp
    src/call.cpp
    src/call_controller.cpp
    src/crash.cpp
    src/call_historian.cpp
    src/call_log.cpp
    src/codecs.cpp
    src/config_store.cpp
    src/contacts.cpp
    src/conversation_recorder.cpp
    src/countries.cpp
    src/credentials.cpp
    src/dial_link.cpp
    src/engine.cpp
    src/event_loop.cpp
    src/exchange_log.cpp
    src/http.cpp
    src/identity.cpp
    src/jobs.cpp
    src/generation_plan.cpp
    src/generation_service.cpp
    src/llm.cpp
    src/llm_dialect.cpp
    src/level_meter.cpp
    src/library_export.cpp
    src/local_hosts.cpp
    src/logging.cpp
    src/metrics.cpp
    src/paths.cpp
    src/pipeline_engine.cpp
    src/provider_knobs.cpp
    src/providers.cpp
    ${SOFTPHONE_PATHS}
    src/registration.cpp
    src/text_search.cpp
    src/recording_controller.cpp
    src/recordings.cpp
    src/recording_library.cpp
    src/recording_ring.cpp
    src/recording_taps.cpp
    src/redial.cpp
    src/retention.cpp
    src/registration_controller.cpp
    src/ringtones.cpp
    src/settings.cpp
    src/settings_groups.cpp
    src/sip_trace.cpp
    src/settings_controller.cpp
    src/shipped_content.cpp
    ${SOFTPHONE_CONTENT_SOURCE}
    src/store.cpp
    src/taxonomy.cpp
    src/templates.cpp
    src/progress.cpp
    src/telemetry_facts.cpp
    src/telemetry_service.cpp
    src/telemetry_store.cpp
    src/time_text.cpp
    src/transcriber.cpp
    src/transcript_export.cpp
    src/transcription_service.cpp
    src/transcripts.cpp
    src/timeline_writer.cpp
    src/tones.cpp
    src/turn_engine.cpp
    src/uuid.cpp
    src/updates.cpp
    ${SOFTPHONE_UPDATE_INSTALLER}
    ${SOFTPHONE_CRASH}
    src/version.cpp
    src/webhook_events.cpp
    src/webhook_notifier.cpp
    src/webhooks.cpp
    src/realtime_purse.cpp
    src/realtime_reader.cpp
    src/rehearsal_feed.cpp
    src/streamed_transcription.cpp
    src/realtime_ring.cpp
    src/resample.cpp
    src/streaming_asr.cpp
    src/web_socket.cpp)

add_library(softphone::core ALIAS softphone_core)

target_include_directories(softphone_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

target_compile_features(softphone_core PUBLIC cxx_std_20)

target_compile_definitions(softphone_core
    PRIVATE
        SOFTPHONE_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        SOFTPHONE_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        SOFTPHONE_VERSION_PATCH=${PROJECT_VERSION_PATCH}
        SOFTPHONE_VERSION_STRING="${PROJECT_VERSION}"

        SOFTPHONE_UPDATE_MANIFEST_URLS="${SOFTPHONE_UPDATE_MANIFEST_URLS}"
        SOFTPHONE_UPDATE_MANIFEST_URLS_BY_COUNTRY="${SOFTPHONE_UPDATE_MANIFEST_URLS_BY_COUNTRY}"
        SOFTPHONE_UPDATE_PUBLIC_KEY="${SOFTPHONE_UPDATE_PUBLIC_KEY}"
        SOFTPHONE_PACKAGE_FORMAT="${SOFTPHONE_PACKAGE_FORMAT}"

        SOFTPHONE_TELEMETRY_REPORTS=${SOFTPHONE_TELEMETRY_REPORTS})

target_link_libraries(softphone_core
    PRIVATE softphone::warnings nlohmann_json::nlohmann_json
    PUBLIC softphone::coverage Threads::Threads)

if(WIN32)

    target_link_libraries(softphone_core PRIVATE shell32 advapi32 dbghelp)
endif()

if(APPLE)
    target_link_libraries(softphone_core PRIVATE "-framework CoreFoundation")
endif()
