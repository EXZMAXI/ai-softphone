# AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
# https://ai-softphone.com
#
# SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
# SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of AI Softphone. There is NO WARRANTY, to the extent
# permitted by law. See the file LICENSE for the full terms.

set(SOFTPHONE_OPENSSL_VERSION "3.5.7")

function(softphone_carry_openssl)
    if(NOT WIN32 OR NOT SOFTPHONE_OPENSSL_RUNTIME_LIBS)
        return()
    endif()

    file(COPY ${SOFTPHONE_OPENSSL_RUNTIME_LIBS} DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
    if(CMAKE_RUNTIME_OUTPUT_DIRECTORY)
        file(COPY ${SOFTPHONE_OPENSSL_RUNTIME_LIBS}
             DESTINATION "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
    endif()
endfunction()

if(APPLE)
    set(SOFTPHONE_MACOS_OPENSSL
        "$ENV{HOME}/.cache/softphone-ai-deps/openssl-${SOFTPHONE_OPENSSL_VERSION}")
    if(DEFINED ENV{SOFTPHONE_OPENSSL_PREFIX})
        set(SOFTPHONE_MACOS_OPENSSL "$ENV{SOFTPHONE_OPENSSL_PREFIX}")
    elseif(DEFINED ENV{SOFTPHONE_CACHE_DIR})
        set(SOFTPHONE_MACOS_OPENSSL
            "$ENV{SOFTPHONE_CACHE_DIR}/openssl-${SOFTPHONE_OPENSSL_VERSION}")
    endif()

    if(NOT OPENSSL_ROOT_DIR AND EXISTS "${SOFTPHONE_MACOS_OPENSSL}/include/openssl/ssl.h")
        set(OPENSSL_ROOT_DIR "${SOFTPHONE_MACOS_OPENSSL}")

        set(OPENSSL_USE_STATIC_LIBS ON)
        message(STATUS "OpenSSL ${SOFTPHONE_OPENSSL_VERSION} from ${OPENSSL_ROOT_DIR}")
    elseif(NOT OPENSSL_ROOT_DIR)
        message(STATUS
            "No OpenSSL at ${SOFTPHONE_MACOS_OPENSSL}: run ./scripts/build-macos-deps.sh")
    endif()

    return()
endif()

if(NOT WIN32)
    return()
endif()

if(NOT SOFTPHONE_OPENSSL_PREFIX)
    if(DEFINED ENV{SOFTPHONE_OPENSSL_PREFIX})
        set(SOFTPHONE_OPENSSL_PREFIX "$ENV{SOFTPHONE_OPENSSL_PREFIX}")
    else()
        if(DEFINED ENV{SOFTPHONE_CACHE_DIR})
            set(_softphone_cache_root "$ENV{SOFTPHONE_CACHE_DIR}")
        else()
            set(_softphone_cache_root "$ENV{LOCALAPPDATA}/softphone-ai-deps")
        endif()

        set(SOFTPHONE_OPENSSL_PREFIX
            "${_softphone_cache_root}/openssl-${SOFTPHONE_OPENSSL_VERSION}")
    endif()
endif()

file(TO_CMAKE_PATH "${SOFTPHONE_OPENSSL_PREFIX}" SOFTPHONE_OPENSSL_PREFIX)

if(NOT OPENSSL_ROOT_DIR AND EXISTS "${SOFTPHONE_OPENSSL_PREFIX}/include/openssl/ssl.h")

    include(AsciiPath)
    softphone_ascii_path(SOFTPHONE_OPENSSL_PREFIX "OpenSSL")

    set(OPENSSL_ROOT_DIR "${SOFTPHONE_OPENSSL_PREFIX}")
endif()

if(SOFTPHONE_PACKAGE AND NOT OPENSSL_ROOT_DIR)
    message(FATAL_ERROR
        "A Windows package needs OpenSSL and none was found at:\n"
        "  ${SOFTPHONE_OPENSSL_PREFIX}\n"
        "Build it with:\n"
        "  powershell -ExecutionPolicy Bypass -File scripts/build-openssl.ps1\n"
        "or point OPENSSL_ROOT_DIR at one you already have.\n"
        "Without it the package would refuse https:// webhooks and could not "
        "verify a release, so it would carry an update channel it can never read.")
endif()

set(SOFTPHONE_OPENSSL_RUNTIME_LIBS "")
foreach(_openssl_dll libcrypto-3-x64.dll libssl-3-x64.dll)
    if(OPENSSL_ROOT_DIR AND EXISTS "${OPENSSL_ROOT_DIR}/bin/${_openssl_dll}")
        list(APPEND SOFTPHONE_OPENSSL_RUNTIME_LIBS "${OPENSSL_ROOT_DIR}/bin/${_openssl_dll}")
    elseif(SOFTPHONE_PACKAGE)
        message(FATAL_ERROR
            "OpenSSL at ${OPENSSL_ROOT_DIR} has no bin/${_openssl_dll}. "
            "A package that links OpenSSL and does not carry it starts on this "
            "machine and on no other.")
    endif()
endforeach()
