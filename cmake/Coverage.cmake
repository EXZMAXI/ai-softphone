# AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
# https://ai-softphone.com
#
# SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
# SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of AI Softphone. There is NO WARRANTY, to the extent
# permitted by law. See the file LICENSE for the full terms.

add_library(softphone_coverage INTERFACE)
add_library(softphone::coverage ALIAS softphone_coverage)

if(SOFTPHONE_ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(softphone_coverage INTERFACE --coverage -O0 -g -fno-inline)
        target_link_options(softphone_coverage INTERFACE --coverage)

        message(STATUS "Coverage instrumentation enabled")
    elseif(MSVC)

        message(STATUS "Coverage instrumentation skipped: MSVC has no gcov, "
                       "and the figure is measured on the Linux job")
    else()
        message(FATAL_ERROR "SOFTPHONE_ENABLE_COVERAGE requires GCC, Clang or MSVC")
    endif()
endif()
