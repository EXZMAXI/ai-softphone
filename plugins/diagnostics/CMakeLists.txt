# AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
# https://ai-softphone.com
#
# SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
# SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of AI Softphone. There is NO WARRANTY, to the extent
# permitted by law. See the file LICENSE for the full terms.

add_library(softphone_plugin_diagnostics ${SOFTPHONE_LIBRARY_KIND}
    src/plugin.cpp)

add_library(softphone::plugin::diagnostics ALIAS softphone_plugin_diagnostics)

target_include_directories(softphone_plugin_diagnostics
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

target_link_libraries(softphone_plugin_diagnostics
    PUBLIC
        softphone::container
        softphone::core
        softphone::plugin::integration
        softphone::coverage
    PRIVATE
        softphone::warnings)

if(SOFTPHONE_BUILD_UI)
    find_package(Qt6 6.8 REQUIRED COMPONENTS Core Quick LinguistTools)
    qt_standard_project_setup(REQUIRES 6.8)

    qt_add_library(softphone_plugin_diagnostics_ui ${SOFTPHONE_LIBRARY_KIND}

        src/ui.cpp)

    add_library(softphone::plugin::diagnostics::ui ALIAS softphone_plugin_diagnostics_ui)

    target_include_directories(softphone_plugin_diagnostics_ui
        PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

    target_link_libraries(softphone_plugin_diagnostics_ui
        PUBLIC
            softphone::plugin::diagnostics
            softphone::shell
            softphone::container
            softphone::coverage
        PRIVATE
            softphone::warnings)

    qt_add_qml_module(softphone_plugin_diagnostics_ui
        URI Softphone.Diagnostics
        VERSION 1.0
        NO_PLUGIN
        OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Softphone/Diagnostics
        QML_FILES
            qml/DiagnosticsBody.qml
            qml/SettingsDiagnostics.qml)

    softphone_translation_catalogue(SOFTPHONE_DIAGNOSTICS_TS diagnostics)

    qt_add_lupdate(
        SOURCE_TARGETS softphone_plugin_diagnostics_ui
        TS_FILES ${SOFTPHONE_DIAGNOSTICS_TS}
        OPTIONS ${SOFTPHONE_LUPDATE_OPTIONS}
        LUPDATE_TARGET update_diagnostics_translations)

    qt_add_lrelease(
        TS_FILES ${SOFTPHONE_DIAGNOSTICS_TS}
        QM_FILES_OUTPUT_VARIABLE SOFTPHONE_DIAGNOSTICS_QM
        LRELEASE_TARGET release_diagnostics_translations)

    qt_add_resources(softphone_plugin_diagnostics_ui "diagnostics_i18n"
        PREFIX "/i18n/diagnostics"
        BASE "${CMAKE_CURRENT_BINARY_DIR}"
        FILES ${SOFTPHONE_DIAGNOSTICS_QM})
endif()
