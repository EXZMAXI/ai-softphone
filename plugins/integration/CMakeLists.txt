# AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
# https://ai-softphone.com
#
# SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
# SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of AI Softphone. There is NO WARRANTY, to the extent
# permitted by law. See the file LICENSE for the full terms.

add_library(softphone_plugin_integration ${SOFTPHONE_LIBRARY_KIND}
    src/api_host.cpp
    src/plugin.cpp)

add_library(softphone::plugin::integration ALIAS softphone_plugin_integration)

target_include_directories(softphone_plugin_integration
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

target_link_libraries(softphone_plugin_integration
    PUBLIC
        softphone::container
        softphone::core
        softphone::http
        softphone::coverage
    PRIVATE
        softphone::warnings)

if(SOFTPHONE_BUILD_UI)
    find_package(Qt6 6.8 REQUIRED COMPONENTS Core Quick LinguistTools)
    qt_standard_project_setup(REQUIRES 6.8)

    qt_add_library(softphone_plugin_integration_ui ${SOFTPHONE_LIBRARY_KIND}
        src/ui.cpp)

    add_library(softphone::plugin::integration::ui ALIAS softphone_plugin_integration_ui)

    target_include_directories(softphone_plugin_integration_ui
        PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

    target_link_libraries(softphone_plugin_integration_ui
        PUBLIC
            softphone::plugin::integration
            softphone::shell
            softphone::coverage
        PRIVATE
            softphone::warnings)

    qt_add_qml_module(softphone_plugin_integration_ui
        URI Softphone.Integration
        VERSION 1.0
        NO_PLUGIN
        OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Softphone/Integration
        QML_FILES
            qml/SettingsIntegration.qml)

    softphone_translation_catalogue(SOFTPHONE_INTEGRATION_TS integration)

    qt_add_lupdate(
        SOURCE_TARGETS softphone_plugin_integration_ui
        TS_FILES ${SOFTPHONE_INTEGRATION_TS}
        OPTIONS ${SOFTPHONE_LUPDATE_OPTIONS}
        LUPDATE_TARGET update_integration_translations)

    qt_add_lrelease(
        TS_FILES ${SOFTPHONE_INTEGRATION_TS}
        QM_FILES_OUTPUT_VARIABLE SOFTPHONE_INTEGRATION_QM
        LRELEASE_TARGET release_integration_translations)

    qt_add_resources(softphone_plugin_integration_ui "integration_i18n"
        PREFIX "/i18n/integration"
        BASE "${CMAKE_CURRENT_BINARY_DIR}"
        FILES ${SOFTPHONE_INTEGRATION_QM})
endif()
