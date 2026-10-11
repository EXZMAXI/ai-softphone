# AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
# https://ai-softphone.com
#
# SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
# SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of AI Softphone. There is NO WARRANTY, to the extent
# permitted by law. See the file LICENSE for the full terms.

include(FetchContent)

FetchContent_Declare(sqlite3_amalgamation
    URL https://sqlite.org/2025/sqlite-amalgamation-3500400.zip
    URL_HASH SHA256=1d3049dd0f830a025a53105fc79fd2ab9431aea99e137809d064d8ee8356b032)

FetchContent_MakeAvailable(sqlite3_amalgamation)

enable_language(C)

add_library(sqlite3 STATIC ${sqlite3_amalgamation_SOURCE_DIR}/sqlite3.c)

target_include_directories(sqlite3 SYSTEM PUBLIC ${sqlite3_amalgamation_SOURCE_DIR})

target_compile_definitions(sqlite3
    PRIVATE

        SQLITE_THREADSAFE=1

        SQLITE_DEFAULT_FOREIGN_KEYS=1

        SQLITE_DQS=0

        SQLITE_OMIT_LOAD_EXTENSION

        SQLITE_ENABLE_FTS5)

set_target_properties(sqlite3 PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(softphone_storage ${SOFTPHONE_LIBRARY_KIND}
    src/sqlite_store.cpp)

add_library(softphone::storage ALIAS softphone_storage)

target_include_directories(softphone_storage
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

target_link_libraries(softphone_storage
    PUBLIC
        softphone::core
        softphone::coverage
    PRIVATE
        softphone::warnings
        sqlite3)
