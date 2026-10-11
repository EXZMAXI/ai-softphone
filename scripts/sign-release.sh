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

if [[ $# -lt 1 ]]; then
    echo "usage: $0 <private key> --base-url <url> [--base-url <url> ...]" \
         "[--notes <url>] [dist directory]" >&2
    exit 2
fi

key=$1
shift

base_urls=()
if [[ -n ${SOFTPHONE_RELEASE_BASE_URL:-} ]]; then
    base_urls+=("${SOFTPHONE_RELEASE_BASE_URL}")
fi
notes=${SOFTPHONE_RELEASE_NOTES_URL:-}
dist=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        --base-url)
            base_urls+=("$2")
            shift 2
            ;;
        --notes)
            notes=$2
            shift 2
            ;;
        *)
            dist=$(realpath "$1")
            shift
            ;;
    esac
done

if [[ ! -f ${key} ]]; then
    echo "No signing key at ${key}. See scripts/make-signing-key.sh." >&2
    exit 1
fi

if [[ ${#base_urls[@]} -eq 0 ]]; then
    echo "No --base-url. The manifest has to say where the packages are." >&2
    exit 2
fi

if [[ -z ${dist} ]]; then
    echo "No directory to sign. Name one platform's:" >&2
    echo "  $0 <key> --base-url ${base_urls[0]} dist/linux" >&2
    echo "  $0 <key> --base-url ${base_urls[0]} dist/windows" >&2
    exit 2
fi

for at in "${!base_urls[@]}"; do
    base_urls[${at}]="${base_urls[${at}]%/}/"
done

if [[ ! -d ${dist} ]]; then
    echo "Nothing to sign: ${dist} does not exist. Run scripts/make-package.sh first." >&2
    exit 1
fi

platform=$(basename "${dist%/}")
case ${platform} in
    linux | windows | macos) ;;
    *)
        echo "Warning: ${dist} is not named after a platform, so the URLs cannot be checked." >&2
        platform=""
        ;;
esac

if [[ -n ${platform} ]]; then
    for base_url in "${base_urls[@]}"; do
        if [[ ${base_url} != */${platform}/ ]]; then
            echo "The base URL does not end in /${platform}:" >&2
            echo "  ${base_url}" >&2
            echo >&2
            echo "A manifest read from <base>/${platform}/manifest.json has to name packages" >&2
            echo "beside it. cmake/Updates.cmake lists the bases without the platform" >&2
            echo "because CMake adds it; this script does not. You want:" >&2
            echo "  ${base_url%/}/${platform}" >&2
            exit 2
        fi
    done
fi

only_one() {
    local what=$1 pattern=$2
    local found
    found=$(find "${dist}" -maxdepth 1 -name "${pattern}" | sort)
    if [[ $(echo "${found}" | grep -c .) -gt 1 ]]; then
        echo "More than one ${what} in ${dist}:" >&2
        echo "${found}" >&2
        echo >&2
        echo "A manifest describes one release. Move the others away first." >&2
        exit 2
    fi
    echo "${found}"
}

image=$(only_one "AppImage" '*.AppImage')
package=$(only_one ".deb" '*.deb')
installer=$(only_one "installer" '*.exe')
disk=$(only_one "disk image" '*.dmg')

if [[ -z ${image} && -z ${package} && -z ${installer} && -z ${disk} ]]; then
    echo "Nothing to sign: no .AppImage, no .deb, no .exe and no .dmg in ${dist}." >&2
    exit 1
fi

kinds=0
[[ -n ${image} || -n ${package} ]] && kinds=$((kinds + 1))
[[ -n ${installer} ]] && kinds=$((kinds + 1))
[[ -n ${disk} ]] && kinds=$((kinds + 1))
if [[ ${kinds} -gt 1 ]]; then
    echo "Refusing to write one manifest for two platforms." >&2
    echo "  ${dist} holds packages for more than one of them." >&2
    echo "Each platform publishes into a directory of its own; sign them separately." >&2
    exit 1
fi

version=""
for candidate in "${image}" "${package}" "${installer}" "${disk}"; do
    [[ -z ${candidate} ]] && continue
    version=$(basename "${candidate}" | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -1)
    [[ -n ${version} ]] && break
done

if [[ -z ${version} ]]; then
    echo "Could not read a version out of the file names in ${dist}." >&2
    exit 1
fi

if stat -c%s "${BASH_SOURCE[0]}" > /dev/null 2>&1; then
    size_of() { stat -c%s "$1"; }
else
    size_of() { stat -f%z "$1"; }
fi

if command -v sha256sum > /dev/null 2>&1; then
    digest_of() { sha256sum "$1" | cut -d' ' -f1; }
else
    digest_of() { shasum -a 256 "$1" | cut -d' ' -f1; }
fi

if base64 -w0 < /dev/null > /dev/null 2>&1; then
    flat_base64() { base64 -w0; }
else
    flat_base64() { base64; }
fi

choose_openssl() {
    local candidate
    for candidate in \
        "${OPENSSL:-}" \
        openssl \
        "${SOFTPHONE_CACHE_DIR:-${HOME}/.cache/softphone-ai-deps}"/openssl-*/bin/openssl \
        /opt/homebrew/opt/openssl@3/bin/openssl \
        /usr/local/opt/openssl@3/bin/openssl; do
        [[ -z ${candidate} ]] && continue
        command -v "${candidate}" > /dev/null 2>&1 || continue
        if "${candidate}" genpkey -algorithm ED25519 -out /dev/null > /dev/null 2>&1; then
            echo "${candidate}"
            return 0
        fi
    done
    return 1
}

if ! openssl_bin=$(choose_openssl); then
    echo "No openssl here can do Ed25519, which is what the release key is." >&2
    echo "macOS ships LibreSSL as /usr/bin/openssl and it cannot read the key." >&2
    echo "Build this project's own dependencies — ./scripts/build-macos-deps.sh —" >&2
    echo "or name one: OPENSSL=/path/to/openssl $0 ..." >&2
    exit 2
fi

entry() {
    local format=$1 file=$2
    [[ -z ${file} ]] && return 0

    local name size digest
    name=$(basename "${file}")
    size=$(size_of "${file}")
    digest=$(digest_of "${file}")

    printf '        {\n'
    printf '            "format": "%s",\n' "${format}"
    printf '            "url": "%s%s",\n' "${base_url}" "${name}"
    printf '            "size": %s,\n' "${size}"
    printf '            "sha256": "%s"\n' "${digest}"
    printf '        }'
}

hostOf() {
    local url=$1
    url=${url#*://}
    echo "${url%%/*}"
}

public_der=$(mktemp)
trap 'rm -f "${public_der}"' EXIT
"${openssl_bin}" pkey -in "${key}" -pubout -out "${public_der}"

wrote=()

for base_url in "${base_urls[@]}"; do
    host=$(hostOf "${base_url}")
    into="${dist}/for-${host}"
    mkdir -p "${into}"
    manifest="${into}/manifest.json"

    {
        printf '{\n'
        printf '    "product": "ai-softphone",\n'
        printf '    "version": "%s",\n' "${version}"
        if [[ -n ${notes} ]]; then
            printf '    "notes": "%s",\n' "${notes}"
        fi
        printf '    "packages": [\n'

        written=0
        for pair in "appimage:${image}" "deb:${package}" "installer:${installer}" "dmg:${disk}"; do
            format=${pair%%:*}
            file=${pair#*:}
            [[ -z ${file} ]] && continue
            [[ ${written} -eq 1 ]] && printf ',\n'
            entry "${format}" "${file}"
            written=1
        done

        printf '\n    ]\n'
        printf '}\n'
    } > "${manifest}"

    "${openssl_bin}" pkeyutl -sign -inkey "${key}" -rawin -in "${manifest}" |
        flat_base64 > "${manifest}.sig"

    signature_raw=$(mktemp)
    base64 -d < "${manifest}.sig" > "${signature_raw}"

    if ! "${openssl_bin}" pkeyutl -verify -pubin -inkey "${public_der}" -rawin -in "${manifest}" \
        -sigfile "${signature_raw}" > /dev/null 2>&1; then
        rm -f "${signature_raw}"
        echo "The signature this script just wrote for ${host} does not verify." >&2
        echo "Nothing was published." >&2
        exit 1
    fi
    rm -f "${signature_raw}"

    wrote+=("${host}")
done

echo "Version:   ${version}"
echo "Signed for ${#wrote[@]} host(s):"
for host in "${wrote[@]}"; do
    echo "  ${dist}/for-${host}/manifest.json  + .sig"
done
echo
echo "Publish each host's pair beside the packages on that host. The packages"
echo "themselves are the same bytes everywhere; only the manifest differs."
echo
cat "${dist}/for-${wrote[0]}/manifest.json"
echo
echo "Public key of the signing key used:"
echo "  $("${openssl_bin}" pkey -in "${key}" -pubout -outform DER | tail -c 32 | flat_base64)"
