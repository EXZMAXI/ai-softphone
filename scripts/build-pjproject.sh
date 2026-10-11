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

readonly PJPROJECT_VERSION="2.17"
readonly PJPROJECT_SHA256="065fe06c06788d97c35f563796d59f00ce52fe9558a52d7b490a042a966facce"

readonly CACHE_ROOT="${SOFTPHONE_CACHE_DIR:-$HOME/.cache/softphone-ai-deps}"
readonly DOWNLOAD_DIR="${CACHE_ROOT}/downloads"
readonly PREFIX="${SOFTPHONE_PJPROJECT_PREFIX:-${CACHE_ROOT}/pjproject-${PJPROJECT_VERSION}}"
readonly BUILD_DIR="${CACHE_ROOT}/build/pjproject-${PJPROJECT_VERSION}"

readonly TARBALL="${DOWNLOAD_DIR}/pjproject-${PJPROJECT_VERSION}.tar.gz"
readonly URL="https://github.com/pjsip/pjproject/archive/refs/tags/${PJPROJECT_VERSION}.tar.gz"
readonly STAMP="${PREFIX}/lib/pkgconfig/libpjproject.pc"

force=0
if [[ "${1:-}" == "--force" ]]; then
    force=1
fi

log() { printf '\033[1m[pjproject]\033[0m %s\n' "$*"; }

sha256_of_stdin() {
    if command -v sha256sum > /dev/null 2>&1; then
        sha256sum | cut -d' ' -f1
    else
        shasum -a 256 | cut -d' ' -f1
    fi
}

sha256_of_file() {
    if command -v sha256sum > /dev/null 2>&1; then
        sha256sum "$1" | cut -d' ' -f1
    else
        shasum -a 256 "$1" | cut -d' ' -f1
    fi
}

cpu_count() {
    if command -v nproc > /dev/null 2>&1; then
        nproc
    elif command -v sysctl > /dev/null 2>&1; then
        sysctl -n hw.ncpu 2>/dev/null || echo 1
    else
        echo 1
    fi
}

readonly SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
readonly CONFIG_SITE="${SCRIPT_DIR}/pjproject-config_site.h"
readonly PATCHER="${SCRIPT_DIR}/patch-pjproject.py"
SCRIPT_FINGERPRINT="$(cat "${BASH_SOURCE[0]}" "${CONFIG_SITE}" "${PATCHER}" | sha256_of_stdin)"
readonly SCRIPT_FINGERPRINT
readonly FINGERPRINT_FILE="${PREFIX}/.build-script-sha256"

if [[ -f "${STAMP}" && "${force}" -eq 0 ]]; then
    if [[ -f "${FINGERPRINT_FILE}" ]] && \
       [[ "$(cat "${FINGERPRINT_FILE}")" == "${SCRIPT_FINGERPRINT}" ]]; then
        log "already installed at ${PREFIX}"
        log "pass --force to rebuild"
        echo "${PREFIX}"
        exit 0
    fi
    log "this script changed since the install was built; rebuilding"
fi

mkdir -p "${DOWNLOAD_DIR}"

if [[ ! -f "${TARBALL}" ]]; then
    log "downloading ${PJPROJECT_VERSION}"
    curl -sSL -o "${TARBALL}" "${URL}"
fi

log "verifying checksum"
if [[ "$(sha256_of_file "${TARBALL}")" != "${PJPROJECT_SHA256}" ]]; then
    echo "Checksum mismatch for ${TARBALL}." >&2
    echo "Delete it and re-run, or update PJPROJECT_SHA256 if the pin changed." >&2
    exit 1
fi

log "extracting into ${BUILD_DIR}"
rm -rf "${BUILD_DIR}"
mkdir -p "${BUILD_DIR}"
tar -xzf "${TARBALL}" -C "${BUILD_DIR}" --strip-components=1

cat > "${BUILD_DIR}/pjlib/include/pj/config_site.h" <<'CONFIG'
/*
 * Site configuration for pjproject.
 *
 * Written by scripts/build-pjproject.sh. Everything not named here is left at
 * pjproject's default on purpose.
 *
 * Both limits below are compile-time array sizes, not policies: pjsua allocates
 * fixed tables from them and asserts when one runs out, which ends the process
 * rather than returning an error. Raising them is only possible here, which is
 * why the numbers live in the build script rather than anywhere nearer the code
 * that runs into them.
 */

/*
 * Default 4. A softphone that supports "answer this and hold that" runs out at
 * three held calls plus one active, and the fourth caller fails for a reason
 * nobody could guess. 16 is generous for a desk — pjproject's own high-load
 * sample uses 512 — and the cost is real but small: fixed arrays sized from it,
 * allocated whether or not anyone makes a call.
 *
 * The product's own ceiling is CallPolicy::maxConcurrentCalls, which is what
 * decides when a caller hears a busy signal. This is the wall behind it.
 */
#define PJSUA_MAX_CALLS 16

/*
 * Default 8. Not raised for its own sake: an account only gives its slot back
 * when the account object is destroyed, which is a thing we now do — see
 * ISipBackend::forgetAccount. 32 leaves room for a person with several
 * registrations to add and remove them freely without the count of ids *ever
 * seen* mattering.
 */
#define PJSUA_MAX_ACC 32
CONFIG

log "correcting"
python3 "${PATCHER}" "${BUILD_DIR}"

configure_options=(
    --disable-video
    --disable-sdl
    --disable-ffmpeg
    --disable-v4l2
    --disable-openh264
    --disable-vpx
)

if [[ "$(uname -s)" != "Darwin" ]]; then
    log "configuring"
    cd "${BUILD_DIR}"

    ./configure \
        --prefix="${PREFIX}" \
        "${configure_options[@]}" \
        CFLAGS="-fPIC -O2" \
        CXXFLAGS="-fPIC -O2" \
        > "${BUILD_DIR}/configure.log" 2>&1 \
        || { tail -30 "${BUILD_DIR}/configure.log" >&2; exit 1; }

    log "building"
    make dep > "${BUILD_DIR}/make-dep.log" 2>&1 \
        || { tail -30 "${BUILD_DIR}/make-dep.log" >&2; exit 1; }
    make -j"$(cpu_count)" > "${BUILD_DIR}/make.log" 2>&1 \
        || { tail -40 "${BUILD_DIR}/make.log" >&2; exit 1; }

    log "installing into ${PREFIX}"
    rm -rf "${PREFIX}"
    make install > "${BUILD_DIR}/make-install.log" 2>&1 \
        || { tail -30 "${BUILD_DIR}/make-install.log" >&2; exit 1; }
else
    readonly MACOS_MINIMUM="14.4"
    readonly OPENSSL_PREFIX="${SOFTPHONE_OPENSSL_PREFIX:-${CACHE_ROOT}/openssl-3.5.7}"
    readonly OPUS_PREFIX="${SOFTPHONE_OPUS_PREFIX:-${CACHE_ROOT}/opus-1.5.2}"

    for needed in "${OPENSSL_PREFIX}/include/openssl/ssl.h" "${OPUS_PREFIX}/include/opus/opus.h"; do
        if [[ ! -f "${needed}" ]]; then
            echo "Missing ${needed}." >&2
            echo "Build the dependencies first: ./scripts/build-macos-deps.sh" >&2
            exit 1
        fi
    done

    staged=()
    for arch in arm64 x86_64; do
        work="${BUILD_DIR}-${arch}"
        stage="${BUILD_DIR}-${arch}-install"
        rm -rf "${work}" "${stage}"
        cp -R "${BUILD_DIR}" "${work}"

        log "configuring for ${arch}"
        (
            cd "${work}"

            ./configure \
                --prefix="${stage}" \
                --host="${arch}-apple-darwin" \
                "${configure_options[@]}" \
                --with-ssl="${OPENSSL_PREFIX}" \
                --with-opus="${OPUS_PREFIX}" \
                CFLAGS="-arch ${arch} -mmacosx-version-min=${MACOS_MINIMUM} -fPIC -O2" \
                CXXFLAGS="-arch ${arch} -mmacosx-version-min=${MACOS_MINIMUM} -fPIC -O2" \
                LDFLAGS="-arch ${arch} -mmacosx-version-min=${MACOS_MINIMUM}" \
                > configure.log 2>&1 \
                || { tail -40 configure.log >&2; exit 1; }

            log "building for ${arch}"
            make dep > make-dep.log 2>&1 || { tail -30 make-dep.log >&2; exit 1; }
            make -j"$(cpu_count)" > make.log 2>&1 || { tail -40 make.log >&2; exit 1; }
            make install > make-install.log 2>&1 || { tail -30 make-install.log >&2; exit 1; }
        )
        staged+=("${stage}")
    done

    log "merging arm64 x86_64 into ${PREFIX}"
    rm -rf "${PREFIX}"
    cp -R "${staged[0]}" "${PREFIX}"

    readonly HOST_SUFFIX="aarch64-apple-darwin"
    readonly OTHER_SUFFIX="x86_64-apple-darwin"
    readonly MERGED_SUFFIX="universal-apple-darwin"

    merged=0
    while IFS= read -r -d '' archive; do
        base="$(basename "${archive}")"
        other="${staged[1]}/lib/${base/${HOST_SUFFIX}/${OTHER_SUFFIX}}"
        if [[ ! -f "${other}" ]]; then
            echo "No ${OTHER_SUFFIX} counterpart for ${base}." >&2
            exit 1
        fi
        rm -f "${PREFIX}/lib/${base}"
        lipo -create "${archive}" "${other}" \
            -output "${PREFIX}/lib/${base/${HOST_SUFFIX}/${MERGED_SUFFIX}}"
        merged=$((merged + 1))
    done < <(find "${staged[0]}/lib" -maxdepth 1 -name "*${HOST_SUFFIX}.a" -print0)

    if [[ "${merged}" -eq 0 ]]; then
        echo "Nothing was merged: no *${HOST_SUFFIX}.a under ${staged[0]}/lib." >&2
        exit 1
    fi
    log "merged ${merged} archive(s)"

    while IFS= read -r -d '' text; do
        LC_ALL=C sed -i '' "s|${staged[0]}|${PREFIX}|g; s|${HOST_SUFFIX}|${MERGED_SUFFIX}|g" \
            "${text}"
    done < <(find "${PREFIX}/lib/pkgconfig" -type f -print0 2>/dev/null)
fi

if [[ ! -f "${STAMP}" ]]; then
    echo "Install finished but ${STAMP} is missing." >&2
    exit 1
fi

if ! ls "${PREFIX}"/lib/libpjsua2*.a > /dev/null 2>&1; then
    echo "pjsua2 was not built — the C++ API is required." >&2
    exit 1
fi

if [[ "$(uname -s)" == "Darwin" ]]; then
    for archive in "${PREFIX}"/lib/libpjsua2*.a "${PREFIX}"/lib/libpjmedia-[a-z]*.a; do
        [[ -f "${archive}" ]] || continue
        got="$(lipo -archs "${archive}")"
        for arch in arm64 x86_64; do
            if [[ "${got}" != *"${arch}"* ]]; then
                echo "$(basename "${archive}") is [${got}] and does not carry ${arch}." >&2
                exit 1
            fi
        done
        log "$(basename "${archive}"): ${got}"
    done
fi

printf '%s\n' "${SCRIPT_FINGERPRINT}" > "${FINGERPRINT_FILE}"

log "done"
echo "${PREFIX}"
