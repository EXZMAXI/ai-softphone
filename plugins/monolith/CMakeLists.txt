# AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
# https://ai-softphone.com
#
# SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
# SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of AI Softphone. There is NO WARRANTY, to the extent
# permitted by law. See the file LICENSE for the full terms.

add_library(softphone_plugin_monolith ${SOFTPHONE_LIBRARY_KIND}
    src/plugin.cpp)

add_library(softphone::plugin::monolith ALIAS softphone_plugin_monolith)

target_include_directories(softphone_plugin_monolith
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

target_link_libraries(softphone_plugin_monolith
    PUBLIC
        softphone::container
        softphone::core
        softphone::coverage
    PRIVATE
        softphone::warnings)
