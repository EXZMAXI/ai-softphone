// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/audio_signal.hpp>

#include <algorithm>
#include <cmath>
#include <numbers>

namespace softphone::core {

namespace {

constexpr double kFloorDecibels = -60.0;

constexpr float kSpectrumFloorDecibels = -70.0F;

constexpr double kFullScale = 32768.0;

constexpr float kLowestBand = 120.0F;
constexpr float kHighestBand = 7800.0F;

}

void SignalRing::write(const std::int16_t *samples, std::size_t count) noexcept
{
    if (samples == nullptr || count == 0) {
        return;
    }

    if (count > kCapacity) {
        samples += count - kCapacity;
        count = kCapacity;
    }

    const std::uint64_t start = written_.load(std::memory_order_relaxed);
    for (std::size_t index = 0; index < count; ++index) {
        buffer_[(start + index) % kCapacity] = samples[index];
    }

    written_.store(start + count, std::memory_order_release);
}

std::size_t SignalRing::read(std::int16_t *out, std::size_t capacity) noexcept
{
    if (out == nullptr || capacity == 0) {
        return 0;
    }

    const std::uint64_t written = written_.load(std::memory_order_acquire);
    std::uint64_t taken = taken_.load(std::memory_order_relaxed);

    if (written - taken > kCapacity) {
        taken = written - kCapacity;
    }

    std::uint64_t available = written - taken;
    if (available > capacity) {
        taken = written - capacity;
        available = capacity;
    }

    for (std::uint64_t index = 0; index < available; ++index) {
        out[index] = buffer_[(taken + index) % kCapacity];
    }

    taken_.store(taken + available, std::memory_order_relaxed);
    return static_cast<std::size_t>(available);
}

std::size_t SignalRing::waiting() const noexcept
{
    const std::uint64_t written = written_.load(std::memory_order_acquire);
    const std::uint64_t taken = taken_.load(std::memory_order_relaxed);
    return static_cast<std::size_t>(std::min<std::uint64_t>(written - taken, kCapacity));
}

void SignalRing::clear() noexcept
{
    taken_.store(written_.load(std::memory_order_acquire), std::memory_order_relaxed);
}

double signalLevel(std::span<const std::int16_t> samples) noexcept
{
    if (samples.empty()) {
        return 0.0;
    }

    double sum = 0.0;
    for (const std::int16_t sample : samples) {
        const double value = static_cast<double>(sample) / kFullScale;
        sum += value * value;
    }

    const double mean = std::sqrt(sum / static_cast<double>(samples.size()));
    if (mean <= 0.0) {
        return 0.0;
    }

    const double decibels = 20.0 * std::log10(mean);
    if (decibels <= kFloorDecibels) {
        return 0.0;
    }

    return std::clamp(1.0 - decibels / kFloorDecibels, 0.0, 1.0);
}

void SignalTrail::push(double level) noexcept
{

    std::rotate(values_.begin(), values_.begin() + 1, values_.end());
    values_.back() = std::clamp(level, 0.0, 1.0);
}

void SignalTrail::clear() noexcept
{
    values_.fill(0.0);
}

namespace {

void transform(std::span<float> real, std::span<float> imaginary) noexcept
{
    const std::size_t count = real.size();

    for (std::size_t i = 1, j = 0; i < count; ++i) {
        std::size_t bit = count >> 1U;
        for (; (j & bit) != 0; bit >>= 1U) {
            j ^= bit;
        }
        j ^= bit;
        if (i < j) {
            std::swap(real[i], real[j]);
            std::swap(imaginary[i], imaginary[j]);
        }
    }

    for (std::size_t length = 2; length <= count; length <<= 1U) {
        const float angle = -2.0F * std::numbers::pi_v<float> / static_cast<float>(length);
        const float stepReal = std::cos(angle);
        const float stepImaginary = std::sin(angle);
        for (std::size_t start = 0; start < count; start += length) {
            float turnReal = 1.0F;
            float turnImaginary = 0.0F;
            for (std::size_t offset = 0; offset < length / 2; ++offset) {
                const std::size_t here = start + offset;
                const std::size_t there = here + length / 2;

                const float productReal = real[there] * turnReal - imaginary[there] * turnImaginary;
                const float productImaginary =
                    real[there] * turnImaginary + imaginary[there] * turnReal;

                real[there] = real[here] - productReal;
                imaginary[there] = imaginary[here] - productImaginary;
                real[here] += productReal;
                imaginary[here] += productImaginary;

                const float nextReal = turnReal * stepReal - turnImaginary * stepImaginary;
                turnImaginary = turnReal * stepImaginary + turnImaginary * stepReal;
                turnReal = nextReal;
            }
        }
    }
}

}

Spectrum::Spectrum(unsigned sampleRate) noexcept
    : sampleRate_(sampleRate == 0 ? 16000U : sampleRate)
{}

void Spectrum::observe(std::span<const std::int16_t> samples) noexcept
{
    for (const std::int16_t sample : samples) {
        window_[cursor_] = static_cast<float>(sample) / static_cast<float>(kFullScale);
        cursor_ = (cursor_ + 1) % kSpectrumWindow;
    }
}

void Spectrum::refresh() noexcept
{

    for (std::size_t index = 0; index < kSpectrumWindow; ++index) {
        const std::size_t source = (cursor_ + index) % kSpectrumWindow;
        const float phase = 2.0F * std::numbers::pi_v<float> * static_cast<float>(index) /
                            static_cast<float>(kSpectrumWindow - 1);
        const float shape = 0.5F * (1.0F - std::cos(phase));
        real_[index] = window_[source] * shape;
        imaginary_[index] = 0.0F;
    }

    transform(real_, imaginary_);

    const float nyquist = static_cast<float>(sampleRate_) / 2.0F;
    const float top = std::min(kHighestBand, nyquist);
    const float binWidth = static_cast<float>(sampleRate_) / static_cast<float>(kSpectrumWindow);

    for (std::size_t band = 0; band < kSpectrumBands; ++band) {

        const float fraction = static_cast<float>(band) / static_cast<float>(kSpectrumBands);
        const float next = static_cast<float>(band + 1) / static_cast<float>(kSpectrumBands);
        const float low = kLowestBand * std::pow(top / kLowestBand, fraction);
        const float high = kLowestBand * std::pow(top / kLowestBand, next);

        auto firstBin = static_cast<std::size_t>(std::floor(low / binWidth));
        auto lastBin = static_cast<std::size_t>(std::ceil(high / binWidth));
        firstBin = std::max<std::size_t>(firstBin, 1);
        lastBin = std::min<std::size_t>(std::max(lastBin, firstBin + 1), kSpectrumWindow / 2);

        float loudest = 0.0F;
        for (std::size_t bin = firstBin; bin < lastBin; ++bin) {
            const float magnitude =
                std::sqrt(real_[bin] * real_[bin] + imaginary_[bin] * imaginary_[bin]);
            loudest = std::max(loudest, magnitude);
        }

        const float scaled = loudest * 4.0F / static_cast<float>(kSpectrumWindow);
        float value = 0.0F;
        if (scaled > 0.0F) {
            const float decibels = 20.0F * std::log10(scaled);
            value = std::clamp(1.0F - decibels / kSpectrumFloorDecibels, 0.0F, 1.0F);
        }

        bands_[band] = std::max(value, bands_[band] * 0.72F);
    }
}

void Spectrum::clear() noexcept
{
    window_.fill(0.0F);
    bands_.fill(0.0F);
    cursor_ = 0;
}

SignalChannel::SignalChannel(unsigned sampleRate) noexcept
    : sampleRate_(sampleRate == 0 ? 16000U : sampleRate),
      spectrum_(sampleRate == 0 ? 16000U : sampleRate)
{
    pending_.reserve(slice() * 2);
    scratch_.resize(SignalRing::kCapacity);
}

std::size_t SignalChannel::poll(std::chrono::milliseconds elapsed, bool wantSpectrum) noexcept
{
    const std::size_t taken = ring_.read(scratch_.data(), scratch_.size());
    if (taken > 0) {
        pending_.insert(pending_.end(), scratch_.begin(),
                        scratch_.begin() + static_cast<std::ptrdiff_t>(taken));
    }

    const std::size_t step = slice();
    if (step == 0) {
        return 0;
    }

    carried_ += elapsed;
    const auto owed = std::min(carried_, kLevelMeterInterval * 10);
    carried_ = owed;

    std::size_t readings = 0;
    while (carried_ >= kLevelMeterInterval) {
        carried_ -= kLevelMeterInterval;
        ++readings;

        const std::size_t have = std::min(pending_.size(), step);
        const std::span<const std::int16_t> block{pending_.data(), have};

        const double level = have == 0 ? 0.0 : signalLevel(block);
        meter_.observe(static_cast<unsigned>(level * kLoudestLevel));
        trail_.push(level);

        constexpr unsigned kPeakHoldReadings = 18;
        constexpr double kPeakFallPerReading = 0.02;
        if (meter_.reading() >= peak_) {
            peak_ = meter_.reading();
            peakHeld_ = 0;
        } else if (++peakHeld_ > kPeakHoldReadings) {
            peak_ = std::max(meter_.reading(), peak_ - kPeakFallPerReading);
        }
        if (wantSpectrum) {
            spectrum_.observe(block);
            spectrum_.refresh();
        }

        if (have > 0) {
            pending_.erase(pending_.begin(), pending_.begin() + static_cast<std::ptrdiff_t>(have));
        }
    }

    if (pending_.size() > step * 2) {
        pending_.erase(pending_.begin(), pending_.end() - static_cast<std::ptrdiff_t>(step * 2));
    }

    return readings;
}

void SignalChannel::clear() noexcept
{
    ring_.clear();
    pending_.clear();
    meter_.reset();
    peak_ = 0.0;
    peakHeld_ = 0;
    carried_ = std::chrono::milliseconds{0};
    trail_.clear();
    spectrum_.clear();
}

}
