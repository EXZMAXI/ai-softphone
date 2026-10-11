# AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
# https://ai-softphone.com
#
# SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
# SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of AI Softphone. There is NO WARRANTY, to the extent
# permitted by law. See the file LICENSE for the full terms.

set(SOFTPHONE_PLUGIN_TREE "lib/ai-softphone/plugins")

if(APPLE AND SOFTPHONE_PACKAGE)
    set(SOFTPHONE_PLUGIN_TREE "ai-softphone.app/Contents/Resources/ai-softphone")
    set(SOFTPHONE_PLUGIN_RPATH
        "@loader_path;@loader_path/..;@loader_path/../../../Frameworks")
else()
    set(SOFTPHONE_PLUGIN_RPATH
        "${SOFTPHONE_ORIGIN}/..;${SOFTPHONE_ORIGIN}/../../..;${SOFTPHONE_ORIGIN}/../../../../${CMAKE_INSTALL_LIBDIR}")
endif()

set(SOFTPHONE_PLUGIN_BUILD_TREE "${CMAKE_BINARY_DIR}/${SOFTPHONE_PLUGIN_TREE}")

function(softphone_plugin_in_a_file id)
    set(tree "${SOFTPHONE_PLUGIN_BUILD_TREE}/${id}")

    set(halves softphone_plugin_${id})
    if(TARGET softphone_plugin_${id}_ui)
        list(APPEND halves softphone_plugin_${id}_ui)
    endif()

    foreach(half IN LISTS halves)
        set_target_properties(${half} PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY ${tree}
            RUNTIME_OUTPUT_DIRECTORY ${tree}

            INSTALL_RPATH "${SOFTPHONE_PLUGIN_RPATH}"

            INSTALL_NAME_DIR "@rpath/${id}")
    endforeach()

    if(WIN32)
        foreach(half IN LISTS halves)
            add_custom_command(TARGET ${half} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        $<TARGET_FILE:${half}> ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
                COMMENT "Copying ${half} beside the binaries that link it"
                VERBATIM)
        endforeach()
    endif()

    add_library(softphone_plugin_${id}_api INTERFACE)
    add_library(softphone::plugin::${id}::api ALIAS softphone_plugin_${id}_api)
    target_include_directories(softphone_plugin_${id}_api
        INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

    add_custom_command(TARGET softphone_plugin_${id} POST_BUILD
        COMMAND softphone_plugin_manifest
                $<TARGET_FILE:softphone_plugin_${id}>
                ${tree}/manifest.json
        BYPRODUCTS ${tree}/manifest.json
        COMMENT "Writing the manifest of the ${id} plugin"
        VERBATIM)

    add_dependencies(softphone_plugin_${id} softphone_plugin_manifest)

    install(TARGETS ${halves}
        LIBRARY DESTINATION ${SOFTPHONE_PLUGIN_TREE}/${id}
        RUNTIME DESTINATION ${SOFTPHONE_PLUGIN_TREE}/${id}
        ARCHIVE DESTINATION ${SOFTPHONE_PLUGIN_TREE}/${id})

    install(FILES ${tree}/manifest.json
        DESTINATION ${SOFTPHONE_PLUGIN_TREE}/${id})

    if(MSVC)
        foreach(half IN LISTS halves)
            install(FILES $<TARGET_PDB_FILE:${half}>
                    DESTINATION ${SOFTPHONE_PLUGIN_TREE}/${id}
                    OPTIONAL)
        endforeach()
    endif()
endfunction()
