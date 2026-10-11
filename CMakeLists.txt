# AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
# https://ai-softphone.com
#
# SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
# SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of AI Softphone. There is NO WARRANTY, to the extent
# permitted by law. See the file LICENSE for the full terms.

cmake_minimum_required(VERSION 3.24)

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "14.4"
        CACHE STRING "The oldest macOS this build will start on")

    if(SOFTPHONE_PACKAGE AND NOT CMAKE_OSX_ARCHITECTURES)
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
    endif()
endif()

project(ai-softphone
    VERSION 1.1.5
    DESCRIPTION "Cross-platform desktop AI softphone"
    LANGUAGES CXX)

if(WIN32)
    enable_language(RC)
endif()

if(APPLE)
    enable_language(OBJCXX)
    set(CMAKE_OBJCXX_STANDARD 20)
    set(CMAKE_OBJCXX_STANDARD_REQUIRED ON)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
    add_compile_options(/utf-8)

    add_compile_definitions(NOMINMAX)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

option(SOFTPHONE_BUILD_UI "Build the Qt Quick user interface" ON)
option(SOFTPHONE_BUILD_SIP "Build the pjsip backend (requires scripts/build-pjproject.sh)" ON)
option(SOFTPHONE_BUILD_SECRETS "Build the Secret Service keyring backend (requires libsecret)" ON)
option(SOFTPHONE_BUILD_STORAGE "Build the SQLite store for contacts and history" ON)
option(SOFTPHONE_BUILD_MEDIA "Build the MP3 encoder and decoder used by call recording" ON)
option(SOFTPHONE_BUILD_CAPTURE "Build third-party capture: the system's output and the microphone" ON)
option(SOFTPHONE_BUILD_HTTP "Build the HTTP client and server (webhooks and the local API)" ON)
option(SOFTPHONE_ENABLE_COVERAGE "Instrument the build for coverage reporting" OFF)
option(SOFTPHONE_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

option(SOFTPHONE_PACKAGE "Lay the install out as a self-contained tree, Qt included" OFF)

if(SOFTPHONE_PACKAGE)
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH OFF)
else()
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH ON)
endif()

set(SOFTPHONE_LIBRARY_KIND SHARED)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

if(WIN32)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif()

include(GNUInstallDirs)

if(APPLE)
    set(SOFTPHONE_ORIGIN "@loader_path")
else()
    set(SOFTPHONE_ORIGIN "$ORIGIN")
endif()

set(CMAKE_INSTALL_RPATH
    "${SOFTPHONE_ORIGIN};${SOFTPHONE_ORIGIN}/../lib;${SOFTPHONE_ORIGIN}/../${CMAKE_INSTALL_LIBDIR}")

if(APPLE AND SOFTPHONE_PACKAGE)
    set(SOFTPHONE_BUNDLE_CONTENTS "ai-softphone.app/Contents")
    set(CMAKE_INSTALL_BINDIR "${SOFTPHONE_BUNDLE_CONTENTS}/MacOS")
    set(CMAKE_INSTALL_LIBDIR "${SOFTPHONE_BUNDLE_CONTENTS}/Frameworks")

    set(CMAKE_INSTALL_RPATH
        "@loader_path;@executable_path/../Frameworks;@loader_path/../Frameworks;@executable_path/../Resources/ai-softphone")
endif()

include(InstallLayout)
include(CompilerWarnings)

if(MSVC)
    add_compile_options($<$<CONFIG:Release>:/Z7>)
    add_link_options($<$<CONFIG:Release>:/DEBUG>
                     $<$<CONFIG:Release>:/OPT:REF>
                     $<$<CONFIG:Release>:/OPT:ICF>)
endif()
include(WindowsResource)
include(Coverage)

include(Updates)

include(Telemetry)

include(OpenSsl)

add_subdirectory(core)

add_subdirectory(container)

include(Languages)

include(Plugins)
add_subdirectory(tools/plugin-manifest)
add_subdirectory(tools/screenshot-audio)

if(SOFTPHONE_BUILD_SIP)
    add_subdirectory(sip)
endif()

if(SOFTPHONE_BUILD_SECRETS)
    add_subdirectory(secrets)
endif()

if(SOFTPHONE_BUILD_STORAGE)
    add_subdirectory(storage)
endif()

if(SOFTPHONE_BUILD_MEDIA)
    add_subdirectory(media)
endif()

if(SOFTPHONE_BUILD_CAPTURE)
    add_subdirectory(capture)
endif()

if(SOFTPHONE_BUILD_STORAGE)
    add_subdirectory(plugins/storage)
endif()

if(SOFTPHONE_BUILD_STORAGE AND SOFTPHONE_BUILD_SIP AND SOFTPHONE_BUILD_SECRETS)
    add_subdirectory(plugins/softphone)
elseif(SOFTPHONE_BUILD_STORAGE)
    message(STATUS
        "No SIP stack or no keyring: the telephone plugin is not built.")
endif()

if(SOFTPHONE_BUILD_STORAGE)
    add_subdirectory(plugins/directory)
endif()

if(SOFTPHONE_BUILD_STORAGE)
    add_subdirectory(plugins/taxonomy)
endif()

if(SOFTPHONE_BUILD_STORAGE)
    add_subdirectory(plugins/mediastorage)
endif()

if(SOFTPHONE_BUILD_STORAGE AND TARGET softphone_plugin_softphone)
    add_subdirectory(plugins/recording)
endif()

if(SOFTPHONE_BUILD_STORAGE AND SOFTPHONE_BUILD_CAPTURE AND SOFTPHONE_BUILD_MEDIA)
    add_subdirectory(plugins/capture)
endif()

if(SOFTPHONE_BUILD_STORAGE)
    add_subdirectory(plugins/transcription)
endif()

if(SOFTPHONE_BUILD_STORAGE)
    add_subdirectory(plugins/processing)
endif()

if(SOFTPHONE_BUILD_STORAGE)
    add_subdirectory(plugins/realtime)
endif()

add_subdirectory(plugins/monolith)

add_subdirectory(plugins/blf)

if(SOFTPHONE_BUILD_UI)
    add_subdirectory(shell)
endif()

if(SOFTPHONE_BUILD_HTTP)
    add_subdirectory(http)

    add_subdirectory(plugins/integration)
endif()

if(SOFTPHONE_BUILD_HTTP)
    add_subdirectory(plugins/webui)
endif()

if(SOFTPHONE_BUILD_HTTP)
    add_subdirectory(plugins/diagnostics)
endif()

if(SOFTPHONE_BUILD_SIP AND SOFTPHONE_BUILD_SECRETS)
    add_subdirectory(cli)
elseif(SOFTPHONE_BUILD_SIP)
    message(STATUS "No keyring: the CLI is not built (it stores passwords in one)")
endif()

if(SOFTPHONE_BUILD_UI AND SOFTPHONE_BUILD_SECRETS)
    add_subdirectory(ui)
elseif(SOFTPHONE_BUILD_UI)
    message(STATUS "No keyring: the desktop shell is not built (it stores passwords in one)")
endif()

set(SOFTPHONE_INSTALLED_LIBRARIES
    softphone_container
    softphone_core
    softphone_http
    softphone_media
    softphone_secrets
    softphone_sip
    softphone_storage

    softphone_capture

    softphone_plugin_diagnostics
    softphone_plugin_integration
    softphone_plugin_monolith

    softphone_plugin_realtime
    softphone_plugin_softphone
    softphone_plugin_storage

    softphone_plugin_diagnostics_ui
    softphone_plugin_integration_ui
    softphone_plugin_softphone_ui)

set(SOFTPHONE_LIBRARIES_TO_INSTALL)
foreach(library IN LISTS SOFTPHONE_INSTALLED_LIBRARIES)
    if(TARGET ${library})
        list(APPEND SOFTPHONE_LIBRARIES_TO_INSTALL ${library})
    endif()
endforeach()

install(TARGETS ${SOFTPHONE_LIBRARIES_TO_INSTALL}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

if(MSVC)
    foreach(library IN LISTS SOFTPHONE_LIBRARIES_TO_INSTALL)
        install(FILES $<TARGET_PDB_FILE:${library}>
                DESTINATION ${CMAKE_INSTALL_BINDIR}
                OPTIONAL)
    endforeach()
endif()

softphone_version_every_library()

include(Packaging)
