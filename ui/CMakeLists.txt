# AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
# https://ai-softphone.com
#
# SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
# SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of AI Softphone. There is NO WARRANTY, to the extent
# permitted by law. See the file LICENSE for the full terms.

include(GNUInstallDirs)

find_package(Qt6 6.8 REQUIRED COMPONENTS Network Quick QuickControls2 QuickDialogs2 LabsPlatform
    LinguistTools)

if(APPLE)
    find_package(Qt6 6.8 QUIET COMPONENTS Svg)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(Qt6 6.8 REQUIRED COMPONENTS DBus)

    if(SOFTPHONE_PACKAGE)
        find_package(Qt6 6.8 REQUIRED COMPONENTS WaylandClient
                     WaylandEglClientHwIntegrationPrivate)
    endif()
endif()

qt_standard_project_setup(REQUIRES 6.8)

if(WIN32)
    set(SOFTPHONE_AUTOSTART src/autostart_win32.cpp)
    set(SOFTPHONE_LINK_HANDLER src/link_handler_win32.cpp)
elseif(APPLE)
    set(SOFTPHONE_AUTOSTART src/autostart_apple.cpp)
    set(SOFTPHONE_LINK_HANDLER src/link_handler_apple.cpp)
else()
    set(SOFTPHONE_AUTOSTART src/autostart_xdg.cpp)
    set(SOFTPHONE_LINK_HANDLER src/link_handler_xdg.cpp)
endif()

set(SOFTPHONE_BUNDLE_IDENTIFIER "com.ai-softphone.phone")

qt_add_library(softphone_ui_model STATIC
    src/about.cpp
    src/autostart.cpp
    ${SOFTPHONE_AUTOSTART}
    ${SOFTPHONE_LINK_HANDLER}
    src/buttons_view_model.cpp
    src/call_view_model.cpp
    src/desktop_away.cpp
    src/session_away.cpp
    src/desktop_colours.cpp
    src/contacts_view_model.cpp
    src/dashboard_view_model.cpp
    src/diagnostics_view_model.cpp
    src/desktop.cpp
    src/wheel_acceleration.cpp

    src/dialling.hpp
    src/global_hotkeys.cpp
    src/history_view_model.cpp
    src/hotkeys.cpp
    src/jobs_view_model.cpp
    src/link_handler.cpp
    src/menu_entry.cpp
    src/list_edit.cpp
    src/list_edit.hpp
    src/chip_wrap.cpp
    src/accounts_view_model.cpp
    src/api_settings_json.cpp
    src/plugin_settings.cpp
    src/settings_sink.cpp
    src/single_instance.cpp
    src/prompter_view_model.cpp
    src/settings_view_model.cpp
    src/signal_geometry.cpp
    src/sound_view_model.cpp
    src/artifacts_view_model.cpp
    src/export_names.cpp
    src/row_marks.cpp
    src/automations_view_model.cpp
    src/providers_view_model.cpp
    src/taxonomy_view_model.cpp
    src/templates_view_model.cpp
    src/recordings_view_model.cpp
    src/plugins_view_model.cpp
    src/sections.cpp
    src/sounds.cpp
    src/startup_state.cpp
    src/storage_view_model.cpp
    src/transcript_view_model.cpp
    src/tray_presentation.cpp
    src/telemetry_view_model.cpp
    src/update_view_model.cpp
    src/tray_view_model.cpp
    src/window_memory.cpp
    src/phone_shape.cpp
    src/window_shape.cpp
    src/window_placement.cpp
    src/translations.cpp)

qt_add_resources(softphone_ui_model "sounds"
    PREFIX "/sounds"
    BASE "sounds"
    FILES
        sounds/ring-classic.wav
        sounds/ring-bell.wav
        sounds/ring-soft.wav
        sounds/ring-digital.wav
        sounds/ring-insistent.wav
        sounds/ringback.wav
        sounds/disconnect.wav
        sounds/test.wav
        sounds/auto-answer.wav
        sounds/busy.wav
        sounds/dtmf-0.wav
        sounds/dtmf-1.wav
        sounds/dtmf-2.wav
        sounds/dtmf-3.wav
        sounds/dtmf-4.wav
        sounds/dtmf-5.wav
        sounds/dtmf-6.wav
        sounds/dtmf-7.wav
        sounds/dtmf-8.wav
        sounds/dtmf-9.wav
        sounds/dtmf-star.wav
        sounds/dtmf-hash.wav
        sounds/recording-notice.wav
        sounds/recording-tone.wav)

qt_add_resources(softphone_ui_model "appicon"
    PREFIX "/icons"
    BASE "icons"
    FILES
        icons/ai-softphone.svg

        icons/tray-connected.svg
        icons/tray-connected-missed.svg
        icons/tray-degraded.svg
        icons/tray-degraded-missed.svg
        icons/tray-failing.svg
        icons/tray-failing-missed.svg
        icons/tray-offline.svg
        icons/tray-offline-missed.svg
        icons/tray-ringing.svg
        icons/tray-ringing-missed.svg)

qt_add_resources(softphone_ui_model "glyphs"
    PREFIX "/glyphs"
    BASE "icons/glyphs"
    FILES
        icons/glyphs/regular/call.svg
        icons/glyphs/regular/call-end.svg
        icons/glyphs/regular/call-dismiss.svg
        icons/glyphs/regular/call-pause.svg
        icons/glyphs/regular/call-swap.svg
        icons/glyphs/regular/dialpad.svg
        icons/glyphs/regular/hold.svg
        icons/glyphs/regular/merge.svg
        icons/glyphs/regular/more-vertical.svg
        icons/glyphs/regular/mic.svg
        icons/glyphs/regular/mic-off.svg
        icons/glyphs/regular/resume.svg
        icons/glyphs/regular/split.svg
        icons/glyphs/regular/transfer.svg
        icons/glyphs/regular/call-inbound.svg
        icons/glyphs/regular/call-checkmark.svg
        icons/glyphs/regular/call-missed.svg
        icons/glyphs/regular/call-outbound.svg
        icons/glyphs/regular/delete.svg
        icons/glyphs/regular/dismiss.svg
        icons/glyphs/regular/edit.svg
        icons/glyphs/regular/person-add.svg
        icons/glyphs/regular/speaker.svg
        icons/glyphs/regular/folder-open.svg
        icons/glyphs/regular/redial.svg
        icons/glyphs/regular/record.svg
        icons/glyphs/regular/record-stop.svg
        icons/glyphs/regular/record-pause.svg
        icons/glyphs/regular/pin.svg
        icons/glyphs/regular/save.svg
        icons/glyphs/regular/copy.svg
        icons/glyphs/regular/cut.svg
        icons/glyphs/regular/paste.svg
        icons/glyphs/regular/undo.svg
        icons/glyphs/regular/redo.svg
        icons/glyphs/regular/select-all.svg
        icons/glyphs/regular/globe.svg
        icons/glyphs/regular/chevron-left.svg
        icons/glyphs/regular/chevron-right.svg
        icons/glyphs/regular/chevron-up.svg
        icons/glyphs/regular/history.svg
        icons/glyphs/regular/sparkle.svg
        icons/glyphs/regular/star.svg
        icons/glyphs/regular/apps.svg
        icons/glyphs/regular/pulse.svg
        icons/glyphs/regular/flash.svg
        icons/glyphs/regular/clock.svg
        icons/glyphs/regular/arrow-import.svg
        icons/glyphs/regular/book-contacts.svg
        icons/glyphs/regular/library.svg
        icons/glyphs/regular/settings.svg
        icons/glyphs/regular/call-prohibited.svg
        icons/glyphs/filled/call.svg
        icons/glyphs/filled/call-end.svg
        icons/glyphs/filled/call-dismiss.svg
        icons/glyphs/filled/call-pause.svg
        icons/glyphs/filled/call-swap.svg
        icons/glyphs/filled/dialpad.svg
        icons/glyphs/filled/hold.svg
        icons/glyphs/filled/merge.svg
        icons/glyphs/filled/more-vertical.svg
        icons/glyphs/filled/mic.svg
        icons/glyphs/filled/mic-off.svg
        icons/glyphs/filled/resume.svg
        icons/glyphs/filled/split.svg
        icons/glyphs/filled/transfer.svg
        icons/glyphs/filled/call-inbound.svg
        icons/glyphs/filled/call-checkmark.svg
        icons/glyphs/filled/call-missed.svg
        icons/glyphs/filled/call-outbound.svg
        icons/glyphs/filled/delete.svg
        icons/glyphs/filled/dismiss.svg
        icons/glyphs/filled/edit.svg
        icons/glyphs/filled/person-add.svg
        icons/glyphs/filled/speaker.svg
        icons/glyphs/filled/folder-open.svg
        icons/glyphs/filled/redial.svg
        icons/glyphs/filled/record.svg
        icons/glyphs/filled/record-stop.svg
        icons/glyphs/filled/record-pause.svg
        icons/glyphs/filled/pin.svg
        icons/glyphs/filled/save.svg
        icons/glyphs/filled/copy.svg
        icons/glyphs/filled/cut.svg
        icons/glyphs/filled/paste.svg
        icons/glyphs/filled/undo.svg
        icons/glyphs/filled/redo.svg
        icons/glyphs/filled/select-all.svg
        icons/glyphs/filled/globe.svg
        icons/glyphs/filled/chevron-left.svg
        icons/glyphs/filled/chevron-right.svg
        icons/glyphs/filled/chevron-up.svg
        icons/glyphs/filled/history.svg
        icons/glyphs/filled/sparkle.svg
        icons/glyphs/filled/star.svg
        icons/glyphs/filled/apps.svg
        icons/glyphs/filled/pulse.svg
        icons/glyphs/filled/flash.svg
        icons/glyphs/filled/clock.svg
        icons/glyphs/filled/arrow-import.svg
        icons/glyphs/filled/book-contacts.svg
        icons/glyphs/filled/library.svg
        icons/glyphs/filled/settings.svg
        icons/glyphs/filled/call-prohibited.svg)

target_include_directories(softphone_ui_model
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>)

target_compile_definitions(softphone_ui_model
    PUBLIC SOFTPHONE_BUNDLE_IDENTIFIER="${SOFTPHONE_BUNDLE_IDENTIFIER}")

target_link_libraries(softphone_ui_model
    PRIVATE

        nlohmann_json::nlohmann_json
    PUBLIC
        softphone::core
        softphone::shell
        softphone::storage
        softphone::coverage

        softphone::plugin::capture::api
        Qt6::Qml

        Qt6::Gui

        Qt6::Network
    PRIVATE
        softphone::warnings)

if(WIN32)
    target_sources(softphone_ui_model PRIVATE src/global_hotkeys_win32.cpp)
elseif(APPLE)

    target_sources(softphone_ui_model PRIVATE src/global_hotkeys_apple.cpp)

    target_sources(softphone_ui_model PRIVATE src/tray_activation_apple.mm)

    target_sources(softphone_ui_model PRIVATE src/dock_reopen_apple.mm)
    target_link_libraries(softphone_ui_model PRIVATE "-framework Carbon"
                                                     "-framework CoreGraphics"
                                                     "-framework CoreServices"

                                                     "-framework AppKit")
elseif(UNIX AND NOT APPLE)
    find_package(X11 QUIET)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(XCB QUIET xcb)
    endif()

    if(X11_FOUND AND XCB_FOUND)
        target_sources(softphone_ui_model PRIVATE src/global_hotkeys_x11.cpp)
        target_compile_definitions(softphone_ui_model PRIVATE SOFTPHONE_HAS_X11)
        target_include_directories(softphone_ui_model PRIVATE ${X11_INCLUDE_DIR} ${XCB_INCLUDE_DIRS})
        target_link_libraries(softphone_ui_model PRIVATE ${X11_LIBRARIES} ${XCB_LIBRARIES})
    else()
        message(STATUS "X11 or xcb not found: global hotkeys will report themselves unsupported")
    endif()
else()
    message(STATUS "No window system backend for hotkeys here: they report themselves unsupported")
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_compile_definitions(softphone_ui_model PRIVATE SOFTPHONE_HAS_PORTAL)
    target_link_libraries(softphone_ui_model PRIVATE Qt6::DBus)
endif()

if(WIN32)
    target_sources(softphone_ui_model PRIVATE src/desktop_win32.cpp)

    target_link_libraries(softphone_ui_model PRIVATE ole32 dwmapi)
elseif(X11_FOUND AND XCB_FOUND)
    target_sources(softphone_ui_model PRIVATE src/desktop_x11.cpp)
else()
    target_sources(softphone_ui_model PRIVATE src/desktop_plain.cpp)
    message(STATUS "No X11: windows cannot be taken out of the task bar, and the window says so")
endif()

set_source_files_properties(qml/Duration.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
set_source_files_properties(qml/InAStrip.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
set_source_files_properties(qml/Ink.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
set_source_files_properties(qml/OneMenu.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
set_source_files_properties(qml/RecordingGlyph.qml qml/Mark.qml
                            PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
set_source_files_properties(qml/ItemsCalled.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
set_source_files_properties(qml/SplitPanes.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(softphone_ui_model
    URI Softphone
    VERSION 1.0

    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Softphone

    QML_FILES
        qml/CallBanner.qml
        qml/CaptureNotice.qml
        qml/CardLoader.qml
        qml/ConfirmDialog.qml
        qml/DialField.qml
        qml/LabelledPicker.qml
        qml/ListCheckBox.qml
        qml/ListRadioButton.qml
        qml/RowMenu.qml
        qml/RightMenu.qml
        qml/TextMenu.qml
        qml/TextMenuItem.qml
        qml/MemoField.qml
        qml/PathRow.qml
        qml/StartupSwitch.qml
        qml/ToolbarAct.qml
        qml/VolumeRow.qml
        qml/Duration.qml
        qml/Ink.qml
        qml/RecordingGlyph.qml
        qml/Mark.qml
        qml/InAStrip.qml
        qml/ItemsCalled.qml
        qml/Spotlight.qml
        qml/WaitingDot.qml
        qml/SplitPanes.qml
        qml/SlotLoader.qml
        qml/OneMenu.qml
        qml/Glyph.qml
        qml/Hint.qml
        qml/NoticeBanner.qml
        qml/Main.qml
        qml/Splash.qml
        qml/PanelAct.qml
        qml/PickerBox.qml
        qml/RowActs.qml
        qml/RowButton.qml
        qml/Screenshots.qml
        qml/SettingsAppearance.qml
        qml/Overview.qml
        qml/SettingsDashboard.qml
        qml/SettingsShortcuts.qml
        qml/SettingsPlugins.qml
        qml/SettingsStartup.qml
        qml/SettingsAbout.qml
        qml/TelemetryChoice.qml
        qml/MenuEntryOffer.qml
        qml/TelemetryNotice.qml
        qml/BodyPanel.qml
        qml/WindowBody.qml
        qml/ShapeDrawing.qml
        qml/ShapeChoice.qml
        qml/ShapeWords.qml
        qml/WorkspaceZones.qml
        qml/WorkspaceWindow.qml
        qml/TabbedBody.qml
        qml/SettingsWindow.qml
        qml/SettingsBody.qml
        qml/SidePanel.qml
        qml/DialWindow.qml
        qml/TransferBody.qml
        qml/TransferWindow.qml
        qml/ThinScrollBar.qml
        qml/StripKey.qml
        qml/SplitGrip.qml)

qt_add_executable(softphone_ui
    src/api_handlers.cpp
    src/main.cpp
    src/plugin_directory.cpp

    src/signal_drawing.cpp

    src/screenshots.cpp

    src/demo_calls.cpp

    src/demo_capture.cpp

    src/demo_prompter.cpp)

if(NOT SOFTPHONE_PACKAGE)
    target_compile_definitions(softphone_ui
        PRIVATE SOFTPHONE_PLUGIN_BUILD_TREE="${SOFTPHONE_PLUGIN_BUILD_TREE}")
endif()

target_link_libraries(softphone_ui
    PRIVATE
        softphone_ui_model
        softphone_ui_modelplugin
        softphone::container
        softphone::plugin::monolith

        softphone::plugin::taxonomy::api

        softphone::plugin::processing::api
        softphone::plugin::storage
        softphone::plugin::transcription::api

        softphone::plugin::realtime
        softphone::plugin::recording::api
        softphone::plugin::capture::api
        softphone::plugin::webui::api
        softphone::plugin::integration
        softphone::plugin::integration::ui
        softphone::plugin::diagnostics
        softphone::plugin::diagnostics::ui
        softphone::plugin::softphone
        softphone::plugin::softphone::ui
        softphone::core
        softphone::media
        softphone::secrets
        softphone::http
        softphone::sip
        softphone::warnings
        Qt6::Quick
        Qt6::QuickControls2

        Qt6::LabsPlatform

        Qt6::QuickDialogs2)

set_target_properties(softphone_ui PROPERTIES
    OUTPUT_NAME ai-softphone
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON)

softphone_add_windows_resource(softphone_ui
    NAME ai-softphone
    DESCRIPTION "AI Softphone"
    ICON "${CMAKE_CURRENT_SOURCE_DIR}/icons/ai-softphone.ico")

softphone_translation_catalogue(SOFTPHONE_UI_TS softphone)

string(JOIN " " SOFTPHONE_OFFERED ${SOFTPHONE_LANGUAGES})
target_compile_definitions(softphone_ui_model
    PRIVATE SOFTPHONE_OFFERED_LANGUAGES="${SOFTPHONE_OFFERED}")

qt_add_lupdate(
    SOURCE_TARGETS softphone_ui_model softphone_ui
    TS_FILES ${SOFTPHONE_UI_TS}
    OPTIONS ${SOFTPHONE_LUPDATE_OPTIONS}
    LUPDATE_TARGET update_ui_translations)

qt_add_lrelease(
    TS_FILES ${SOFTPHONE_UI_TS}
    QM_FILES_OUTPUT_VARIABLE SOFTPHONE_UI_QM
    LRELEASE_TARGET release_ui_translations)

qt_add_resources(softphone_ui_model "ui_i18n"
    PREFIX "/i18n"
    BASE "${CMAKE_CURRENT_BINARY_DIR}"
    FILES ${SOFTPHONE_UI_QM})

if(APPLE)
    set(SOFTPHONE_BUNDLE_NAME "AI Softphone")
    set(SOFTPHONE_BUNDLE_EXECUTABLE "ai-softphone")

    set(SOFTPHONE_BUNDLE_ICON "ai-softphone")

    configure_file(Info.plist.in "${CMAKE_CURRENT_BINARY_DIR}/Info.plist" @ONLY)

    set_source_files_properties(icons/ai-softphone.icns PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources)
    target_sources(softphone_ui PRIVATE icons/ai-softphone.icns)

    set_target_properties(softphone_ui PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_BINARY_DIR}/Info.plist"
        MACOSX_BUNDLE_BUNDLE_NAME "${SOFTPHONE_BUNDLE_NAME}"
        MACOSX_BUNDLE_GUI_IDENTIFIER "${SOFTPHONE_BUNDLE_IDENTIFIER}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}")

    set(SOFTPHONE_CODESIGN_IDENTITY "" CACHE STRING
        "Code-signing identity for the .app; empty to look for the development one")

    set(SOFTPHONE_SIGNING_IDENTITY "${SOFTPHONE_CODESIGN_IDENTITY}")
    if(NOT SOFTPHONE_SIGNING_IDENTITY)
        execute_process(
            COMMAND security find-identity -v -p codesigning
            OUTPUT_VARIABLE SOFTPHONE_IDENTITIES
            ERROR_QUIET)
        if(SOFTPHONE_IDENTITIES MATCHES "AI Softphone Development")
            set(SOFTPHONE_SIGNING_IDENTITY "AI Softphone Development")
        endif()
    endif()

    if(SOFTPHONE_SIGNING_IDENTITY)
        message(STATUS "Signing the bundle as \"${SOFTPHONE_SIGNING_IDENTITY}\"")

        if(NOT SOFTPHONE_PACKAGE)
            add_custom_command(TARGET softphone_ui POST_BUILD
                COMMAND codesign --force --sign "${SOFTPHONE_SIGNING_IDENTITY}"
                    --identifier "${SOFTPHONE_BUNDLE_IDENTIFIER}"
                    "$<TARGET_BUNDLE_DIR:softphone_ui>"
                COMMENT "Signing ai-softphone.app as ${SOFTPHONE_SIGNING_IDENTITY}"
                VERBATIM)
        endif()
    else()

        message(STATUS
            "No code-signing identity: the bundle keeps the linker's ad-hoc one, and macOS "
            "will ask about the keychain after every build. ./scripts/make-signing-identity.sh")
    endif()
endif()

install(TARGETS softphone_ui
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

if(SOFTPHONE_PACKAGE)

    if(WIN32)
        set(SOFTPHONE_DEPLOY_TOOL_OPTIONS DEPLOY_TOOL_OPTIONS --no-compiler-runtime)
    endif()

    qt_generate_deploy_qml_app_script(
        TARGET softphone_ui
        OUTPUT_SCRIPT SOFTPHONE_DEPLOY_SCRIPT

        NO_UNSUPPORTED_PLATFORM_ERROR

        ${SOFTPHONE_DEPLOY_TOOL_OPTIONS})
    install(SCRIPT ${SOFTPHONE_DEPLOY_SCRIPT})

    if(APPLE AND TARGET Qt6::QSvgPlugin)
        install(FILES $<TARGET_FILE:Qt6::QSvgPlugin>
                DESTINATION ai-softphone.app/Contents/PlugIns/imageformats)
    endif()

    if(UNIX AND NOT APPLE)
        install(FILES $<TARGET_FILE:Qt6::QGtk3ThemePlugin>
                      $<TARGET_FILE:Qt6::QXdgDesktopPortalThemePlugin>
                DESTINATION plugins/platformthemes)

        install(FILES $<TARGET_FILE:Qt6::QIbusPlatformInputContextPlugin>
                      $<TARGET_FILE:Qt6::QComposePlatformInputContextPlugin>
                DESTINATION plugins/platforminputcontexts)

        if(SOFTPHONE_PACKAGE)
            install(FILES $<TARGET_FILE:Qt6::QWaylandIntegrationPlugin>
                          $<TARGET_FILE:Qt6::QWaylandEglPlatformIntegrationPlugin>
                    DESTINATION plugins/platforms)

            install(FILES $<TARGET_FILE:Qt6::QWaylandXdgShellIntegrationPlugin>
                    DESTINATION plugins/wayland-shell-integration)

            install(FILES $<TARGET_FILE:Qt6::QWaylandBradientDecorationPlugin>
                    DESTINATION plugins/wayland-decoration-client)

            install(FILES $<TARGET_FILE:Qt6::QWaylandEglClientBufferPlugin>
                    DESTINATION plugins/wayland-graphics-integration-client)

            install(FILES
                    $<TARGET_FILE:Qt6::WaylandClient>
                    $<TARGET_SONAME_FILE:Qt6::WaylandClient>
                    $<TARGET_FILE:Qt6::WaylandEglClientHwIntegrationPrivate>
                    $<TARGET_SONAME_FILE:Qt6::WaylandEglClientHwIntegrationPrivate>
                    DESTINATION ${CMAKE_INSTALL_LIBDIR})
        endif()
    endif()

    if(MSVC)
        set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
        include(InstallRequiredSystemLibraries)
        install(PROGRAMS ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS}
                DESTINATION ${CMAKE_INSTALL_BINDIR})
    endif()

    if(WIN32 AND SOFTPHONE_OPENSSL_RUNTIME_LIBS)
        install(PROGRAMS ${SOFTPHONE_OPENSSL_RUNTIME_LIBS}
                DESTINATION ${CMAKE_INSTALL_BINDIR})
    endif()

    if(MSVC)
        install(FILES $<TARGET_PDB_FILE:softphone_ui>
                DESTINATION ${CMAKE_INSTALL_BINDIR}
                OPTIONAL)
    endif()
endif()

if(SOFTPHONE_LINK_BINDIR)
    install(CODE "
        set(_link_directory \"\$ENV{DESTDIR}${SOFTPHONE_LINK_BINDIR}\")
        file(MAKE_DIRECTORY \"\${_link_directory}\")

        foreach(_binary ai-softphone ai-softphone-cli)
            set(_link \"\${_link_directory}/\${_binary}\")
            file(CREATE_LINK
                \"\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/\${_binary}\"
                \"\${_link}\"
                SYMBOLIC)
            message(STATUS \"Installing: \${_link}\")
        endforeach()
    ")
endif()

if(UNIX AND NOT APPLE)

    install(CODE "
        set(_entry_source \"${CMAKE_CURRENT_SOURCE_DIR}/ai-softphone.desktop\")

        set(_data_directory \"${SOFTPHONE_DESKTOP_DATA_DIR}\")
        if(NOT IS_ABSOLUTE \"\${_data_directory}\")
            set(_data_directory \"\${CMAKE_INSTALL_PREFIX}/\${_data_directory}\")
        endif()
        set(_entry_target \"\$ENV{DESTDIR}\${_data_directory}/applications/ai-softphone.desktop\")

        set(_exec \"${SOFTPHONE_DESKTOP_EXEC}\")
        if(NOT _exec)
            set(_exec \"\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/ai-softphone\")
            if(NOT \"${SOFTPHONE_LINK_BINDIR}\" STREQUAL \"\")
                set(_exec \"${SOFTPHONE_LINK_BINDIR}/ai-softphone\")
            endif()
        endif()

        file(READ \"\${_entry_source}\" _entry)
        string(REPLACE \"Exec=ai-softphone\" \"Exec=\${_exec}\" _entry \"\${_entry}\")

        get_filename_component(_entry_directory \"\${_entry_target}\" DIRECTORY)
        file(MAKE_DIRECTORY \"\${_entry_directory}\")
        file(WRITE \"\${_entry_target}\" \"\${_entry}\")

        message(STATUS \"Installing: \${_entry_target}\")
    ")

    install(FILES ai-softphone.metainfo.xml
        DESTINATION ${SOFTPHONE_DESKTOP_DATA_DIR}/metainfo

        RENAME ${SOFTPHONE_BUNDLE_IDENTIFIER}.metainfo.xml)

    install(FILES icons/ai-softphone.svg
        DESTINATION ${SOFTPHONE_DESKTOP_DATA_DIR}/icons/hicolor/scalable/apps)

    foreach(_icon_size 16 24 32 48 64 128 256 512)
        install(FILES icons/ai-softphone-${_icon_size}.png
            DESTINATION
                ${SOFTPHONE_DESKTOP_DATA_DIR}/icons/hicolor/${_icon_size}x${_icon_size}/apps
            RENAME ai-softphone.png)
    endforeach()

    install(FILES ${CMAKE_SOURCE_DIR}/LICENSE
        DESTINATION ${SOFTPHONE_DESKTOP_DATA_DIR}/doc/ai-softphone
        RENAME copyright)

    install(FILES ${CMAKE_SOURCE_DIR}/THIRD-PARTY-LICENCES
        DESTINATION ${SOFTPHONE_DESKTOP_DATA_DIR}/doc/ai-softphone)
endif()

if(WIN32)
    install(FILES ${CMAKE_SOURCE_DIR}/LICENSE
        DESTINATION .
        RENAME LICENSE.txt)
    install(FILES ${CMAKE_SOURCE_DIR}/THIRD-PARTY-LICENCES
        DESTINATION .
        RENAME THIRD-PARTY-LICENCES.txt)
endif()

softphone_carry_openssl()

if(APPLE AND SOFTPHONE_PACKAGE)
    set(SOFTPHONE_QT_TRANSLATIONS "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_TRANSLATIONS}")

    install(CODE "
        set(languages \"${SOFTPHONE_LANGUAGES}\")
        set(from \"${SOFTPHONE_QT_TRANSLATIONS}\")
        set(into \"\${CMAKE_INSTALL_PREFIX}/ai-softphone.app/Contents/translations\")
        file(MAKE_DIRECTORY \"\${into}\")
        set(carried 0)
        foreach(language IN LISTS languages)
            set(parts \"\")
            foreach(module qtbase qtdeclarative)
                if(EXISTS \"\${from}/\${module}_\${language}.qm\")
                    list(APPEND parts \"\${from}/\${module}_\${language}.qm\")
                endif()
            endforeach()
            if(parts)
                execute_process(
                    COMMAND \"$<TARGET_FILE:Qt6::lconvert>\"
                        -o \"\${into}/qt_\${language}.qm\" \${parts}
                    COMMAND_ERROR_IS_FATAL ANY)
                math(EXPR carried \"\${carried} + 1\")
            endif()
        endforeach()
        message(STATUS \"Qt's catalogues: \${carried} of the offered languages\")
    ")
endif()
