# AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
# https://ai-softphone.com
#
# SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
# SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of AI Softphone. There is NO WARRANTY, to the extent
# permitted by law. See the file LICENSE for the full terms.

include_guard(GLOBAL)

function(softphone_ascii_path variable what)
    set(path "${${variable}}")

    if(NOT WIN32)
        return()
    endif()

    string(REGEX MATCH "[^ -~]" outside "${path}")
    if(outside STREQUAL "")
        return()
    endif()

    file(TO_NATIVE_PATH "${path}" native)

    execute_process(
        COMMAND powershell -NoProfile -NonInteractive -Command
                "(New-Object -ComObject Scripting.FileSystemObject).GetFolder('${native}').ShortPath"
        OUTPUT_VARIABLE short
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        RESULT_VARIABLE asked)

    string(REGEX MATCH "[^ -~]" still "${short}")
    if(NOT asked EQUAL 0 OR short STREQUAL "" OR NOT still STREQUAL "")

        message(FATAL_ERROR
            "${what} is at:\n"
            "  ${path}\n"
            "which has characters outside ASCII in it, and this machine has no "
            "8.3 short name for it. MSVC's linker reads a response file in the "
            "ANSI code page, so a path like that arrives as mojibake and the "
            "link fails naming a file that is there.\n"
            "Put the dependency cache somewhere spelled in ASCII:\n"
            "  set SOFTPHONE_CACHE_DIR=D:\softphone-ai-deps\n"
            "and build it again with scripts/build-pjproject.ps1 and "
            "scripts/build-openssl.ps1.")
    endif()

    file(TO_CMAKE_PATH "${short}" short)
    message(STATUS "${what}: using the 8.3 name ${short}, because MSVC cannot read the other one")
    set(${variable} "${short}" PARENT_SCOPE)
endfunction()
