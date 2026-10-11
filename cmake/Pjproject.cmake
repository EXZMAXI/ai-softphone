# AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
# https://ai-softphone.com
#
# SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
# SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of AI Softphone. There is NO WARRANTY, to the extent
# permitted by law. See the file LICENSE for the full terms.

set(SOFTPHONE_PJPROJECT_VERSION "2.17")

if(NOT SOFTPHONE_PJPROJECT_PREFIX)
    if(DEFINED ENV{SOFTPHONE_PJPROJECT_PREFIX})
        set(SOFTPHONE_PJPROJECT_PREFIX "$ENV{SOFTPHONE_PJPROJECT_PREFIX}")
    else()
        if(DEFINED ENV{SOFTPHONE_CACHE_DIR})
            set(_softphone_cache_root "$ENV{SOFTPHONE_CACHE_DIR}")
        elseif(WIN32)
            set(_softphone_cache_root "$ENV{LOCALAPPDATA}/softphone-ai-deps")
        else()
            set(_softphone_cache_root "$ENV{HOME}/.cache/softphone-ai-deps")
        endif()

        set(SOFTPHONE_PJPROJECT_PREFIX
            "${_softphone_cache_root}/pjproject-${SOFTPHONE_PJPROJECT_VERSION}")
    endif()
endif()

add_library(softphone_pjproject INTERFACE)
add_library(softphone::pjproject ALIAS softphone_pjproject)

if(WIN32)
    set(_softphone_pjproject_marker "${SOFTPHONE_PJPROJECT_PREFIX}/include/pjsua2.hpp")

    if(NOT EXISTS "${_softphone_pjproject_marker}")
        message(FATAL_ERROR
            "pjproject ${SOFTPHONE_PJPROJECT_VERSION} was not found at:\n"
            "  ${SOFTPHONE_PJPROJECT_PREFIX}\n"
            "Build it first:\n"
            "  powershell -ExecutionPolicy Bypass -File scripts/build-pjproject.ps1")
    endif()

    include(AsciiPath)
    softphone_ascii_path(SOFTPHONE_PJPROJECT_PREFIX "pjproject")

    target_include_directories(softphone_pjproject SYSTEM
        INTERFACE "${SOFTPHONE_PJPROJECT_PREFIX}/include")

    target_link_libraries(softphone_pjproject INTERFACE
        "$<IF:$<CONFIG:Debug>,${SOFTPHONE_PJPROJECT_PREFIX}/lib/Debug/libpjproject.lib,${SOFTPHONE_PJPROJECT_PREFIX}/lib/Release/libpjproject.lib>")

    target_link_libraries(softphone_pjproject INTERFACE
        ws2_32
        mswsock
        iphlpapi
        dnsapi
        secur32
        crypt32
        ole32
        oleaut32
        uuid
        strmiids
        winmm
        dsound
        dxguid
        advapi32
        user32
        gdi32
        netapi32)

    message(STATUS
        "pjproject ${SOFTPHONE_PJPROJECT_VERSION} from ${SOFTPHONE_PJPROJECT_PREFIX}")
else()
    find_package(PkgConfig REQUIRED)

    if(NOT EXISTS "${SOFTPHONE_PJPROJECT_PREFIX}/lib/pkgconfig/libpjproject.pc")
        message(FATAL_ERROR
            "pjproject ${SOFTPHONE_PJPROJECT_VERSION} was not found at:\n"
            "  ${SOFTPHONE_PJPROJECT_PREFIX}\n"
            "Build it first:\n"
            "  ./scripts/build-pjproject.sh")
    endif()

    if(APPLE AND NOT SOFTPHONE_OPUS_PREFIX)
        if(DEFINED ENV{SOFTPHONE_OPUS_PREFIX})
            set(SOFTPHONE_OPUS_PREFIX "$ENV{SOFTPHONE_OPUS_PREFIX}")
        elseif(DEFINED ENV{SOFTPHONE_CACHE_DIR})
            set(SOFTPHONE_OPUS_PREFIX "$ENV{SOFTPHONE_CACHE_DIR}/opus-1.5.2")
        else()

            set(SOFTPHONE_OPUS_PREFIX "$ENV{HOME}/.cache/softphone-ai-deps/opus-1.5.2")
        endif()
    endif()

    set(ENV{PKG_CONFIG_PATH}
        "${SOFTPHONE_PJPROJECT_PREFIX}/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
    pkg_check_modules(PJPROJECT REQUIRED libpjproject)

    target_include_directories(softphone_pjproject SYSTEM
        INTERFACE ${PJPROJECT_STATIC_INCLUDE_DIRS})

    target_compile_options(softphone_pjproject INTERFACE ${PJPROJECT_STATIC_CFLAGS_OTHER})

    set(_softphone_pjproject_link ${PJPROJECT_STATIC_LDFLAGS})

    if(APPLE)
        set(_softphone_corrected "")
        set(_softphone_framework OFF)
        foreach(item IN LISTS _softphone_pjproject_link)
            if(_softphone_framework)
                list(APPEND _softphone_corrected "-framework ${item}")
                set(_softphone_framework OFF)
            elseif(item STREQUAL "-framework")
                set(_softphone_framework ON)
            elseif(item STREQUAL "-lopus")
                list(APPEND _softphone_corrected "${SOFTPHONE_OPUS_PREFIX}/lib/libopus.a")
            elseif(item STREQUAL "-lssl")
                list(APPEND _softphone_corrected "${OPENSSL_ROOT_DIR}/lib/libssl.a")
            elseif(item STREQUAL "-lcrypto")
                list(APPEND _softphone_corrected "${OPENSSL_ROOT_DIR}/lib/libcrypto.a")
            else()
                list(APPEND _softphone_corrected "${item}")
            endif()
        endforeach()

        list(REMOVE_DUPLICATES _softphone_corrected)
        set(_softphone_pjproject_link ${_softphone_corrected})
    endif()

    target_link_libraries(softphone_pjproject INTERFACE ${_softphone_pjproject_link})

    message(STATUS "pjproject ${PJPROJECT_VERSION} from ${SOFTPHONE_PJPROJECT_PREFIX}")
endif()
