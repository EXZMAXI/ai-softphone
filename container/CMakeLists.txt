# AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
# https://ai-softphone.com
#
# SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
# SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of AI Softphone. There is NO WARRANTY, to the extent
# permitted by law. See the file LICENSE for the full terms.

add_library(softphone_container ${SOFTPHONE_LIBRARY_KIND}
    src/bus.cpp
    src/host.cpp
    src/loader.cpp
    src/loading.cpp
    src/manifest.cpp
    src/registry.cpp
    src/services.cpp
    src/settings.cpp
    src/version.cpp)

add_library(softphone::container ALIAS softphone_container)

target_include_directories(softphone_container
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

target_link_libraries(softphone_container
    PUBLIC softphone::coverage
    PRIVATE nlohmann_json::nlohmann_json softphone::warnings ${CMAKE_DL_LIBS})

target_compile_definitions(softphone_container
    PUBLIC SOFTPHONE_CONTAINER_VERSION="${PROJECT_VERSION}")
