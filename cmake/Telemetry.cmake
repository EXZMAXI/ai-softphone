# AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
# https://ai-softphone.com
#
# SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
# SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of AI Softphone. There is NO WARRANTY, to the extent
# permitted by law. See the file LICENSE for the full terms.

if(SOFTPHONE_PACKAGE)
    set(SOFTPHONE_TELEMETRY_REPORTS 1)
    message(STATUS "Telemetry: this build reports, because it is a package")
else()
    set(SOFTPHONE_TELEMETRY_REPORTS 0)
endif()
