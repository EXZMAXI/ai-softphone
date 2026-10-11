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

root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)

wanted_glibc="2.35"
wanted_glibcxx="3.4.30"

highest() {
    grep -oE "${2}_[0-9.]+" | sed "s/${2}_//" | sort -u -V | tail -1
}

look_at() {
    local file="$1"
    local glibc glibcxx
    glibc=$(objdump -T "${file}" 2> /dev/null | highest "${file}" GLIBC || true)
    glibcxx=$(objdump -T "${file}" 2> /dev/null | highest "${file}" GLIBCXX || true)
    printf '  %-46s glibc %-8s libstdc++ %s\n' "$(basename "${file}")" \
        "${glibc:-—}" "${glibcxx:-—}"

    if [[ -n ${glibc} ]] && [[ $(printf '%s\n%s\n' "${wanted_glibc}" "${glibc}" | sort -V | tail -1) != "${wanted_glibc}" ]]; then
        echo "    ^ demands glibc ${glibc}, and ${wanted_glibc} is what was promised" >&2
        failed=1
    fi
    if [[ -n ${glibcxx} ]] && [[ $(printf '%s\n%s\n' "${wanted_glibcxx}" "${glibcxx}" | sort -V | tail -1) != "${wanted_glibcxx}" ]]; then
        echo "    ^ demands libstdc++ ${glibcxx}, and ${wanted_glibcxx} is what was promised" >&2
        echo "      (harmless where the image carries its own — see AppRun)" >&2
    fi
}

failed=0

if [[ $# -ge 1 ]]; then
    look_at "$1"
    exit "${failed}"
fi

image=$(find "${root}/dist/linux" -maxdepth 1 -name '*.AppImage' 2> /dev/null | head -1)
if [[ -z ${image} ]]; then
    echo "Nothing in dist/linux to look at. Package first." >&2
    exit 1
fi

echo "The floor of $(basename "${image}"):"

work=$(mktemp -d)
trap 'rm -rf "${work}"' EXIT
(cd "${work}" && "${image}" --appimage-extract > /dev/null)

look_at "${work}/squashfs-root/usr/bin/ai-softphone"

while IFS= read -r one; do
    look_at "${one}"
done < <(find "${work}/squashfs-root/usr/lib" -name '*.so*' -type f | sort)

echo
if [[ ${failed} -eq 0 ]]; then
    echo "check-linux-floor: nothing asks for more than glibc ${wanted_glibc}."
else
    echo "check-linux-floor: the floor has risen above what is published." >&2
fi
exit "${failed}"
