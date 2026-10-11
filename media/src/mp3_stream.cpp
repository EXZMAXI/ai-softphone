// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/media/mp3_stream.hpp>

#include <softphone/media/mp3.hpp>

#include <algorithm>
#include <cmath>

namespace softphone::media {
namespace {

constexpr unsigned kDepthSeconds = 2;

constexpr std::size_t kFrameSamples = 320;

}

struct Mp3Stream::Reader {
    Mp3Decoder decoder;
};

Mp3Stream::Mp3Stream(std::filesystem::path file, unsigned rate)
    : file_{std::move(file)}, rate_{rate == 0 ? 16000u : rate}, reader_{std::make_unique<Reader>()},
      ring_{static_cast<std::size_t>(rate_) * kDepthSeconds,
            (static_cast<std::size_t>(rate_) * kDepthSeconds) / kFrameSamples + 8},
      stretch_{rate_ == 0 ? 16000u : rate_}
{
    frame_.resize(kFrameSamples);
}

Mp3Stream::~Mp3Stream()
{
    stop();
}

bool Mp3Stream::start()
{
    if (thread_.joinable()) {
        return true;
    }

    if (!succeeded(reader_->decoder.open(file_))) {
        return false;
    }

    const auto &info = reader_->decoder.info();
    sourceRate_ = info.sampleRate;
    channels_ = info.channels;
    if (sourceRate_ == 0 || channels_ == 0) {
        reader_->decoder.close();
        return false;
    }

    durationMs_ = info.frames * 1000 / sourceRate_;

    cursor_ = 0.0;
    played_.store(0, std::memory_order_relaxed);
    finished_.store(false, std::memory_order_relaxed);
    stopping_ = false;
    ring_.restart();
    pending_.clear();
    stretch_.reset();
    stretch_.setSpeed(speed_.load(std::memory_order_acquire));

    thread_ = std::thread{[this]() { run(); }};
    return true;
}

void Mp3Stream::stop()
{
    {
        const std::lock_guard<std::mutex> held{mutex_};
        stopping_ = true;
    }
    wake_.notify_all();

    if (thread_.joinable()) {
        thread_.join();
    }

    reader_->decoder.close();
}

std::uint64_t Mp3Stream::positionMs() const noexcept
{
    const auto played = played_.load(std::memory_order_acquire);
    return rate_ == 0 ? 0 : played * 1000 / rate_;
}

void Mp3Stream::seek(std::uint64_t positionMs)
{

    const std::lock_guard<std::mutex> held{mutex_};

    const auto frame = std::min(positionMs, durationMs_) * sourceRate_ / 1000;
    static_cast<void>(reader_->decoder.seek(frame));

    cursor_ = 0.0;
    pending_.clear();
    held_ = 0;
    ring_.restart();
    stretch_.reset();
    stretch_.setSpeed(speed_.load(std::memory_order_acquire));
    played_.store(positionMs * rate_ / 1000, std::memory_order_release);
    finished_.store(false, std::memory_order_release);
}

void Mp3Stream::setSpeed(double speed)
{
    speed_.store(std::clamp(speed, 0.25, 4.0), std::memory_order_release);
}

void Mp3Stream::setChannels(bool nearSide, bool farSide)
{

    if (!nearSide && !farSide) {
        nearSide = true;
        farSide = true;
    }
    nearSide_.store(nearSide, std::memory_order_release);
    farSide_.store(farSide, std::memory_order_release);
}

std::size_t Mp3Stream::pull(std::span<std::int16_t> out) noexcept
{

    core::RecordingRing::Frame frame;
    if (!ring_.read(frame, out)) {
        return 0;
    }

    const double speed = speed_.load(std::memory_order_relaxed);
    const auto advanced =
        static_cast<std::uint64_t>(std::llround(static_cast<double>(frame.count) * speed));

    played_.fetch_add(advanced, std::memory_order_release);
    return frame.count;
}

bool Mp3Stream::fill()
{
    const bool wantNear = nearSide_.load(std::memory_order_acquire);
    const bool wantFar = farSide_.load(std::memory_order_acquire);

    stretch_.setSpeed(speed_.load(std::memory_order_acquire));

    const double step = static_cast<double>(sourceRate_) / rate_;

    const auto wanted =
        static_cast<std::size_t>(std::ceil(step * static_cast<double>(kFrameSamples))) + 2;

    while (pending_.size() / channels_ < wanted) {
        const std::size_t had = pending_.size();
        pending_.resize(had + wanted * channels_);

        const std::size_t got =
            reader_->decoder.read(std::span<std::int16_t>{pending_}.subspan(had));
        pending_.resize(had + got);
        if (got == 0) {
            break;
        }
    }

    const std::size_t frames = pending_.size() / channels_;
    if (frames < 2) {
        return false;
    }

    mono_.resize(kFrameSamples);

    std::size_t made = 0;
    while (made < kFrameSamples) {
        const auto at = static_cast<std::size_t>(cursor_);
        if (at + 1 >= frames) {
            break;
        }

        const double fraction = cursor_ - static_cast<double>(at);

        const auto blend = [&](unsigned channel) {
            const auto one = pending_[at * channels_ + channel];
            const auto other = pending_[(at + 1) * channels_ + channel];
            return static_cast<double>(one) + (static_cast<double>(other) - one) * fraction;
        };

        double value = 0.0;
        if (channels_ >= 2) {
            const double nearValue = wantNear ? blend(0) : 0.0;
            const double farValue = wantFar ? blend(1) : 0.0;

            value = (wantNear && wantFar) ? (nearValue + farValue) / 2.0 : nearValue + farValue;
        } else {
            value = blend(0);
        }

        mono_[made] = static_cast<std::int16_t>(std::clamp(value, -32768.0, 32767.0));
        ++made;
        cursor_ += step;
    }

    const auto used = static_cast<std::size_t>(cursor_);
    if (used > 0) {
        pending_.erase(pending_.begin(),
                       pending_.begin() + static_cast<std::ptrdiff_t>(used * channels_));
        cursor_ -= static_cast<double>(used);
    }

    if (made == 0) {
        return false;
    }

    stretch_.write(std::span<const std::int16_t>{mono_.data(), made});
    return true;
}

void Mp3Stream::deliver(bool everything)
{

    for (;;) {
        spill_.resize(kFrameSamples);
        const std::size_t took = stretch_.read(std::span<std::int16_t>{spill_}.subspan(held_));
        held_ += took;

        if (held_ == kFrameSamples) {
            ring_.write(0, spill_.data(), held_);
            held_ = 0;
            continue;
        }

        if (took == 0) {

            if (everything && held_ > 0) {
                ring_.write(0, spill_.data(), held_);
                held_ = 0;
            }
            return;
        }
    }
}

void Mp3Stream::run()
{
    for (;;) {
        {
            std::unique_lock<std::mutex> held{mutex_};
            if (stopping_) {
                return;
            }

            const bool over = finished_.load(std::memory_order_acquire);

            if (!over && ring_.waiting() < static_cast<std::size_t>(rate_) * kDepthSeconds / 2) {
                if (!fill()) {

                    stretch_.finish();
                    deliver(true);
                    finished_.store(true, std::memory_order_release);
                } else {
                    deliver(false);
                }
                continue;
            }

            wake_.wait_for(held, std::chrono::milliseconds{20}, [this]() { return stopping_; });
        }
    }
}

}
