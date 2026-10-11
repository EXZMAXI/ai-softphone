# AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
# https://ai-softphone.com
#
# SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
# SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of AI Softphone. There is NO WARRANTY, to the extent
# permitted by law. See the file LICENSE for the full terms.

set(SOFTPHONE_LANGUAGES
    ru de fr es it pt nl sv da nb fi pl cs sk hu ro bg hr sl et lv lt el tr uk id vi
    sr sr_Latn)

set(SOFTPHONE_LANGUAGES_UNFINISHED)

set(SOFTPHONE_PSEUDOLOCALE eo)

set(SOFTPHONE_CATALOGUE_LANGUAGES
    ${SOFTPHONE_LANGUAGES}
    ${SOFTPHONE_LANGUAGES_UNFINISHED}
    ${SOFTPHONE_PSEUDOLOCALE})

function(softphone_translation_catalogue out_variable prefix)
    set(files "")
    foreach(language IN LISTS SOFTPHONE_CATALOGUE_LANGUAGES)
        list(APPEND files "${CMAKE_CURRENT_SOURCE_DIR}/i18n/${prefix}_${language}.ts")
    endforeach()
    set(${out_variable} "${files}" PARENT_SCOPE)
endfunction()

set(SOFTPHONE_LUPDATE_OPTIONS -locations none -no-obsolete)
