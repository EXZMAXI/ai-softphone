# AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
# https://ai-softphone.com
#
# SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
# SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of AI Softphone. There is NO WARRANTY, to the extent
# permitted by law. See the file LICENSE for the full terms.

add_library(softphone_plugin_processing ${SOFTPHONE_LIBRARY_KIND}
    src/plugin.cpp)

add_library(softphone::plugin::processing ALIAS softphone_plugin_processing)

target_include_directories(softphone_plugin_processing
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

target_link_libraries(softphone_plugin_processing
    PUBLIC
        softphone::container
        softphone::core
        softphone::plugin::storage
        softphone::plugin::transcription
        softphone::coverage
    PRIVATE
        softphone::warnings)

if(SOFTPHONE_BUILD_UI)
    find_package(Qt6 6.8 REQUIRED COMPONENTS Core Quick LinguistTools)
    qt_standard_project_setup(REQUIRES 6.8)

    qt_add_library(softphone_plugin_processing_ui ${SOFTPHONE_LIBRARY_KIND}
        src/ui.cpp)

    add_library(softphone::plugin::processing::ui ALIAS softphone_plugin_processing_ui)

    target_include_directories(softphone_plugin_processing_ui
        PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

    target_link_libraries(softphone_plugin_processing_ui
        PUBLIC
            softphone::plugin::processing

            softphone::plugin::mediastorage::api
            softphone::shell
            softphone::coverage
        PRIVATE
            softphone::warnings)

    qt_add_qml_module(softphone_plugin_processing_ui
        URI Softphone.Processing
        VERSION 1.0
        NO_PLUGIN
        OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Softphone/Processing
        QML_FILES
            qml/AnswerPanel.qml
            qml/SettingsProcessing.qml)

    softphone_translation_catalogue(SOFTPHONE_PROCESSING_TS processing)

    qt_add_lupdate(
        SOURCE_TARGETS softphone_plugin_processing_ui
        TS_FILES ${SOFTPHONE_PROCESSING_TS}
        OPTIONS ${SOFTPHONE_LUPDATE_OPTIONS}
        LUPDATE_TARGET update_processing_translations)

    qt_add_lrelease(
        TS_FILES ${SOFTPHONE_PROCESSING_TS}
        QM_FILES_OUTPUT_VARIABLE SOFTPHONE_PROCESSING_QM
        LRELEASE_TARGET release_processing_translations)

    qt_add_resources(softphone_plugin_processing_ui "processing_i18n"
        PREFIX "/i18n/processing"
        BASE "${CMAKE_CURRENT_BINARY_DIR}"
        FILES ${SOFTPHONE_PROCESSING_QM})
endif()

softphone_plugin_in_a_file(processing)
