# AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
# https://ai-softphone.com
#
# SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
# SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of AI Softphone. There is NO WARRANTY, to the extent
# permitted by law. See the file LICENSE for the full terms.

add_library(softphone_plugin_webui ${SOFTPHONE_LIBRARY_KIND}
    src/pages.cpp
    src/words.cpp
    src/plugin.cpp)

add_library(softphone::plugin::webui ALIAS softphone_plugin_webui)

target_include_directories(softphone_plugin_webui
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

target_link_libraries(softphone_plugin_webui
    PUBLIC
        softphone::container
        softphone::core
        softphone::plugin::integration
        softphone::plugin::mediastorage::api
        softphone::plugin::transcription::api
        softphone::coverage
    PRIVATE
        softphone::warnings)

if(SOFTPHONE_BUILD_UI)
    find_package(Qt6 6.8 REQUIRED COMPONENTS Core Quick LinguistTools)
    qt_standard_project_setup(REQUIRES 6.8)

    qt_add_library(softphone_plugin_webui_ui ${SOFTPHONE_LIBRARY_KIND}
        src/ui.cpp)

    add_library(softphone::plugin::webui::ui ALIAS softphone_plugin_webui_ui)

    target_include_directories(softphone_plugin_webui_ui
        PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

    target_link_libraries(softphone_plugin_webui_ui
        PUBLIC
            softphone::plugin::webui
            softphone::shell
            softphone::coverage
        PRIVATE
            softphone::warnings)

    qt_add_qml_module(softphone_plugin_webui_ui
        URI Softphone.WebUi
        VERSION 1.0
        NO_PLUGIN
        OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Softphone/WebUi
        QML_FILES
            qml/OpenInBrowserAct.qml)

    softphone_translation_catalogue(SOFTPHONE_WEBUI_TS webui)

    qt_add_lupdate(
        SOURCE_TARGETS softphone_plugin_webui_ui
        TS_FILES ${SOFTPHONE_WEBUI_TS}
        OPTIONS ${SOFTPHONE_LUPDATE_OPTIONS}
        LUPDATE_TARGET update_webui_translations)

    qt_add_lrelease(
        TS_FILES ${SOFTPHONE_WEBUI_TS}
        QM_FILES_OUTPUT_VARIABLE SOFTPHONE_WEBUI_QM
        LRELEASE_TARGET release_webui_translations)

    qt_add_resources(softphone_plugin_webui_ui "webui_i18n"
        PREFIX "/i18n/webui"
        BASE "${CMAKE_CURRENT_BINARY_DIR}"
        FILES ${SOFTPHONE_WEBUI_QM})
endif()

softphone_plugin_in_a_file(webui)
