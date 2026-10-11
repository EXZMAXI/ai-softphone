# AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
# https://ai-softphone.com
#
# SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
# SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of AI Softphone. There is NO WARRANTY, to the extent
# permitted by law. See the file LICENSE for the full terms.

add_executable(softphone_cli
    src/collections.cpp
    src/main.cpp
    src/store_commands.cpp)

if(WIN32)
    target_sources(softphone_cli PRIVATE src/keyboard_win32.cpp)
else()
    target_sources(softphone_cli PRIVATE src/keyboard_posix.cpp)
endif()

target_link_libraries(softphone_cli
    PRIVATE
        softphone::container
        softphone::core
        softphone::plugin::monolith

        softphone::plugin::mediastorage::api
        softphone::plugin::processing::api
        softphone::plugin::directory::api
        softphone::plugin::softphone
        softphone::plugin::taxonomy::api
        softphone::plugin::transcription::api
        softphone::media
        softphone::secrets
        softphone::sip
        softphone::storage
        softphone::warnings)

set_target_properties(softphone_cli PROPERTIES OUTPUT_NAME ai-softphone-cli)

softphone_add_windows_resource(softphone_cli
    NAME ai-softphone-cli
    DESCRIPTION "AI Softphone command line")

install(TARGETS softphone_cli
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

softphone_carry_openssl()
