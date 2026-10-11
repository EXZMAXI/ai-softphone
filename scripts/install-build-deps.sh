#!/usr/bin/env bash
#
# AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
# https://ai-softphone.com
#
# SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
# SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of AI Softphone. There is NO WARRANTY, to the extent
# permitted by law. See the file LICENSE for the full terms.

set -euo pipefail

packages=(
    ninja-build
    clang-format
    clang-tidy
    gcovr
    lcov

    libgl1-mesa-dev
    libxkbcommon-dev

    libwayland-dev

    libasound2-dev
    libssl-dev
    uuid-dev
    libopus-dev

    libsecret-1-dev

    libx11-dev
    libxcb1-dev

    dpkg-dev
    fakeroot
    patchelf
    squashfs-tools

    libpulse-dev
)

sudo_prefix=()
if [[ ${EUID} -ne 0 ]]; then
    sudo_prefix=(sudo)
fi

"${sudo_prefix[@]}" apt-get update
"${sudo_prefix[@]}" apt-get install -y --no-install-recommends "${packages[@]}"

echo
echo "Installed ${#packages[@]} packages."
echo "Qt is separate — see the README."
