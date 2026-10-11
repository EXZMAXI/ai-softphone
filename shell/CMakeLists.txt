# AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
# https://ai-softphone.com
#
# SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
# SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of AI Softphone. There is NO WARRANTY, to the extent
# permitted by law. See the file LICENSE for the full terms.

find_package(Qt6 6.8 REQUIRED COMPONENTS Core)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_library(softphone_shell STATIC
    src/ui_loader.cpp
    src/ui_registry.cpp

    include/softphone/shell/ui_registry.hpp)

add_library(softphone::shell ALIAS softphone_shell)

target_include_directories(softphone_shell
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

target_link_libraries(softphone_shell
    PUBLIC
        softphone::container
        softphone::coverage
        Qt6::Core
    PRIVATE
        softphone::warnings)
