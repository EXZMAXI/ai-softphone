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

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <string_view>

namespace softphone::capture {

enum class Side {
    Near,
    Far,
};

[[nodiscard]] std::string_view toString(Side side) noexcept;

using FrameSink =
    std::function<void(Side, std::span<const std::int16_t>, std::chrono::steady_clock::time_point)>;

struct CaptureRequest {

    std::string outputDevice;
    std::string inputDevice;

    unsigned sampleRate{16000};

    unsigned frameSamples{320};
};

struct CaptureStatus {
    bool running{false};

    std::string outputDevice;
    std::string inputDevice;

    std::uint64_t nearFrames{0};
    std::uint64_t farFrames{0};

    std::uint64_t dropped{0};
};

enum class CaptureError {
    None,

    NoBackend,

    NoServer,

    NoDevice,

    Refused,
};

[[nodiscard]] std::string_view toString(CaptureError error) noexcept;

class ISystemCapture {
public:
    virtual ~ISystemCapture() = default;

    ISystemCapture() = default;
    ISystemCapture(const ISystemCapture &) = delete;
    ISystemCapture &operator=(const ISystemCapture &) = delete;
    ISystemCapture(ISystemCapture &&) = delete;
    ISystemCapture &operator=(ISystemCapture &&) = delete;

    [[nodiscard]] virtual CaptureError start(const CaptureRequest &request, FrameSink sink) = 0;

    virtual void stop() = 0;

    [[nodiscard]] virtual CaptureStatus status() const = 0;

    enum class Yields {

        Nothing,

        WhatTheMachinePlays,

        Everything,
    };

    [[nodiscard]] virtual Yields mustYieldTheSoundCard() const = 0;

    virtual void holdOffWhatTheMachinePlays(bool ) {}
};

[[nodiscard]] std::unique_ptr<ISystemCapture> makeSystemCapture();

}
