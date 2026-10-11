# AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
# https://ai-softphone.com
#
# SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
# SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of AI Softphone. There is NO WARRANTY, to the extent
# permitted by law. See the file LICENSE for the full terms.

function(softphone_add_windows_resource target)
    if(NOT WIN32)
        return()
    endif()

    cmake_parse_arguments(ARG "" "NAME;DESCRIPTION;ICON;KIND" "" ${ARGN})

    if(ARG_ICON)

        set(SOFTPHONE_RC_ICON_LINE "IDI_ICON1 ICON \"${ARG_ICON}\"")
    else()
        set(SOFTPHONE_RC_ICON_LINE "")
    endif()

    set(SOFTPHONE_RC_NAME "${ARG_NAME}")
    set(SOFTPHONE_RC_DESCRIPTION "${ARG_DESCRIPTION}")

    if(ARG_KIND STREQUAL "DLL")
        set(SOFTPHONE_RC_FILETYPE "VFT_DLL")
        set(SOFTPHONE_RC_FILENAME "${ARG_NAME}.dll")
    else()
        set(SOFTPHONE_RC_FILETYPE "VFT_APP")
        set(SOFTPHONE_RC_FILENAME "${ARG_NAME}.exe")
    endif()

    set(_generated "${CMAKE_CURRENT_BINARY_DIR}/${ARG_NAME}-version.rc")
    configure_file("${CMAKE_SOURCE_DIR}/cmake/windows-version.rc.in" "${_generated}" @ONLY)

    target_sources(${target} PRIVATE "${_generated}")
endfunction()

function(_softphone_targets_under dir out)
    get_property(_here DIRECTORY "${dir}" PROPERTY BUILDSYSTEM_TARGETS)
    get_property(_below DIRECTORY "${dir}" PROPERTY SUBDIRECTORIES)

    set(_found ${_here})
    foreach(_one IN LISTS _below)
        _softphone_targets_under("${_one}" _deeper)
        list(APPEND _found ${_deeper})
    endforeach()

    set(${out} ${_found} PARENT_SCOPE)
endfunction()

function(_softphone_describe target out)
    if(target MATCHES "^softphone_plugin_(.+)_ui$")
        set(${out} "AI Softphone - the ${CMAKE_MATCH_1} module's interface" PARENT_SCOPE)
    elseif(target MATCHES "^softphone_plugin_(.+)$")
        set(${out} "AI Softphone - the ${CMAKE_MATCH_1} module" PARENT_SCOPE)
    elseif(target MATCHES "^softphone_(.+)$")
        set(${out} "AI Softphone - the ${CMAKE_MATCH_1} library" PARENT_SCOPE)
    else()
        set(${out} "AI Softphone" PARENT_SCOPE)
    endif()
endfunction()

function(softphone_version_every_library)
    if(NOT WIN32)
        return()
    endif()

    _softphone_targets_under("${CMAKE_SOURCE_DIR}" _targets)
    foreach(_target IN LISTS _targets)
        if(NOT _target MATCHES "^softphone_")
            continue()
        endif()

        get_target_property(_kind ${_target} TYPE)
        if(NOT _kind STREQUAL "SHARED_LIBRARY" AND NOT _kind STREQUAL "MODULE_LIBRARY")
            continue()
        endif()

        _softphone_describe(${_target} _description)
        softphone_add_windows_resource(${_target}
            NAME ${_target}
            DESCRIPTION "${_description}"
            KIND DLL)
    endforeach()
endfunction()
