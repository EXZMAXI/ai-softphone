# AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
# https://ai-softphone.com
#
# SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
# SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of AI Softphone. There is NO WARRANTY, to the extent
# permitted by law. See the file LICENSE for the full terms.

set(SOFTPHONE_UPDATE_MANIFEST_URLS ""
    CACHE STRING "Every place the signed release manifest is published, space separated; empty means this build never looks")

set(SOFTPHONE_UPDATE_MANIFEST_URLS_BY_COUNTRY ""
    CACHE STRING "Manifest URLs a country adds, as <ISO 3166-1 alpha-2>=<URL>, space separated")

if(SOFTPHONE_UPDATE_MANIFEST_URL)
    message(FATAL_ERROR
        "SOFTPHONE_UPDATE_MANIFEST_URL is no longer read: this build asks every host it is given "
        "and believes the newest release any of them can prove. Pass SOFTPHONE_UPDATE_MANIFEST_URLS "
        "instead, space separated.")
endif()

set(SOFTPHONE_UPDATE_PUBLIC_KEY ""
    CACHE STRING "Base64 Ed25519 public key that release manifests are signed with")

function(softphone_publishes name type doc)
    set(_default "${ARGN}")
    set(_remembered "SOFTPHONE_DEFAULT_OF_${name}")

    if(DEFINED CACHE{${name}} AND DEFINED CACHE{${_remembered}}
       AND "$CACHE{${name}}" STREQUAL "$CACHE{${_remembered}}"
       AND NOT "$CACHE{${name}}" STREQUAL "${_default}")
        message(STATUS "${name}: this cache held the previous default; following the new one")
        set(${name} "${_default}" CACHE ${type} "${doc}" FORCE)
    endif()

    set(${name} "${_default}" CACHE ${type} "${doc}")

    set(${_remembered} "${_default}" CACHE INTERNAL
        "What ${name} defaulted to when this cache was written")
endfunction()

softphone_publishes(SOFTPHONE_RELEASE_BASE_URLS STRING
    "Every place this project publishes its releases, without the platform directory"
    "https://ai-softphone.com/downloads"
    "https://ai-softphone.org/downloads")

softphone_publishes(SOFTPHONE_RELEASE_BASE_URLS_BY_COUNTRY STRING
    "Release bases a country adds to the list above, as <ISO 3166-1 alpha-2>=<base URL>"
    "RU=https://ai-softphone.ru/downloads")

softphone_publishes(SOFTPHONE_RELEASE_PUBLIC_KEY STRING
    "The public half of this project's release signing key"
    "UUvGh5Vn2NtlxUqcCSgxTHpyhn17HhETuZue929eebU=")

if(SOFTPHONE_PACKAGE
   AND NOT SOFTPHONE_UPDATE_MANIFEST_URLS
   AND NOT SOFTPHONE_UPDATE_PUBLIC_KEY)
    if(WIN32)
        set(SOFTPHONE_RELEASE_PLATFORM "windows")
    elseif(APPLE)
        set(SOFTPHONE_RELEASE_PLATFORM "macos")
    else()
        set(SOFTPHONE_RELEASE_PLATFORM "linux")
    endif()

    set(SOFTPHONE_UPDATE_MANIFEST_URLS "")
    foreach(base IN LISTS SOFTPHONE_RELEASE_BASE_URLS)
        if(SOFTPHONE_UPDATE_MANIFEST_URLS)
            string(APPEND SOFTPHONE_UPDATE_MANIFEST_URLS " ")
        endif()
        string(APPEND SOFTPHONE_UPDATE_MANIFEST_URLS
               "${base}/${SOFTPHONE_RELEASE_PLATFORM}/manifest.json")
    endforeach()
    set(SOFTPHONE_UPDATE_MANIFEST_URLS_BY_COUNTRY "")
    foreach(entry IN LISTS SOFTPHONE_RELEASE_BASE_URLS_BY_COUNTRY)

        string(FIND "${entry}" "=" divide)
        if(divide LESS 0)
            message(FATAL_ERROR
                "SOFTPHONE_RELEASE_BASE_URLS_BY_COUNTRY entry '${entry}' is not <code>=<url>")
        endif()
        string(SUBSTRING "${entry}" 0 ${divide} code)
        math(EXPR after "${divide} + 1")
        string(SUBSTRING "${entry}" ${after} -1 base)
        if(SOFTPHONE_UPDATE_MANIFEST_URLS_BY_COUNTRY)
            string(APPEND SOFTPHONE_UPDATE_MANIFEST_URLS_BY_COUNTRY " ")
        endif()
        string(APPEND SOFTPHONE_UPDATE_MANIFEST_URLS_BY_COUNTRY
               "${code}=${base}/${SOFTPHONE_RELEASE_PLATFORM}/manifest.json")
    endforeach()

    set(SOFTPHONE_UPDATE_PUBLIC_KEY "${SOFTPHONE_RELEASE_PUBLIC_KEY}")

    message(STATUS "Updates: ${SOFTPHONE_UPDATE_MANIFEST_URLS}")
    if(SOFTPHONE_UPDATE_MANIFEST_URLS_BY_COUNTRY)
        message(STATUS "Updates by country: ${SOFTPHONE_UPDATE_MANIFEST_URLS_BY_COUNTRY}")
    endif()
endif()

set(SOFTPHONE_PACKAGE_FORMAT "source"
    CACHE STRING "How this build is delivered: source, deb, appimage, installer or dmg")
set_property(CACHE SOFTPHONE_PACKAGE_FORMAT PROPERTY STRINGS source deb appimage installer dmg)

if(SOFTPHONE_UPDATE_MANIFEST_URLS AND NOT SOFTPHONE_UPDATE_PUBLIC_KEY)
    message(FATAL_ERROR
        "SOFTPHONE_UPDATE_MANIFEST_URLS is set without SOFTPHONE_UPDATE_PUBLIC_KEY. "
        "An unverifiable update channel is worse than none: see scripts/make-signing-key.sh.")
endif()
