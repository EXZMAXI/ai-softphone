# AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
# https://ai-softphone.com
#
# SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
# SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of AI Softphone. There is NO WARRANTY, to the extent
# permitted by law. See the file LICENSE for the full terms.

add_library(softphone_plugin_softphone ${SOFTPHONE_LIBRARY_KIND}
    src/engine_host.cpp
    src/plugin.cpp)

add_library(softphone::plugin::softphone ALIAS softphone_plugin_softphone)

target_include_directories(softphone_plugin_softphone
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

target_link_libraries(softphone_plugin_softphone
    PUBLIC
        softphone::container
        softphone::core
        softphone::plugin::storage

        softphone::sip
        softphone::media
        softphone::secrets
        softphone::storage
        softphone::coverage
    PRIVATE
        softphone::warnings)

if(SOFTPHONE_BUILD_UI)
    find_package(Qt6 6.8 REQUIRED COMPONENTS Core Quick LinguistTools)
    qt_standard_project_setup(REQUIRES 6.8)

    qt_add_library(softphone_plugin_softphone_ui ${SOFTPHONE_LIBRARY_KIND}
        src/ui.cpp)

    add_library(softphone::plugin::softphone::ui ALIAS softphone_plugin_softphone_ui)

    target_include_directories(softphone_plugin_softphone_ui
        PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

    target_link_libraries(softphone_plugin_softphone_ui
        PUBLIC
            softphone::plugin::softphone

            softphone_plugin_directory_api
            softphone::shell
            softphone::coverage
        PRIVATE
            softphone::warnings)

    set_source_files_properties(qml/CallInk.qml qml/CallGlyph.qml
                                PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

    qt_add_qml_module(softphone_plugin_softphone_ui
        URI Softphone.Telephone
        VERSION 1.0
        NO_PLUGIN
        OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Softphone/Telephone
        QML_FILES
            qml/SettingsAccounts.qml
            qml/SettingsAudio.qml
            qml/SettingsCalls.qml
            qml/HistoryTab.qml
            qml/Dialler.qml
            qml/BarPage.qml
            qml/HistoryAct.qml
            qml/CallInk.qml
            qml/CallGlyph.qml)

    softphone_translation_catalogue(SOFTPHONE_TELEPHONE_TS telephone)

    qt_add_lupdate(
        SOURCE_TARGETS softphone_plugin_softphone_ui
        TS_FILES ${SOFTPHONE_TELEPHONE_TS}
        OPTIONS ${SOFTPHONE_LUPDATE_OPTIONS}
        LUPDATE_TARGET update_telephone_translations)

    qt_add_lrelease(
        TS_FILES ${SOFTPHONE_TELEPHONE_TS}
        QM_FILES_OUTPUT_VARIABLE SOFTPHONE_TELEPHONE_QM
        LRELEASE_TARGET release_telephone_translations)

    qt_add_resources(softphone_plugin_softphone_ui "telephone_i18n"
        PREFIX "/i18n/telephone"
        BASE "${CMAKE_CURRENT_BINARY_DIR}"
        FILES ${SOFTPHONE_TELEPHONE_QM})
endif()
