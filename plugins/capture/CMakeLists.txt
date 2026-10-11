# AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
# https://ai-softphone.com
#
# SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
# SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of AI Softphone. There is NO WARRANTY, to the extent
# permitted by law. See the file LICENSE for the full terms.

add_library(softphone_plugin_capture ${SOFTPHONE_LIBRARY_KIND}
    src/control.cpp
    src/controller.cpp
    src/plugin.cpp)

add_library(softphone::plugin::capture ALIAS softphone_plugin_capture)

target_include_directories(softphone_plugin_capture
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

target_link_libraries(softphone_plugin_capture
    PUBLIC
        softphone::container
        softphone::core
        softphone::capture
        softphone::plugin::mediastorage
        softphone::coverage
    PRIVATE
        softphone::warnings
        softphone::media)

if(SOFTPHONE_BUILD_UI)
    find_package(Qt6 6.8 REQUIRED COMPONENTS Core Quick LinguistTools)
    qt_standard_project_setup(REQUIRES 6.8)

    qt_add_library(softphone_plugin_capture_ui ${SOFTPHONE_LIBRARY_KIND}
        src/ui.cpp)

    add_library(softphone::plugin::capture::ui ALIAS softphone_plugin_capture_ui)

    target_include_directories(softphone_plugin_capture_ui
        PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

    target_link_libraries(softphone_plugin_capture_ui
        PUBLIC
            softphone::plugin::capture
            softphone::shell
            softphone::coverage
        PRIVATE
            softphone::warnings)

    qt_add_qml_module(softphone_plugin_capture_ui
        URI Softphone.Capture
        VERSION 1.0
        NO_PLUGIN
        OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Softphone/Capture
        QML_FILES
            qml/CapturePage.qml
            qml/CaptureBar.qml
            qml/CaptureStrip.qml
            qml/NameCapture.qml
            qml/SettingsCapture.qml)

    softphone_translation_catalogue(SOFTPHONE_CAPTURE_TS capture)

    qt_add_lupdate(
        SOURCE_TARGETS softphone_plugin_capture_ui
        TS_FILES ${SOFTPHONE_CAPTURE_TS}
        OPTIONS ${SOFTPHONE_LUPDATE_OPTIONS}
        LUPDATE_TARGET update_capture_translations)

    qt_add_lrelease(
        TS_FILES ${SOFTPHONE_CAPTURE_TS}
        QM_FILES_OUTPUT_VARIABLE SOFTPHONE_CAPTURE_QM
        LRELEASE_TARGET release_capture_translations)

    qt_add_resources(softphone_plugin_capture_ui "capture_i18n"
        PREFIX "/i18n/capture"
        BASE "${CMAKE_CURRENT_BINARY_DIR}"
        FILES ${SOFTPHONE_CAPTURE_QM})
endif()

softphone_plugin_in_a_file(capture)

target_include_directories(softphone_plugin_capture_api
    INTERFACE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/capture/include>)
