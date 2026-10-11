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

if [[ $# -ne 4 ]]; then
    echo "usage: $0 <bundle.app> <identity> <entitlements> <identifier>" >&2
    exit 2
fi

bundle=$1
identity=$2
entitlements=$3
identifier=$4

for path in "${bundle}" "${entitlements}"; do
    if [[ ! -e ${path} ]]; then
        echo "No such thing: ${path}" >&2
        exit 1
    fi
done

xattr -cr "${bundle}"

sign() {
    codesign --force --sign "${identity}" --options runtime --timestamp "$@"
}

echo "   sealing what is inside ${bundle##*/}"

objects=()
while IFS= read -r -d '' object; do
    case "${object}" in
        *.framework/*) continue ;;
    esac
    [[ ${object} == "${bundle}/Contents/MacOS/"* ]] && continue
    file -b "${object}" 2> /dev/null | grep -q "Mach-O" || continue
    objects+=("${object}")
done < <(find "${bundle}" -type f -print0)
if ((${#objects[@]} > 0)); then
    sign "${objects[@]}"
fi
nested=${#objects[@]}

frameworks_found=()
while IFS= read -r -d '' framework; do
    if [[ -d "${framework}/Versions/A" ]]; then
        frameworks_found+=("${framework}/Versions/A")
    else
        frameworks_found+=("${framework}")
    fi
done < <(find "${bundle}" -type d -name '*.framework' -print0)
if ((${#frameworks_found[@]} > 0)); then
    sign "${frameworks_found[@]}"
fi
frameworks=${#frameworks_found[@]}

main=$(/usr/libexec/PlistBuddy -c 'Print :CFBundleExecutable' \
    "${bundle}/Contents/Info.plist" 2> /dev/null || true)

helpers_found=()
while IFS= read -r -d '' object; do
    [[ -n ${main} && ${object} == "${bundle}/Contents/MacOS/${main}" ]] && continue
    file -b "${object}" 2> /dev/null | grep -q "Mach-O" || continue
    helpers_found+=("${object}")
done < <(find "${bundle}/Contents/MacOS" -type f -print0)
if ((${#helpers_found[@]} > 0)); then
    sign "${helpers_found[@]}"
fi
helpers=${#helpers_found[@]}

echo "   ${nested} nested binaries, ${frameworks} frameworks, ${helpers} helpers"

sign --entitlements "${entitlements}" --identifier "${identifier}" "${bundle}"

codesign --verify --strict --deep --verbose=2 "${bundle}" 2>&1 | sed 's/^/   /'

if ! codesign --display --entitlements - --xml "${bundle}" 2> /dev/null |
    plutil -convert xml1 -o - - | grep -q "com.apple.security.device.audio-input"; then
    echo "The signed bundle carries no audio-input entitlement, so it would be deaf." >&2
    echo "  ${entitlements}" >&2
    exit 1
fi

echo "   signed as ${identity}"
