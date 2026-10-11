// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/media/time_stretch.hpp>

#include <algorithm>
#include <cmath>
#include <numbers>

namespace softphone::media {
namespace {

[[nodiscard]] std::size_t millisecondsOf(unsigned rate, unsigned ms)
{
    return std::max<std::size_t>(2, static_cast<std::size_t>(rate) * ms / 1000);
}

}

TimeStretch::TimeStretch(unsigned sampleRate) : rate_{sampleRate == 0 ? 16000u : sampleRate}
{
    window_ = millisecondsOf(rate_, 30);

    window_ -= window_ % 2;
    hop_ = window_ / 2;
    search_ = millisecondsOf(rate_, 10);

    shape_.resize(window_);
    for (std::size_t at = 0; at < window_; ++at) {

        shape_[at] = static_cast<float>(
            0.5 - 0.5 * std::cos(2.0 * std::numbers::pi * static_cast<double>(at) /
                                 static_cast<double>(window_)));
    }

    wanted_.resize(hop_);
    reset();
}

void TimeStretch::setSpeed(double speed)
{
    speed_ = std::clamp(speed, 0.25, 4.0);
}

void TimeStretch::reset()
{
    source_.clear();
    made_.clear();
    taken_ = 0;
    open_ = 0;
    settled_ = 0;
    nominal_ = 0;
    at_ = 0;
    first_ = true;
    ending_ = false;
    std::fill(wanted_.begin(), wanted_.end(), std::int16_t{0});
}

void TimeStretch::write(std::span<const std::int16_t> samples)
{
    source_.insert(source_.end(), samples.begin(), samples.end());
    grind();
}

void TimeStretch::finish()
{
    ending_ = true;
    grind();

    settled_ = made_.size();
}

void TimeStretch::layDown(std::size_t from)
{

    const std::size_t needed = open_ + window_;
    if (made_.size() < needed) {
        made_.resize(needed, 0.0F);
    }

    for (std::size_t sample = 0; sample < window_; ++sample) {
        made_[open_ + sample] += shape_[sample] * static_cast<float>(source_[from + sample]);
    }

    const std::size_t after = from + hop_;
    for (std::size_t sample = 0; sample < hop_; ++sample) {
        const std::size_t taken = after + sample;
        wanted_[sample] = taken < source_.size() ? source_[taken] : std::int16_t{0};
    }

    open_ += hop_;
    settled_ = open_;
}

void TimeStretch::grind()
{

    if (std::abs(speed_ - 1.0) < 0.001) {
        if (source_.empty()) {
            return;
        }
        made_.reserve(made_.size() + source_.size());
        for (const std::int16_t sample : source_) {
            made_.push_back(static_cast<float>(sample));
        }
        open_ = made_.size();
        settled_ = made_.size();
        source_.clear();
        nominal_ = 0;
        at_ = 0;
        first_ = true;
        return;
    }

    const auto step = static_cast<std::size_t>(std::lround(static_cast<double>(hop_) * speed_));

    for (;;) {

        const std::size_t nominal = nominal_;
        const std::size_t from = nominal > search_ ? nominal - search_ : 0;
        const std::size_t to = nominal + search_;

        if (to + window_ > source_.size()) {
            if (!ending_) {
                return;
            }
            if (nominal + window_ > source_.size()) {
                return;
            }
        }

        std::size_t chosen = nominal;
        if (!first_) {

            double best = -1e30;
            for (std::size_t candidate = from; candidate <= to; ++candidate) {
                if (candidate + window_ > source_.size()) {
                    break;
                }

                double score = 0.0;
                double energy = 0.0;
                for (std::size_t sample = 0; sample < hop_; ++sample) {
                    const double here = source_[candidate + sample];
                    score += here * wanted_[sample];
                    energy += here * here;
                }

                const double fit = energy > 1.0 ? score / std::sqrt(energy) : 0.0;
                if (fit > best) {
                    best = fit;
                    chosen = candidate;
                }
            }
        }

        if (chosen + window_ > source_.size()) {
            return;
        }

        layDown(chosen);
        at_ = chosen;
        nominal_ += step;
        first_ = false;

        const std::size_t earliest = std::min(at_, nominal_);
        const std::size_t keepFrom = earliest > search_ ? earliest - search_ : 0;
        if (keepFrom > window_ * 4) {
            source_.erase(source_.begin(), source_.begin() + static_cast<std::ptrdiff_t>(keepFrom));
            at_ -= keepFrom;
            nominal_ -= keepFrom;
        }
    }
}

std::size_t TimeStretch::read(std::span<std::int16_t> out)
{

    const std::size_t available = settled_ > taken_ ? settled_ - taken_ : 0;
    const std::size_t giving = std::min(available, out.size());

    for (std::size_t sample = 0; sample < giving; ++sample) {
        const float value = made_[taken_ + sample];
        out[sample] = static_cast<std::int16_t>(std::clamp(value, -32768.0F, 32767.0F));
    }

    taken_ += giving;

    if (taken_ > hop_ * 8) {
        made_.erase(made_.begin(), made_.begin() + static_cast<std::ptrdiff_t>(taken_));
        open_ -= taken_;
        settled_ -= taken_;
        taken_ = 0;
    }

    return giving;
}

}
