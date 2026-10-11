// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace softphone::core {

class IPlaybackSource {
public:
    IPlaybackSource() = default;
    IPlaybackSource(const IPlaybackSource &) = delete;
    IPlaybackSource &operator=(const IPlaybackSource &) = delete;
    IPlaybackSource(IPlaybackSource &&) = delete;
    IPlaybackSource &operator=(IPlaybackSource &&) = delete;
    virtual ~IPlaybackSource() = default;

    [[nodiscard]] virtual unsigned sampleRate() const noexcept = 0;

    [[nodiscard]] virtual std::size_t pull(std::span<std::int16_t> out) noexcept = 0;

    [[nodiscard]] virtual bool finished() const noexcept = 0;
};

class IPlaybackDevice {
public:
    IPlaybackDevice() = default;
    IPlaybackDevice(const IPlaybackDevice &) = delete;
    IPlaybackDevice &operator=(const IPlaybackDevice &) = delete;
    IPlaybackDevice(IPlaybackDevice &&) = delete;
    IPlaybackDevice &operator=(IPlaybackDevice &&) = delete;
    virtual ~IPlaybackDevice() = default;

    virtual void playFrom(IPlaybackSource *source) = 0;

    [[nodiscard]] virtual unsigned playbackRate() const noexcept = 0;
};

}
