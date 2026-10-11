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
dist="${root}/dist/linux"
work="${build}/staging"

runtime_url="https://github.com/AppImage/type2-runtime/releases/download/continuous/runtime-x86_64"
runtime_sha256="1cc49bcf1e2ccd593c379adb17c9f85a36d619088296504de95b1d06215aebbf"
runtime_cache="${HOME}/.cache/softphone-ai-deps/appimage-runtime-x86_64"

what=${1:-all}
case "${what}" in
    deb | appimage | all) ;;
    *)
        echo "usage: $0 [deb|appimage|all]" >&2
        exit 2
        ;;
esac

missing=()
for tool in dpkg-deb fakeroot mksquashfs patchelf; do
    command -v "${tool}" > /dev/null 2>&1 || missing+=("${tool}")
done
if ((${#missing[@]} > 0)); then
    echo "Not installed: ${missing[*]}" >&2
    echo "  sudo apt-get install dpkg-dev fakeroot squashfs-tools patchelf" >&2
    echo "  or run scripts/install-build-deps.sh, which now names all of them." >&2
    exit 2
fi

if [[ ! -d ${build} ]]; then
    echo "No packaging build. Run:" >&2
    echo "  cmake --preset package && cmake --build --preset package" >&2
    exit 1
fi

mkdir -p "${dist}"

built_version() {
    local v
    v=$(sed -n 's/^CMAKE_PROJECT_VERSION:STATIC=//p' "${build}/CMakeCache.txt" | head -1)
    if [[ -z ${v} ]]; then
        echo "Could not read the version out of ${build}/CMakeCache.txt" >&2
        exit 1
    fi
    echo "${v}"
}

lay_out() {
    cmake -S "${root}" -B "${build}" \
        -DCMAKE_INSTALL_PREFIX="$1" \
        -DSOFTPHONE_DESKTOP_DATA_DIR="$2" \
        -DSOFTPHONE_LINK_BINDIR="$3" \
        -DSOFTPHONE_DESKTOP_EXEC="$4" \
        -DSOFTPHONE_PACKAGE_FORMAT="$5" \
        > /dev/null
    cmake --build "${build}" > /dev/null
}

make_deb() {
    echo "== the .deb"

    lay_out "/opt/ai-softphone" "/usr/share" "/usr/bin" "" "deb"

    rm -rf "${build}/_CPack_Packages"
    (cd "${build}" && cpack -G DEB) | sed 's/^/   /'

    local package
    package=$(find "${build}" -maxdepth 1 -name '*.deb' -newermt '-5 minutes' | head -1)
    if [[ -z ${package} ]]; then
        echo "cpack produced no .deb" >&2
        exit 1
    fi

    rm -rf "${work}/deb"
    mkdir -p "${work}/deb"
    dpkg-deb -x "${package}" "${work}/deb"
    "${root}/scripts/check-package-tree.sh" \
        "${work}/deb/opt/ai-softphone" \
        --data-root "${work}/deb/usr/share" \
        --root "${work}/deb" 2>&1 | sed 's/^/   /'

    mv -f "${package}" "${dist}/"
    echo "   ${dist}/$(basename "${package}")"
}

carry_what_it_can() {
    local appdir=$1
    local libdir="${appdir}/usr/lib/x86_64-linux-gnu"
    local list="${root}/packaging/linux/from-the-machine"
    mkdir -p "${libdir}"

    if [[ ! -r ${list} ]]; then
        echo "No ${list}, so nothing can say what may be carried." >&2
        exit 1
    fi

    local machines
    machines=$(grep -vE '^[[:space:]]*(#|$)' "${list}")

    local round added total=0
    for round in 1 2 3 4 5 6 7 8; do
        added=0
        local object soname path
        while read -r object; do
            file -b "${object}" 2> /dev/null | grep -q ELF || continue
            [[ ${object} == */platformthemes/libqgtk3.so ]] && continue
            while read -r soname path; do
                [[ -z ${path} || ${path} != /* ]] && continue
                [[ -e "${libdir}/${soname}" ]] && continue
                grep -qxF "${soname}" <<< "${machines}" && continue
                cp -L "${path}" "${libdir}/${soname}"
                chmod u+w "${libdir}/${soname}"
                patchelf --set-rpath '$ORIGIN' "${libdir}/${soname}"
                echo "${path}" >> "${carried_from}"
                added=$((added + 1))
            done < <(ldd "${object}" 2> /dev/null | awk '/=> \//{ print $1, $3 }')
        done < <(find "${appdir}" -type f \( -name '*.so*' -o -perm -u+x \))

        total=$((total + added))
        ((added == 0)) && break
    done

    echo "   carried ${total} more libraries out of the build machine"
}

carry_their_copyrights() {
    local appdir=$1
    local into="${appdir}/usr/share/doc/ai-softphone/third-party"
    mkdir -p "${into}"

    [[ -s ${carried_from} ]] || { echo "   nothing was carried, so no notices to carry"; return; }

    local qt="${QT_ROOT_DIR:-/nonexistent-qt}"
    local cache="${HOME}/.cache/softphone-ai-deps"

    local path package doc copied=0 covered=0
    local -a nameless=()
    while read -r path; do
        if [[ ${path} == "${qt}"/* || ${path} == "${cache}"/* ]]; then
            covered=$((covered + 1))
            continue
        fi
        local resolved base
        resolved=$(readlink -f "${path}" 2> /dev/null || true)
        base=$(basename "${resolved:-${path}}")
        package=$(dpkg -S "${resolved:-${path}}" 2> /dev/null | head -1 | cut -d: -f1 || true)
        if [[ -z ${package} && ${resolved} != "${path}" ]]; then
            package=$(dpkg -S "${path}" 2> /dev/null | head -1 | cut -d: -f1 || true)
        fi
        if [[ -z ${package} ]]; then
            package=$(dpkg -S "${base}" 2> /dev/null \
                | awk -F': ' -v want="/${base}" \
                      'index($2, want) == length($2) - length(want) + 1 { print $1; exit }' \
                | cut -d: -f1 || true)
        fi
        if [[ -z ${package} ]]; then
            nameless+=("${path}")
            continue
        fi
        doc="/usr/share/doc/${package}/copyright"
        if [[ ! -r ${doc} ]]; then
            nameless+=("${path} (${package} has no copyright file)")
            continue
        fi
        [[ -e "${into}/${package}.copyright" ]] && continue
        cp "${doc}" "${into}/${package}.copyright"
        copied=$((copied + 1))
    done < <(sort -u "${carried_from}")

    if ((${#nameless[@]} > 0)); then
        echo "These carried libraries have no copyright statement to travel with them:" >&2
        printf '   %s\n' "${nameless[@]}" >&2
        echo "A package may not redistribute a library whose licence it cannot state." >&2
        exit 1
    fi

    cat > "${into}/README" <<'NOTICE'
The files beside this one are the copyright and licence statements that the
distribution this package was built on makes about the shared libraries carried
inside it - one file per package, copied verbatim and not summarised.

They are here because those licences ask for it: BSD-3 asks for its notice to be
reproduced in the materials of a binary redistribution, the Lesser GPL asks for a
prominent notice and a copy, MIT asks for its notice in every substantial portion
of the work.

The components this product builds from source or compiles into its own binaries
are in ../THIRD-PARTY-LICENCES instead, and this product's own licence is in
../copyright.
NOTICE

    echo "   carried ${copied} copyright statement(s) from the build machine;" \
         "${covered} more are answered by THIRD-PARTY-LICENCES"
}

make_appimage() {
    echo "== the AppImage"

    if [[ ! -f ${runtime_cache} ]]; then
        echo "   fetching the AppImage runtime"
        mkdir -p "$(dirname "${runtime_cache}")"
        curl -sSL --fail -o "${runtime_cache}.part" "${runtime_url}"
        mv -f "${runtime_cache}.part" "${runtime_cache}"
    fi

    local seen
    seen=$(sha256sum "${runtime_cache}" | cut -d' ' -f1)
    if [[ ${seen} != "${runtime_sha256}" ]]; then
        echo "The AppImage runtime is not the one this script was written against." >&2
        echo "  expected ${runtime_sha256}" >&2
        echo "  found    ${seen}" >&2
        echo "Delete ${runtime_cache}, look at what upstream published, and re-pin." >&2
        exit 1
    fi

    local appdir="${work}/AppDir"
    rm -rf "${appdir}"
    lay_out "/usr" "share" "" "ai-softphone" "appimage"
    cmake --install "${build}" --prefix "${appdir}/usr" > /dev/null

    carried_from="${work}/carried-from.txt"
    : > "${carried_from}"
    carry_what_it_can "${appdir}"

    "${root}/scripts/check-package-tree.sh" "${appdir}/usr" \
        --carries "${root}/packaging/linux/from-the-machine" 2>&1 | sed 's/^/   /'

    cp "${appdir}/usr/share/applications/ai-softphone.desktop" "${appdir}/"
    cp "${appdir}/usr/share/icons/hicolor/256x256/apps/ai-softphone.png" "${appdir}/"
    cp "${appdir}/ai-softphone.png" "${appdir}/.DirIcon"
    cp "${root}/packaging/appimage/AppRun" "${appdir}/AppRun"
    chmod +x "${appdir}/AppRun"

    mkdir -p "${appdir}/usr/lib/ai-softphone/runtime"
    local runtime found
    for runtime in libstdc++.so.6 libgcc_s.so.1; do
        found=$(gcc -print-file-name="${runtime}")
        if [[ ! -f ${found} ]]; then
            echo "The C++ runtime ${runtime} is not where the compiler says it is." >&2
            exit 1
        fi
        cp -L "${found}" "${appdir}/usr/lib/ai-softphone/runtime/${runtime}"
        echo "${found}" >> "${carried_from}"
    done

    carry_their_copyrights "${appdir}"

    {
        echo "glibc=$(objdump -T "${appdir}/usr/bin/ai-softphone" \
            | grep -oE 'GLIBC_[0-9.]+' | sed 's/GLIBC_//' | sort -u -V | tail -1)"
        echo "libstdcxx=$(objdump -T "${appdir}/usr/lib/ai-softphone/runtime/libstdc++.so.6" \
            | grep -oE 'GLIBCXX_[0-9.]+' | sed 's/GLIBCXX_//' | sort -u -V | tail -1)"
    } > "${appdir}/usr/lib/ai-softphone/needs"

    local image="${dist}/ai-softphone-$(built_version)-x86_64.AppImage"
    rm -f "${image}" "${work}/filesystem.squashfs"

    mksquashfs "${appdir}" "${work}/filesystem.squashfs" \
        -root-owned -noappend -no-progress -quiet -comp zstd -Xcompression-level 19

    cat "${runtime_cache}" "${work}/filesystem.squashfs" > "${image}"
    chmod +x "${image}"
    rm -f "${work}/filesystem.squashfs"

    echo "   ${image}"
}

case "${what}" in
    deb) make_deb ;;
    appimage) make_appimage ;;
    all)
        make_deb
        make_appimage
        ;;
esac

echo
ls -lh "${dist}"
