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

if [[ "$(uname -s)" != "Darwin" ]]; then
    echo "This script is macOS's. Linux takes both libraries from the" >&2
    echo "distribution — see scripts/install-build-deps.sh." >&2
    exit 1
fi

readonly OPENSSL_VERSION="3.5.7"
readonly OPENSSL_SHA256="a8c0d28a529ca480f9f36cf5792e2cd21984552a3c8e4aa11a24aa31aeac98e8"
readonly OPENSSL_URL="https://github.com/openssl/openssl/releases/download/openssl-${OPENSSL_VERSION}/openssl-${OPENSSL_VERSION}.tar.gz"

readonly OPUS_VERSION="1.5.2"
readonly OPUS_SHA256="65c1d2f78b9f2fb20082c38cbe47c951ad5839345876e46941612ee87f9a7ce1"
readonly OPUS_URL="https://downloads.xiph.org/releases/opus/opus-${OPUS_VERSION}.tar.gz"

readonly MACOS_MINIMUM="14.4"

readonly CACHE_ROOT="${SOFTPHONE_CACHE_DIR:-$HOME/.cache/softphone-ai-deps}"
readonly DOWNLOAD_DIR="${CACHE_ROOT}/downloads"
readonly WORK_ROOT="${CACHE_ROOT}/build"

readonly OPENSSL_PREFIX="${SOFTPHONE_OPENSSL_PREFIX:-${CACHE_ROOT}/openssl-${OPENSSL_VERSION}}"
readonly OPUS_PREFIX="${SOFTPHONE_OPUS_PREFIX:-${CACHE_ROOT}/opus-${OPUS_VERSION}}"

readonly ARCHES=(arm64 x86_64)

force=0
if [[ "${1:-}" == "--force" ]]; then
    force=1
fi

log() { printf '\033[1m[macos-deps]\033[0m %s\n' "$*"; }

sha256_of_file() { shasum -a 256 "$1" | cut -d' ' -f1; }
cpu_count() { sysctl -n hw.ncpu 2>/dev/null || echo 1; }

readonly SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SCRIPT_FINGERPRINT="$(sha256_of_file "${BASH_SOURCE[0]}")"
readonly SCRIPT_FINGERPRINT

fetch() {
    local url="$1" file="$2" want="$3"
    mkdir -p "${DOWNLOAD_DIR}"
    if [[ ! -f "${file}" ]]; then
        log "downloading $(basename "${file}")"
        curl -sSL -o "${file}" "${url}"
    fi
    local got
    got="$(sha256_of_file "${file}")"
    if [[ "${got}" != "${want}" ]]; then
        echo "Checksum mismatch for ${file}." >&2
        echo "  expected ${want}" >&2
        echo "  got      ${got}" >&2
        echo "Delete it and re-run, or update the pin if it moved." >&2
        exit 1
    fi
}

merge_universal() {
    local prefix="$1" first="$2" second="$3"

    rm -rf "${prefix}"
    mkdir -p "$(dirname "${prefix}")"
    cp -R "${first}" "${prefix}"

    local differing=()
    while IFS= read -r -d '' header; do
        local relative="${header#"${first}"/}"
        if [[ -f "${second}/${relative}" ]] \
            && ! cmp -s "${header}" "${second}/${relative}"; then
            differing+=("${relative}")
        fi
    done < <(find "${first}" -name '*.h' -print0)

    if [[ ${#differing[@]} -gt 0 ]]; then
        echo "These headers differ between the two architectures:" >&2
        printf '  %s\n' "${differing[@]}" >&2
        echo "A universal tree cannot carry one of each. Whichever build" >&2
        echo "generated them has to be told to agree, or the header has to" >&2
        echo "become a shim that picks on __arm64__." >&2
        exit 1
    fi

    local archive relative
    while IFS= read -r -d '' archive; do
        relative="${archive#"${first}"/}"
        if [[ -f "${second}/${relative}" ]]; then
            lipo -create "${archive}" "${second}/${relative}" \
                -output "${prefix}/${relative}"
        fi
    done < <(find "${first}" -name '*.a' -print0)
}

build_openssl() {
    local stamp="${OPENSSL_PREFIX}/.build-script-sha256"
    if [[ -f "${OPENSSL_PREFIX}/include/openssl/ssl.h" && "${force}" -eq 0 ]] \
        && [[ -f "${stamp}" && "$(cat "${stamp}")" == "${SCRIPT_FINGERPRINT}" ]]; then
        log "openssl ${OPENSSL_VERSION} already installed at ${OPENSSL_PREFIX}"
        return
    fi

    local tarball="${DOWNLOAD_DIR}/openssl-${OPENSSL_VERSION}.tar.gz"
    fetch "${OPENSSL_URL}" "${tarball}" "${OPENSSL_SHA256}"

    local staged=()
    for arch in "${ARCHES[@]}"; do
        local work="${WORK_ROOT}/openssl-${OPENSSL_VERSION}-${arch}"
        local stage="${WORK_ROOT}/openssl-${OPENSSL_VERSION}-${arch}-install"
        rm -rf "${work}" "${stage}"
        mkdir -p "${work}"
        tar -xzf "${tarball}" -C "${work}" --strip-components=1

        log "openssl ${OPENSSL_VERSION}: configuring for ${arch}"
        (
            cd "${work}"
            MACOSX_DEPLOYMENT_TARGET="${MACOS_MINIMUM}" ./Configure \
                "darwin64-${arch}-cc" \
                no-shared no-tests no-docs \
                --prefix="${stage}" \
                --openssldir="${stage}/ssl" \
                > configure.log 2>&1 \
                || { tail -30 configure.log >&2; exit 1; }

            log "openssl ${OPENSSL_VERSION}: building for ${arch}"
            MACOSX_DEPLOYMENT_TARGET="${MACOS_MINIMUM}" make -j"$(cpu_count)" \
                > make.log 2>&1 \
                || { tail -40 make.log >&2; exit 1; }

            make install_sw > install.log 2>&1 \
                || { tail -30 install.log >&2; exit 1; }
        )
        staged+=("${stage}")
    done

    log "openssl ${OPENSSL_VERSION}: merging ${ARCHES[*]}"
    merge_universal "${OPENSSL_PREFIX}" "${staged[0]}" "${staged[1]}"
    printf '%s\n' "${SCRIPT_FINGERPRINT}" > "${stamp}"
}

build_opus() {
    local stamp="${OPUS_PREFIX}/.build-script-sha256"
    if [[ -f "${OPUS_PREFIX}/lib/pkgconfig/opus.pc" && "${force}" -eq 0 ]] \
        && [[ -f "${stamp}" && "$(cat "${stamp}")" == "${SCRIPT_FINGERPRINT}" ]]; then
        log "opus ${OPUS_VERSION} already installed at ${OPUS_PREFIX}"
        return
    fi

    local tarball="${DOWNLOAD_DIR}/opus-${OPUS_VERSION}.tar.gz"
    fetch "${OPUS_URL}" "${tarball}" "${OPUS_SHA256}"

    local staged=()
    for arch in "${ARCHES[@]}"; do
        local work="${WORK_ROOT}/opus-${OPUS_VERSION}-${arch}"
        local stage="${WORK_ROOT}/opus-${OPUS_VERSION}-${arch}-install"
        rm -rf "${work}" "${stage}"
        mkdir -p "${work}"
        tar -xzf "${tarball}" -C "${work}" --strip-components=1

        local host="${arch}-apple-darwin"

        log "opus ${OPUS_VERSION}: configuring for ${arch}"
        (
            cd "${work}"
            ./configure \
                --prefix="${stage}" \
                --host="${host}" \
                --disable-shared \
                --enable-static \
                --disable-doc \
                --disable-extra-programs \
                CC="clang -arch ${arch}" \
                CFLAGS="-arch ${arch} -mmacosx-version-min=${MACOS_MINIMUM} -O2" \
                > configure.log 2>&1 \
                || { tail -30 configure.log >&2; exit 1; }

            log "opus ${OPUS_VERSION}: building for ${arch}"
            make -j"$(cpu_count)" > make.log 2>&1 \
                || { tail -40 make.log >&2; exit 1; }

            make install > install.log 2>&1 \
                || { tail -30 install.log >&2; exit 1; }
        )
        staged+=("${stage}")
    done

    log "opus ${OPUS_VERSION}: merging ${ARCHES[*]}"
    merge_universal "${OPUS_PREFIX}" "${staged[0]}" "${staged[1]}"
    printf '%s\n' "${SCRIPT_FINGERPRINT}" > "${stamp}"
}

build_openssl
build_opus

log "reading back what was built"
for archive in "${OPENSSL_PREFIX}/lib/libssl.a" "${OPENSSL_PREFIX}/lib/libcrypto.a" \
               "${OPUS_PREFIX}/lib/libopus.a"; do
    if [[ ! -f "${archive}" ]]; then
        echo "Missing after install: ${archive}" >&2
        exit 1
    fi
    got="$(lipo -archs "${archive}")"
    for arch in "${ARCHES[@]}"; do
        if [[ "${got}" != *"${arch}"* ]]; then
            echo "${archive} is [${got}] and does not carry ${arch}." >&2
            exit 1
        fi
    done
    log "$(basename "${archive}"): ${got}"
done

log "done"
echo "OPENSSL ${OPENSSL_PREFIX}"
echo "OPUS ${OPUS_PREFIX}"
