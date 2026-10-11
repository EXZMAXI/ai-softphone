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
image="ai-softphone-build:22.04"

if ! command -v docker > /dev/null; then
    echo "docker is not installed, and this needs a container to build in." >&2
    echo "The build itself is scripts/make-package.sh; what the container buys" >&2
    echo "is the older base — see packaging/linux/Dockerfile." >&2
    exit 1
fi

echo "== the base"
docker build -t "${image}" -f "${root}/packaging/linux/Dockerfile" "${root}/packaging/linux"

docker volume create ai-softphone-deps > /dev/null

docker volume create ai-softphone-package > /dev/null

docker volume rm ai-softphone-package > /dev/null 2>&1 || true
docker volume create ai-softphone-package > /dev/null

if [[ ${1:-} == "--shell" ]]; then
    exec docker run --rm -it \
        -v "${root}:/src" -v ai-softphone-deps:/root/.cache/softphone-ai-deps \
        -v ai-softphone-package:/src/build/package \
        "${image}" bash
fi

docker run --rm \
    -v "${root}:/src" \
    -v ai-softphone-deps:/root/.cache/softphone-ai-deps \
    -v ai-softphone-package:/src/build/package \
    "${image}" bash -euo pipefail -c '
        echo "== the dependencies"
        ./scripts/build-pjproject.sh > /tmp/pjproject.log 2>&1 || {
            tail -30 /tmp/pjproject.log >&2
            exit 1
        }

        echo "== the phone"

        cached=$(sed -n "s/^CMAKE_CXX_COMPILER:[A-Z]*=//p" build/package/CMakeCache.txt 2>/dev/null || true)
        if [ -n "${cached}" ] && [ "${cached}" != "$(command -v "${CXX}")" ]; then
            echo "   the cached compiler is ${cached}, not ${CXX} — starting again"
            find build/package -mindepth 1 -delete
        fi

        cmake --preset package -U "SOFTPHONE_RELEASE_*"
        cmake --build --preset package

        echo "== the packages"
        ./scripts/make-package.sh
    '

docker run --rm \
    -v "${root}:/src" \
    "${image}" chown -R "$(id -u):$(id -g)" /src/dist/linux

echo
echo "== what came out, and what it will run on"
"${root}/scripts/check-linux-floor.sh" || true
