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

#include <softphone/core/level_meter.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace softphone::core {

class SignalRing {
public:

    static constexpr std::size_t kCapacity = 8192;

    void write(const std::int16_t *samples, std::size_t count) noexcept;

    [[nodiscard]] std::size_t read(std::int16_t *out, std::size_t capacity) noexcept;

    [[nodiscard]] std::size_t waiting() const noexcept;

    void clear() noexcept;

private:
    std::array<std::int16_t, kCapacity> buffer_{};

    std::atomic<std::uint64_t> written_{0};

    std::atomic<std::uint64_t> taken_{0};
};

[[nodiscard]] double signalLevel(std::span<const std::int16_t> samples) noexcept;

inline constexpr unsigned kSignalRate = 20;

class SignalTrail {
public:
    static constexpr std::size_t kLength = 140;

    void push(double level) noexcept;

    [[nodiscard]] std::span<const double> values() const noexcept
    {
        return {values_.data(), values_.size()};
    }

    void clear() noexcept;

private:
    std::array<double, kLength> values_{};
};

inline constexpr std::size_t kSpectrumBands = 22;

inline constexpr std::size_t kSpectrumWindow = 512;

class Spectrum {
public:
    explicit Spectrum(unsigned sampleRate) noexcept;

    void observe(std::span<const std::int16_t> samples) noexcept;

    void refresh() noexcept;

    [[nodiscard]] std::span<const float> bands() const noexcept
    {
        return {bands_.data(), bands_.size()};
    }

    void clear() noexcept;

private:
    unsigned sampleRate_;

    std::array<float, kSpectrumWindow> window_{};

    std::size_t cursor_{0};

    std::array<float, kSpectrumWindow> real_{};
    std::array<float, kSpectrumWindow> imaginary_{};

    std::array<float, kSpectrumBands> bands_{};
};

class SignalChannel {
public:
    explicit SignalChannel(unsigned sampleRate) noexcept;

    [[nodiscard]] SignalRing &ring() noexcept
    {
        return ring_;
    }

    std::size_t poll(std::chrono::milliseconds elapsed, bool wantSpectrum) noexcept;

    [[nodiscard]] double level() const noexcept
    {
        return meter_.reading();
    }

    [[nodiscard]] double peak() const noexcept
    {
        return peak_;
    }

    [[nodiscard]] const SignalTrail &trail() const noexcept
    {
        return trail_;
    }

    [[nodiscard]] std::span<const float> bands() const noexcept
    {
        return spectrum_.bands();
    }

    void clear() noexcept;

private:

    [[nodiscard]] std::size_t slice() const noexcept
    {
        return sampleRate_ / kSignalRate;
    }

    unsigned sampleRate_;
    SignalRing ring_;
    LevelMeter meter_;
    SignalTrail trail_;
    Spectrum spectrum_;

    std::vector<std::int16_t> pending_;

    std::chrono::milliseconds carried_{0};

    double peak_{0.0};

    unsigned peakHeld_{0};

    std::vector<std::int16_t> scratch_;
};

struct CallSignals {
    explicit CallSignals(unsigned sampleRate) noexcept : local(sampleRate), remote(sampleRate) {}

    SignalChannel local;

    SignalChannel remote;

    std::size_t poll(std::chrono::milliseconds elapsed, bool wantSpectrum) noexcept
    {
        const std::size_t here = local.poll(elapsed, wantSpectrum);
        const std::size_t there = remote.poll(elapsed, wantSpectrum);
        return std::max(here, there);
    }

    void clear() noexcept
    {
        local.clear();
        remote.clear();
    }
};

class ISignalTaps {
public:
    ISignalTaps() = default;
    ISignalTaps(const ISignalTaps &) = delete;
    ISignalTaps &operator=(const ISignalTaps &) = delete;
    ISignalTaps(ISignalTaps &&) = delete;
    ISignalTaps &operator=(ISignalTaps &&) = delete;
    virtual ~ISignalTaps() = default;

    virtual void watchSignals(const std::string &callId) = 0;

    [[nodiscard]] virtual CallSignals &feed() noexcept = 0;
};

}
