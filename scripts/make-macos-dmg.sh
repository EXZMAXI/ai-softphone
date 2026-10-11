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
build="${root}/build/package"
dist="${root}/dist/macos"
work="${build}/staging"
profile="ai-softphone-notary"
notarise=1

while [[ $# -gt 0 ]]; do
    case "$1" in
        --no-notarise) notarise=0; shift ;;
        --keychain-profile) profile=$2; shift 2 ;;
        *)
            echo "usage: $0 [--no-notarise] [--keychain-profile <name>]" >&2
            exit 2
            ;;
    esac
done

missing=()
for tool in hdiutil codesign xcrun plutil; do
    command -v "${tool}" > /dev/null 2>&1 || missing+=("${tool}")
done
if ((${#missing[@]} > 0)); then
    echo "Not installed: ${missing[*]}" >&2
    echo "  xcode-select --install" >&2
    exit 2
fi

if [[ ! -d ${build} ]]; then
    echo "No packaging build. Run:" >&2
    echo "  cmake --preset package && cmake --build --preset package" >&2
    exit 1
fi

mkdir -p "${build}"
if ! cmake -S "${root}" -B "${build}" -DSOFTPHONE_PACKAGE_FORMAT=dmg \
        > "${build}/configure.log" 2>&1; then
    echo "The configure failed; the last of it:" >&2
    tail -30 "${build}/configure.log" >&2
    exit 1
fi
if ! cmake --build "${build}" > "${build}/build.log" 2>&1; then
    echo "The build failed; the last of it:" >&2
    tail -30 "${build}/build.log" >&2
    exit 1
fi

version=$(sed -n 's/^CMAKE_PROJECT_VERSION:STATIC=//p' "${build}/CMakeCache.txt" | head -1)
if [[ -z ${version} ]]; then
    echo "Could not read the version out of ${build}/CMakeCache.txt" >&2
    exit 1
fi

identity=$(sed -n 's/^SOFTPHONE_CODESIGN_IDENTITY:STRING=//p' "${build}/CMakeCache.txt" | head -1)
if [[ -z ${identity} ]]; then
    identity=$(security find-identity -v -p codesigning |
        sed -n 's/.*"\(Developer ID Application: [^"]*\)".*/\1/p' | head -1)
fi
if [[ -z ${identity} ]]; then
    echo "No Developer ID Application identity in the keychain, and none configured." >&2
    echo "An image signed with a development certificate cannot be notarised and" >&2
    echo "will not open on anybody else's machine. Configure the build with:" >&2
    echo "  cmake --preset package -D SOFTPHONE_CODESIGN_IDENTITY='Developer ID Application: …'" >&2
    exit 1
fi

mkdir -p "${dist}"

echo "== the staged bundle"
mkdir -p "${work}"
rm -rf "${work}/root"
cmake --install "${build}" --prefix "${work}/root" > "${work}/install.log" 2>&1 || {
    echo "The install failed; the last of it:" >&2
    tail -30 "${work}/install.log" >&2
    exit 1
}

app="${work}/root/ai-softphone.app"
if [[ ! -d ${app} ]]; then
    echo "The install produced no ${app}" >&2
    exit 1
fi

beside=$(find "${work}/root" -maxdepth 1 -mindepth 1 ! -name 'ai-softphone.app' | wc -l | tr -d ' ')
if [[ ${beside} != 0 ]]; then
    echo "The install left something outside the bundle:" >&2
    find "${work}/root" -maxdepth 1 -mindepth 1 ! -name 'ai-softphone.app' >&2
    exit 1
fi

identifier=$(/usr/libexec/PlistBuddy -c 'Print :CFBundleIdentifier' \
    "${app}/Contents/Info.plist")
"${root}/scripts/sign-macos-bundle.sh" \
    "${app}" "${identity}" "${root}/ui/ai-softphone.entitlements" "${identifier}"

"${root}/scripts/check-package-tree.sh" "${app}" 2>&1 | sed 's/^/   /'

notarise() {
    local what=$1
    if ! xcrun notarytool submit "${what}" \
        --keychain-profile "${profile}" --wait --timeout 45m 2>&1 |
        tee "${work}/notary.log" | sed 's/^/   /'; then
        echo "Notarisation failed for ${what}. The whole of what Apple said:" >&2
        submission=$(sed -n 's/^ *id: \([0-9a-f-]*\)$/\1/p' "${work}/notary.log" | head -1)
        [[ -n ${submission} ]] &&
            xcrun notarytool log "${submission}" --keychain-profile "${profile}" >&2
        return 1
    fi

    if ! grep -q "status: Accepted" "${work}/notary.log"; then
        echo "Apple did not accept ${what}. The log:" >&2
        submission=$(sed -n 's/^ *id: \([0-9a-f-]*\)$/\1/p' "${work}/notary.log" | head -1)
        [[ -n ${submission} ]] &&
            xcrun notarytool log "${submission}" --keychain-profile "${profile}" >&2
        return 1
    fi
}

if [[ ${notarise} -eq 1 ]]; then
    if ! xcrun notarytool history --keychain-profile "${profile}" > /dev/null 2>&1; then
        echo "No keychain profile called ${profile}." >&2
        echo "Store the credentials once, by hand:" >&2
        echo "  xcrun notarytool store-credentials ${profile} \\" >&2
        echo "      --key <AuthKey_XXXX.p8> --key-id <id> --issuer <uuid>" >&2
        exit 1
    fi

    echo "== notarising the application"

    rm -f "${work}/ai-softphone.zip"
    ditto -c -k --keepParent "${app}" "${work}/ai-softphone.zip"
    notarise "${work}/ai-softphone.zip"

    xcrun stapler staple "${app}" 2>&1 | sed 's/^/   /'

    spctl --assess --type exec --verbose=2 "${app}" 2>&1 | sed 's/^/   /'
fi

echo "== the image"
staging="${work}/dmg"
rm -rf "${staging}"
mkdir -p "${staging}"
cp -R "${app}" "${staging}/"

ln -s /Applications "${staging}/Applications"

image="${dist}/ai-softphone-${version}.dmg"
rm -f "${image}"

hdiutil create \
    -volname "AI Softphone ${version}" \
    -srcfolder "${staging}" \
    -ov -quiet \
    -format UDZO \
    "${image}"

codesign --force --sign "${identity}" --timestamp "${image}"
codesign --verify --strict --verbose=2 "${image}" 2>&1 | sed 's/^/   /'
echo "   ${image}"

if [[ ${notarise} -eq 0 ]]; then
    echo "== notarisation skipped"
    echo "   The image is signed and will open on this machine and on no other:"
    echo "   Gatekeeper refuses an un-notarised download. Run again without"
    echo "   --no-notarise before publishing it."
    exit 0
fi

echo "== notarising the image"
notarise "${image}"

xcrun stapler staple "${image}" 2>&1 | sed 's/^/   /'
xcrun stapler validate "${image}" 2>&1 | sed 's/^/   /'

spctl --assess --type open --context context:primary-signature --verbose=2 "${image}" 2>&1 |
    sed 's/^/   /'

echo "   ${image}"
