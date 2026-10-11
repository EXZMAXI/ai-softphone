// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "sound_view_model.hpp"

#include "signal_geometry.hpp"

#include <softphone/core/logging.hpp>

#include <QString>

#include <algorithm>
#include <array>
#include <span>
#include <utility>

namespace softphone::ui {

namespace {

constexpr int kStripHeight = 64;
constexpr int kHaloHeight = 132;

}

SoundViewModel::Drawing toDrawing(core::Visualisation drawing) noexcept
{
    switch (drawing) {
    case core::Visualisation::Off:
        return SoundViewModel::Drawing::Nothing;
    case core::Visualisation::Levels:
        return SoundViewModel::Drawing::Levels;
    case core::Visualisation::Histogram:
        return SoundViewModel::Drawing::Histogram;
    case core::Visualisation::Wave:
        return SoundViewModel::Drawing::Wave;
    case core::Visualisation::Spectrum:
        return SoundViewModel::Drawing::Spectrum;
    case core::Visualisation::Aurora:
        return SoundViewModel::Drawing::Aurora;
    case core::Visualisation::Halo:
        return SoundViewModel::Drawing::Halo;
    case core::Visualisation::Uprights:
        return SoundViewModel::Drawing::Uprights;
    }
    return SoundViewModel::Drawing::Nothing;
}

core::Visualisation toVisualisation(SoundViewModel::Drawing drawing) noexcept
{
    switch (drawing) {
    case SoundViewModel::Drawing::Nothing:
        return core::Visualisation::Off;
    case SoundViewModel::Drawing::Levels:
        return core::Visualisation::Levels;
    case SoundViewModel::Drawing::Histogram:
        return core::Visualisation::Histogram;
    case SoundViewModel::Drawing::Wave:
        return core::Visualisation::Wave;
    case SoundViewModel::Drawing::Spectrum:
        return core::Visualisation::Spectrum;
    case SoundViewModel::Drawing::Aurora:
        return core::Visualisation::Aurora;
    case SoundViewModel::Drawing::Halo:
        return core::Visualisation::Halo;
    case SoundViewModel::Drawing::Uprights:
        return core::Visualisation::Uprights;
    }
    return core::Visualisation::Off;
}

SoundViewModel::SoundViewModel(core::ISignalTaps *taps, Dispatcher dispatch, QObject *parent)
    : QObject(parent), taps_{taps}, dispatch_{std::move(dispatch)}
{
    frames_.setInterval(static_cast<int>(kFrame.count()));
    frames_.setTimerType(Qt::PreciseTimer);
    connect(&frames_, &QTimer::timeout, this, [this]() {
        if (taps_ == nullptr) {
            return;
        }

        const auto elapsed = std::chrono::milliseconds{std::max<qint64>(0, since_.restart())};
        const std::size_t readings =
            taps_->feed().poll(elapsed, core::needsSpectrum(toVisualisation(drawing_)));
        if (readings > 0) {
            sinceReading_.restart();
            layOutAboveTheFloor();
        }
        sayWhatItReads();
        emit advanced();
    });
}

void SoundViewModel::sayWhatItReads()
{
    if (taps_ == nullptr || callId_.isEmpty()) {
        return;
    }
    if (sinceSaid_.isValid() && sinceSaid_.elapsed() < 1000) {
        return;
    }
    sinceSaid_.restart();

    const auto decibels = [](double level) { return level <= 0.0 ? -60.0 : (level - 1.0) * 60.0; };
    const auto band = [](std::span<const double> values) {

        double least = 1.0;
        double most = 0.0;
        const auto from = values.size() > core::kSignalRate ? values.size() - core::kSignalRate : 0;
        for (auto at = from; at < values.size(); ++at) {
            least = std::min(least, values[at]);
            most = std::max(most, values[at]);
        }
        return std::pair{least, most};
    };
    const auto two = [](double value) { return QString::number(value, 'f', 2).toStdString(); };

    auto &feed = taps_->feed();
    const auto [nearLeast, nearMost] = band(feed.local.trail().values());
    const auto [farLeast, farMost] = band(feed.remote.trail().values());

    const auto said = callId_.toStdString();
    const std::array<core::LogAttribute, 1> about{core::LogAttribute{"call", said}};

    core::Logger::instance().log(
        core::LogLevel::Debug, "ui",
        "signal: near " + two(feed.local.level()) + " (" + two(decibels(feed.local.level())) +
            " dBFS) peak " + two(feed.local.peak()) + " second " + two(nearLeast) + ".." +
            two(nearMost) + "; far " + two(feed.remote.level()) + " (" +
            two(decibels(feed.remote.level())) + " dBFS) peak " + two(feed.remote.peak()) +
            " second " + two(farLeast) + ".." + two(farMost),
        about);
}

bool SoundViewModel::showing() const noexcept
{
    return showing_;
}

int SoundViewModel::height() const noexcept
{
    switch (drawing_) {
    case Drawing::Nothing:
    case Drawing::Aurora:
    case Drawing::Uprights:
    case Drawing::Levels:
        return 0;
    case Drawing::Halo:
        return kHaloHeight;
    case Drawing::Histogram:
    case Drawing::Wave:
    case Drawing::Spectrum:
        return kStripHeight;
    }
    return 0;
}

bool SoundViewModel::flanking() const noexcept
{
    return drawing_ == Drawing::Uprights;
}

bool SoundViewModel::behind() const noexcept
{
    return drawing_ == Drawing::Aurora;
}

void SoundViewModel::follow(const QString &callId)
{
    if (callId_ == callId) {
        return;
    }
    callId_ = callId;
    retarget();
    reconsider();
}

void SoundViewModel::chooseDrawing(core::Visualisation drawing)
{
    const Drawing wanted = toDrawing(drawing);
    if (wanted == drawing_) {
        return;
    }

    drawing_ = wanted;
    emit drawingChanged();
    retarget();
    reconsider();
}

void SoundViewModel::watchWindow(bool onScreen)
{
    if (onScreen_ == onScreen) {
        return;
    }
    onScreen_ = onScreen;

    if (showing_) {
        if (onScreen_) {
            since_.restart();
            sinceReading_.restart();
            frames_.start();
        } else {
            frames_.stop();
        }
    }
}

void SoundViewModel::retarget()
{
    if (!dispatch_) {
        return;
    }

    const std::string wanted = drawing_ == Drawing::Nothing ? std::string{} : callId_.toStdString();

    dispatch_([taps = taps_, wanted]() {
        if (taps != nullptr) {
            taps->watchSignals(wanted);
        }
    });
}

void SoundViewModel::reconsider()
{
    const bool wanted = drawing_ != Drawing::Nothing && !callId_.isEmpty();
    if (wanted == showing_) {
        return;
    }

    showing_ = wanted;
    if (showing_ && onScreen_) {
        since_.start();
        sinceReading_.start();
        frames_.start();
    } else {
        frames_.stop();
    }
    emit showingChanged();
}

void SoundViewModel::layOutAboveTheFloor()
{
    if (taps_ == nullptr) {
        return;
    }
    const auto &feed = taps_->feed();

    const auto lay = [](const core::SignalChannel &channel, double &floor,
                        std::array<double, core::SignalTrail::kLength> &out) {
        const auto trail = channel.trail().values();
        floor = noiseFloor(trail);
        const auto count = std::min(trail.size(), out.size());
        for (std::size_t at = 0; at < count; ++at) {
            out[at] = aboveTheFloor(trail[at], floor);
        }
    };

    lay(feed.local, localFloor_, localTrail_);
    lay(feed.remote, remoteFloor_, remoteTrail_);
}

double SoundViewModel::level(bool local) const noexcept
{
    if (taps_ == nullptr || !showing_) {
        return 0.0;
    }
    const auto &feed = taps_->feed();
    return aboveTheFloor(local ? feed.local.level() : feed.remote.level(),
                         local ? localFloor_ : remoteFloor_);
}

double SoundViewModel::peak(bool local) const noexcept
{
    if (taps_ == nullptr || !showing_) {
        return 0.0;
    }
    const auto &feed = taps_->feed();
    return aboveTheFloor(local ? feed.local.peak() : feed.remote.peak(),
                         local ? localFloor_ : remoteFloor_);
}

std::span<const double> SoundViewModel::trail(bool local) const noexcept
{
    if (taps_ == nullptr || !showing_) {
        return {};
    }

    const auto &feed = taps_->feed();
    const auto held = (local ? feed.local : feed.remote).trail().values().size();
    const auto &out = local ? localTrail_ : remoteTrail_;
    return {out.data(), std::min(held, out.size())};
}

std::span<const float> SoundViewModel::bands(bool local) const noexcept
{
    if (taps_ == nullptr || !showing_ || !core::needsSpectrum(toVisualisation(drawing_))) {
        return {};
    }
    const auto &feed = taps_->feed();
    return local ? feed.local.bands() : feed.remote.bands();
}

double SoundViewModel::slide() const noexcept
{
    if (!showing_ || !sinceReading_.isValid()) {
        return 1.0;
    }
    const double since = static_cast<double>(sinceReading_.elapsed());
    return std::clamp(since / static_cast<double>(core::kLevelMeterInterval.count()), 0.0, 1.0);
}

}
