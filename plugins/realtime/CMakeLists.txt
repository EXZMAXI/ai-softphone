# AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
# https://ai-softphone.com
#
# SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
# SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of AI Softphone. There is NO WARRANTY, to the extent
# permitted by law. See the file LICENSE for the full terms.

add_library(softphone_plugin_realtime ${SOFTPHONE_LIBRARY_KIND}
    src/plugin.cpp
    src/session.cpp)

add_library(softphone::plugin::realtime ALIAS softphone_plugin_realtime)

target_include_directories(softphone_plugin_realtime
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

target_link_libraries(softphone_plugin_realtime
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

    qt_add_library(softphone_plugin_realtime_ui ${SOFTPHONE_LIBRARY_KIND}
        src/ui.cpp)

    add_library(softphone::plugin::realtime::ui ALIAS softphone_plugin_realtime_ui)

    target_include_directories(softphone_plugin_realtime_ui
        PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

    target_link_libraries(softphone_plugin_realtime_ui
        PUBLIC
            softphone::plugin::realtime
            softphone::shell
            softphone::coverage
        PRIVATE
            softphone::warnings)

    qt_add_qml_module(softphone_plugin_realtime_ui
        URI Softphone.Realtime
        VERSION 1.0
        NO_PLUGIN
        OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Softphone/Realtime
        QML_FILES
            qml/Lane.qml
            qml/PrompterAct.qml
            qml/PrompterWindow.qml
            qml/RehearsalBar.qml
            qml/RehearsalChooser.qml
            qml/SettingsRealtime.qml
            qml/Tail.qml)

    softphone_translation_catalogue(SOFTPHONE_REALTIME_TS realtime)

    qt_add_lupdate(
        SOURCE_TARGETS softphone_plugin_realtime_ui
        TS_FILES ${SOFTPHONE_REALTIME_TS}
        OPTIONS ${SOFTPHONE_LUPDATE_OPTIONS}
        LUPDATE_TARGET update_realtime_translations)

    qt_add_lrelease(
        TS_FILES ${SOFTPHONE_REALTIME_TS}
        QM_FILES_OUTPUT_VARIABLE SOFTPHONE_REALTIME_QM
        LRELEASE_TARGET release_realtime_translations)

    qt_add_resources(softphone_plugin_realtime_ui "realtime_i18n"
        PREFIX "/i18n/realtime"
        BASE "${CMAKE_CURRENT_BINARY_DIR}"
        FILES ${SOFTPHONE_REALTIME_QM})
endif()

softphone_plugin_in_a_file(realtime)
