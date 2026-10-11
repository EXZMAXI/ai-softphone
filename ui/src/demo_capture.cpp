// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "demo_capture.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace softphone::ui {
namespace {

constexpr double kLoudest = 0.34;

constexpr double kSyllablesASecond = 4.5;

constexpr double kNearVoice = 190.0;
constexpr double kFarVoice = 115.0;

}

DemoCapture::DemoCapture(QObject *parent) : QObject{parent}
{
    clock_.setInterval(kTick);
    clock_.setTimerType(Qt::PreciseTimer);
    QObject::connect(&clock_, &QTimer::timeout, this, &DemoCapture::speak);
}

void DemoCapture::capturing(const QString &what, int secondsIn)
{
    what_ = what;
    running_ = true;
    began_ = std::chrono::milliseconds{static_cast<std::int64_t>(secondsIn) * 1000};
    spoken_ = std::chrono::milliseconds{0};
    turnEnds_ = std::chrono::milliseconds{0};
    silent_ = true;
    nearSide_ = true;
    word_ = std::chrono::milliseconds{0};
    between_ = false;
    loud_ = 1.0;
    syllable_ = 0.0;
    phase_ = 0.0;
    noise_.seed(20260909);
    signals_.clear();
    clock_.start();
    Q_EMIT changed();
}

void DemoCapture::quiet()
{
    if (!running_) {
        return;
    }
    running_ = false;
    what_.clear();
    clock_.stop();
    signals_.clear();
    Q_EMIT changed();
}

std::chrono::milliseconds DemoCapture::elapsed() const noexcept
{
    return running_ ? began_ + spoken_ : std::chrono::milliseconds{0};
}

void DemoCapture::watchSignals(const std::string &name)
{

    const bool wanted = !name.empty();
    if (wanted == watching_) {
        return;
    }
    watching_ = wanted;
    if (!watching_) {
        signals_.clear();
    }
}

core::CallSignals &DemoCapture::feed() noexcept
{
    return signals_;
}

bool DemoCapture::bothVoicesInTheTrail(int seconds) const
{
    if (nearSpokeAt_ < std::chrono::milliseconds{0} || farSpokeAt_ < std::chrono::milliseconds{0}) {
        return false;
    }
    const auto trail = std::chrono::milliseconds{seconds * 1000};
    return spoken_ - nearSpokeAt_ <= trail && spoken_ - farSpokeAt_ <= trail;
}

void DemoCapture::speak()
{
    spoken_ += kTick;

    if (spoken_ >= turnEnds_) {
        if (silent_) {
            silent_ = false;
            nearSide_ = !nearSide_;
            turnEnds_ = spoken_ + std::chrono::milliseconds{
                                      std::uniform_int_distribution<int>{1800, 5200}(noise_)};
            word_ = std::chrono::milliseconds{0};
        } else {
            silent_ = true;
            turnEnds_ = spoken_ + std::chrono::milliseconds{
                                      std::uniform_int_distribution<int>{220, 700}(noise_)};
        }
    }

    if (!silent_ && spoken_ >= word_) {
        between_ = !between_;
        if (between_) {
            word_ = spoken_ +
                    std::chrono::milliseconds{std::uniform_int_distribution<int>{60, 220}(noise_)};
        } else {
            word_ = spoken_ +
                    std::chrono::milliseconds{std::uniform_int_distribution<int>{140, 460}(noise_)};
            loud_ = std::uniform_real_distribution<double>{0.06, 1.0}(noise_);
        }
    }

    if (!watching_) {

        return;
    }

    if (silent_ || between_) {

        return;
    }

    const double voice = nearSide_ ? kNearVoice : kFarVoice;
    const double step = 2.0 * std::numbers::pi * voice / kRate;
    const double syllableStep = 2.0 * std::numbers::pi * kSyllablesASecond / kRate;

    for (auto &sample : block_) {
        syllable_ += syllableStep;
        phase_ += step;

        const double open = std::pow(std::abs(std::sin(syllable_)), 0.6);
        const double loud = kLoudest * loud_ * (0.25 + 0.75 * open);

        const double wave =
            0.62 * std::sin(phase_) + 0.26 * std::sin(2.0 * phase_) + 0.12 * std::sin(3.0 * phase_);

        sample = static_cast<std::int16_t>(std::clamp(wave * loud * 32767.0, -32767.0, 32767.0));
    }

    (nearSide_ ? nearSpokeAt_ : farSpokeAt_) = spoken_;

    auto &channel = nearSide_ ? signals_.local : signals_.remote;
    channel.ring().write(block_.data(), block_.size());
}

}
