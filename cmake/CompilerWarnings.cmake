# AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
# https://ai-softphone.com
#
# SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
# SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of AI Softphone. There is NO WARRANTY, to the extent
# permitted by law. See the file LICENSE for the full terms.

add_library(softphone_warnings INTERFACE)
add_library(softphone::warnings ALIAS softphone_warnings)

if(MSVC)

    target_compile_options(softphone_warnings INTERFACE /W4 /permissive- /Zc:preprocessor /wd4702 /wd4251)

    target_compile_definitions(softphone_warnings INTERFACE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(softphone_warnings INTERFACE
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wnon-virtual-dtor
        -Wold-style-cast
        -Wcast-align
        -Wunused
        -Woverloaded-virtual
        -Wconversion
        -Wsign-conversion
        -Wnull-dereference
        -Wdouble-promotion
        -Wformat=2)
endif()

if(SOFTPHONE_WARNINGS_AS_ERRORS)
    if(MSVC)
        target_compile_options(softphone_warnings INTERFACE /WX)
    else()
        target_compile_options(softphone_warnings INTERFACE -Werror)
    endif()
endif()
