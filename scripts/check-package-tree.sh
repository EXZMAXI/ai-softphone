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
    echo "usage: $0 <staged tree> [--data-root <directory>]" >&2
    exit 2
fi

tree=$(realpath "$1")

plugins_in_files=()
while read -r one; do
    plugins_in_files+=("${one}")
done < <("$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/plugins-in-files.py")
if [[ ${#plugins_in_files[@]} -eq 0 ]]; then
    echo "Could not work out which plugins ship; this check would pass by asking nothing." >&2
    exit 2
fi
shift

data_root="${tree}/share"
root=""
carries=""
while [[ $# -gt 0 ]]; do
    case "$1" in
        --data-root)
            data_root=$(realpath "$2")
            shift 2
            ;;
        --root)
            root=$(realpath "$2")
            shift 2
            ;;
        --carries)
            carries=$(realpath "$2")
            shift 2
            ;;
        *)
            echo "unknown argument: $1" >&2
            exit 2
            ;;
    esac
done

failures=0

complain() {
    echo "  FAIL  $*" >&2
    failures=$((failures + 1))
}

approve() {
    echo "  ok    $*"
}

echo "Checking ${tree}"

if [[ -d "${tree}/Contents/MacOS" ]]; then
    contents="${tree}/Contents"

    for binary in ai-softphone ai-softphone-cli; do
        path="${contents}/MacOS/${binary}"
        if [[ ! -x ${path} ]]; then
            complain "${binary} is missing from Contents/MacOS/"
            continue
        fi
        reported=$(env -i "${path}" --version 2>&1) || {
            complain "${binary} --version did not succeed: ${reported}"
            continue
        }
        approve "${binary} starts and says: ${reported}"
    done

    outside=""
    while IFS= read -r -d '' object; do
        file -b "${object}" 2> /dev/null | grep -q "Mach-O" || continue
        while read -r name; do
            case "${name}" in
                @rpath/* | @loader_path/* | @executable_path/*) ;;
                /usr/lib/* | /System/Library/*) ;;
                "") ;;
                *) outside+="${object#"${tree}"/} needs ${name}"$'\n' ;;
            esac
        done < <(otool -L "${object}" 2> /dev/null | awk '/^\t/ { print $1 }')
    done < <(find "${tree}" -type f -print0)

    if [[ -n ${outside} ]]; then
        complain "something in the bundle names a path outside it:"$'\n'"${outside}"
    else
        approve "every library is named relative to the bundle"
    fi

    for plugin in "${plugins_in_files[@]}"; do
        directory="${contents}/Resources/ai-softphone/${plugin}"
        if [[ ! -f ${directory}/manifest.json ]]; then
            complain "the ${plugin} plugin has no manifest.json"
        elif [[ ! -f ${directory}/libsoftphone_plugin_${plugin}.dylib ]]; then
            complain "the ${plugin} plugin has a manifest and no library"
        elif [[ ! -f ${directory}/libsoftphone_plugin_${plugin}_ui.dylib ]]; then
            complain "the ${plugin} plugin has no interface half: it will contribute nothing to any window"
        else
            approve "the ${plugin} plugin is a pair of files the phone can open"
        fi
    done

    for wanted in \
        platforms/libqcocoa.dylib \
        tls/libqopensslbackend.dylib \
        imageformats/libqsvg.dylib \
        iconengines/libqsvgicon.dylib \
        styles/libqmacstyle.dylib; do
        if [[ -f ${contents}/PlugIns/${wanted} ]]; then
            approve "Qt's ${wanted} is here"
        else
            complain "Qt's ${wanted} is missing"
        fi
    done

    if [[ -f ${contents}/Resources/qt.conf ]]; then
        approve "qt.conf is in Contents/Resources"
    else
        complain "Contents/Resources/qt.conf is missing: the carried Qt would not find its own plugins"
    fi

    here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    catalogue_source="${here}/../ui/src/translations.cpp"
    asked=$(sed -n 's/.*kQtCatalogues\[\] *= *{\(.*\)}.*/\1/p' "${catalogue_source}" |
        grep -o '"[^"]*"' | tr -d '"')
    if [[ ! -d ${contents}/translations ]]; then
        complain "Contents/translations is missing: every standard dialog button would be English"
    elif [[ -z ${asked} ]]; then
        complain "could not read kQtCatalogues out of ${catalogue_source}"
    else
        counted=""
        total=0
        for name in ${asked}; do
            found=$(find "${contents}/translations" -maxdepth 1 -name "${name}_*.qm" | wc -l | tr -d ' ')
            counted="${counted}${counted:+, }${name} ${found}"
            total=$((total + found))
        done
        if [[ ${total} -eq 0 ]]; then
            complain "no catalogue here is under a name the phone loads. It asks for ${asked//$'\n'/, }"
        else
            approve "Qt's own catalogues are here under a name the phone loads (${counted})"
        fi
    fi

    plist="${contents}/Info.plist"
    for key in CFBundleIdentifier CFBundleExecutable CFBundleShortVersionString \
        LSMinimumSystemVersion NSMicrophoneUsageDescription NSAudioCaptureUsageDescription \
        CFBundleIconFile; do
        if /usr/libexec/PlistBuddy -c "Print :${key}" "${plist}" > /dev/null 2>&1; then
            approve "Info.plist says ${key}"
        else
            complain "Info.plist has no ${key}"
        fi
    done

    if codesign --verify --strict --deep "${tree}" 2> /dev/null; then
        approve "the bundle and everything nested in it is signed"
        if codesign --display --entitlements - --xml "${tree}" 2> /dev/null |
            plutil -convert xml1 -o - - 2> /dev/null |
            grep -q "com.apple.security.device.audio-input"; then
            approve "it carries the audio-input entitlement, so the microphone will be heard"
        else
            complain "no audio-input entitlement: under the hardened runtime every sample would be zero"
        fi
    else
        complain "the bundle is not validly signed"
    fi

    echo
    if [[ ${failures} -eq 0 ]]; then
        echo "The bundle is one somebody else can run."
        exit 0
    fi
    echo "${failures} problem(s)." >&2
    exit 1
fi

for binary in ai-softphone ai-softphone-cli; do
    path="${tree}/bin/${binary}"

    if [[ ! -x ${path} ]]; then
        complain "${binary} is missing from bin/"
        continue
    fi

    missing=$(env -i ldd "${path}" 2>/dev/null | grep 'not found' || true)
    if [[ -n ${missing} ]]; then
        complain "${binary} cannot resolve:"$'\n'"${missing}"
    else
        approve "${binary} resolves every library it needs"
    fi
done

if [[ -n ${carries} ]]; then
    if [[ ! -r ${carries} ]]; then
        complain "cannot read the list of what may come from the machine: ${carries}"
    else
        allowed=$(grep -vE '^[[:space:]]*(#|$)' "${carries}")

        inside=$(find "${tree}" \( -type f -o -type l \) -printf '%f\n' | sort -u)

        wanted=$(while read -r object; do
            [[ ${object} == */platformthemes/libqgtk3.so ]] && continue
            file -b "${object}" 2>/dev/null | grep -q ELF || continue
            objdump -p "${object}" 2>/dev/null | awk '/NEEDED/ { print $2 }'
        done < <(find "${tree}" -type f \( -name '*.so*' -o -perm -u+x \) -print) | sort -u)

        orphans=""
        while read -r soname; do
            [[ -z ${soname} ]] && continue
            grep -qxF "${soname}" <<< "${inside}" && continue
            grep -qxF "${soname}" <<< "${allowed}" && continue
            orphans+="    ${soname}"$'\n'
        done <<< "${wanted}"

        if [[ -n ${orphans} ]]; then
            complain "needed, but neither carried nor on the list:"$'\n'"${orphans%$'\n'}"
        else
            approve "every library is carried or is named on the list, with its reason"
        fi

        pairs="$(dirname "${carries}")/travel-together"
        if [[ ! -r ${pairs} ]]; then
            complain "cannot read the groups that must travel together: ${pairs}"
        else
            split=""
            while read -r group; do
                [[ -z ${group} || ${group} == \#* ]] && continue

                here=""
                there=""
                for soname in ${group}; do
                    if grep -qxF "${soname}" <<< "${inside}"; then
                        here+=" ${soname}"
                    elif grep -qxF "${soname}" <<< "${allowed}"; then
                        grep -qxF "${soname}" <<< "${wanted}" && there+=" ${soname}"
                    fi
                done

                if [[ -n ${here} && -n ${there} ]]; then
                    split+="    carried:${here} · from the machine:${there}"$'\n'
                fi
            done < <(grep -vE '^[[:space:]]*(#|$)' "${pairs}")

            if [[ -n ${split} ]]; then
                complain "a group that shares its internals is split:"$'\n'"${split%$'\n'}"
            else
                approve "no library that shares its internals is half carried"
            fi
        fi
    fi
fi

while read -r object; do
    baked=$(objdump -x "${object}" 2>/dev/null |
        awk '/RUNPATH|RPATH/ { print $2 }' |
        grep -v '^\$ORIGIN' || true)

    if [[ -n ${baked} ]]; then
        complain "${object#"${tree}"/} carries an absolute run path: ${baked}"
    fi
done < <(find "${tree}" -type f \( -name '*.so*' -o -perm -u+x \) -print)
approve "no run path names the machine that built this"

reported=$(env -i "${tree}/bin/ai-softphone" --version 2>&1) || {
    complain "ai-softphone --version did not succeed: ${reported}"
    reported=""
}
if [[ -n ${reported} ]]; then
    approve "it starts and says: ${reported}"
fi

for plugin in "${plugins_in_files[@]}"; do
    directory="${tree}/lib/ai-softphone/plugins/${plugin}"

    if [[ ! -f ${directory}/manifest.json ]]; then
        complain "the ${plugin} plugin has no manifest.json"
    elif ! grep -q "\"id\"" "${directory}/manifest.json"; then
        complain "the ${plugin} plugin's manifest.json says nothing"
    elif [[ ! -f ${directory}/libsoftphone_plugin_${plugin}.so ]]; then
        complain "the ${plugin} plugin has a manifest and no library"
    elif [[ ! -f ${directory}/libsoftphone_plugin_${plugin}_ui.so ]]; then
        complain "the ${plugin} plugin has no interface half: it will contribute nothing to any window"
    else
        unresolved=""
        while read -r object; do
            unresolved+=$(env -i ldd "${object}" 2>/dev/null | grep 'not found' || true)
        done < <(find "${directory}" -name '*.so' -print)

        if [[ -n ${unresolved} ]]; then
            complain "the ${plugin} plugin cannot resolve:"$'\n'"${unresolved}"
        else
            approve "the ${plugin} plugin is a pair of files the phone can open"
        fi
    fi
done

for wanted in \
    "${data_root}/applications/ai-softphone.desktop" \
    "${data_root}/metainfo/com.ai-softphone.phone.metainfo.xml" \
    "${data_root}/icons/hicolor/scalable/apps/ai-softphone.svg" \
    "${data_root}/icons/hicolor/48x48/apps/ai-softphone.png" \
    "${data_root}/doc/ai-softphone/copyright"; do
    if [[ -e ${wanted} ]]; then
        approve "${wanted#"${data_root}"/}"
    else
        complain "${wanted} is missing"
    fi
done

if python3 "$(dirname "$0")/check-packaged-qml-modules.py" "${tree}"; then
    approve 'every QML module the product imports is in the tree'
else
    complain 'a QML module the product imports is not in the tree: the phone would not start'
fi

entry="${data_root}/applications/ai-softphone.desktop"
if [[ -e ${entry} ]]; then
    exec_line=$(grep '^Exec=' "${entry}" | head -1 | cut -d= -f2-)
    exec_line=${exec_line%% *}
    if [[ ${exec_line} == /* ]]; then
        landing="${root}${exec_line}"
        if [[ -L ${landing} ]]; then
            landing="${root}$(readlink "${landing}")"
        fi

        if [[ -e ${landing} ]]; then
            approve "the entry runs ${exec_line}"
        else
            complain "the entry runs ${exec_line}, which is not in this package"
        fi
    else
        approve "the entry runs ${exec_line}, to be resolved on the PATH"
    fi
fi

if [[ -e ${tree}/bin/qt.conf ]]; then
    approve "qt.conf is beside the binaries"
else
    complain "bin/qt.conf is missing: the bundled Qt is not private to this tree"
fi

for plugin in \
    platforms/libqxcb.so \
    platforms/libqwayland-generic.so \
    platforms/libqwayland-egl.so \
    wayland-shell-integration/libxdg-shell.so \
    wayland-decoration-client/libbradient.so \
    wayland-graphics-integration-client/libqt-plugin-wayland-egl.so \
    tls/libqopensslbackend.so \
    imageformats/libqsvg.so \
    platformthemes/libqgtk3.so \
    platformthemes/libqxdgdesktopportal.so \
    platforminputcontexts/libibusplatforminputcontextplugin.so \
    platforminputcontexts/libcomposeplatforminputcontextplugin.so; do
    if [[ -e ${tree}/plugins/${plugin} ]]; then
        approve "plugins/${plugin}"
    else
        complain "plugins/${plugin} is missing"
    fi
done

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
catalogue_source="${here}/../ui/src/translations.cpp"
translations="${tree}/translations"

if [[ ! -d ${translations} ]]; then
    complain "translations/ is missing: every standard dialog button would be English"
else
    asked=$(sed -n 's/.*kQtCatalogues\[\] *= *{\(.*\)}.*/\1/p' "${catalogue_source}" |
                grep -o '"[^"]*"' | tr -d '"')
    if [[ -z ${asked} ]]; then
        complain "could not read kQtCatalogues out of ${catalogue_source}"
    else
        counted=""
        total=0
        for name in ${asked}; do
            found=$(find "${translations}" -maxdepth 1 -name "${name}_*.qm" | wc -l)
            counted="${counted}${counted:+, }${name} ${found}"
            total=$((total + found))
        done
        if [[ ${total} -eq 0 ]]; then
            there=$(find "${translations}" -maxdepth 1 -name '*.qm' -printf '%f\n' |
                        head -4 | paste -sd', ' -)
            complain "no catalogue here is under a name the phone loads." \
                     "It asks for ${asked//$'\n'/, }; the tree has ${there}"
        else
            approve "Qt's own catalogues are here under a name the phone loads (${counted})"
        fi
    fi
fi

echo
if [[ ${failures} -eq 0 ]]; then
    echo "The tree is one somebody else can run."
else
    echo "${failures} problem(s)." >&2
    exit 1
fi
