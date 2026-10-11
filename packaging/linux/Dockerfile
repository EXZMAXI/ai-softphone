# AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
# https://ai-softphone.com
#
# SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
# SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of AI Softphone. There is NO WARRANTY, to the extent
# permitted by law. See the file LICENSE for the full terms.

FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
        ca-certificates curl git gnupg software-properties-common \
    && add-apt-repository -y ppa:ubuntu-toolchain-r/test \
    && apt-get update && apt-get install -y --no-install-recommends \
        gcc-13 g++-13 \
    && rm -rf /var/lib/apt/lists/*

ENV CC=gcc-13
ENV CXX=g++-13

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential ninja-build pkg-config python3 python3-pip \
        libgl1-mesa-dev libxkbcommon-dev libxkbcommon-x11-dev \
        libasound2-dev libssl-dev uuid-dev libopus-dev libsecret-1-dev \
        libx11-dev libxcb1-dev libxcb-cursor-dev libxcb-xkb-dev \
        libxcb-icccm4-dev libxcb-image0-dev libxcb-keysyms1-dev \
        libxcb-randr0-dev libxcb-render-util0-dev libxcb-shape0-dev \
        libxcb-sync-dev libxcb-xfixes0-dev libxcb-xinerama0-dev \
        libxkbfile-dev libxrender-dev libfontconfig1-dev libfreetype6-dev \
        libglib2.0-dev libdbus-1-dev libpulse-dev \
        libwayland-dev \
        squashfs-tools file patchelf zstd \
        dpkg-dev fakeroot \
        libgtk-3-0 \
    && rm -rf /var/lib/apt/lists/*

RUN pip3 install --no-cache-dir "cmake==3.28.*" && cmake --version

ARG QT_VERSION=6.8.3
RUN pip3 install --no-cache-dir aqtinstall==3.1.* \
    && python3 -m aqt install-qt linux desktop "${QT_VERSION}" linux_gcc_64 \
        -m qtshadertools -O /opt/qt \
    && rm -rf /root/.cache

ENV QT_ROOT_DIR=/opt/qt/${QT_VERSION}/gcc_64
ENV CMAKE_PREFIX_PATH=/opt/qt/${QT_VERSION}/gcc_64
ENV PATH=/opt/qt/${QT_VERSION}/gcc_64/bin:${PATH}

WORKDIR /src
