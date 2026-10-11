# AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
# https://ai-softphone.com
#
# SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
# SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of AI Softphone. There is NO WARRANTY, to the extent
# permitted by law. See the file LICENSE for the full terms.

set(SOFTPHONE_DESKTOP_DATA_DIR "${CMAKE_INSTALL_DATAROOTDIR}"
    CACHE STRING "Where the desktop entry, the icons and the AppStream metadata go")

set(SOFTPHONE_LINK_BINDIR ""
    CACHE STRING "Directory to symlink the binaries into; empty means do not")

set(SOFTPHONE_DESKTOP_EXEC ""
    CACHE STRING "What the desktop entry's Exec says; empty means the installed binary's own path")
