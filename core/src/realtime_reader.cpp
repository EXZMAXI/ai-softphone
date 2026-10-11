// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/realtime_reader.hpp>

#include <algorithm>
#include <chrono>
#include <span>

namespace softphone::core {
namespace {

[[nodiscard]] std::chrono::milliseconds asTime(std::uint64_t samples, unsigned rate) noexcept
{
    if (rate == 0) {
        return std::chrono::milliseconds::zero();
    }
    return std::chrono::milliseconds{static_cast<std::int64_t>(samples * 1000 / rate)};
}

constexpr auto kMostAtOnce = std::chrono::milliseconds{200};

}

RealtimeReader::RealtimeReader(RealtimeRing &ring, unsigned sourceRate, AsrWireFormat wire,
                               std::chrono::milliseconds leastPerMessage)
    : ring_{ring}, sourceRate_{sourceRate == 0 ? 16000 : sourceRate}, wire_{wire},
      least_{leastPerMessage}
{

    read_.resize(static_cast<std::size_t>(sourceRate_) / 2);

    if (const auto edge = ring.clock(); edge != 0) {
        origin_ = edge;
    }
}

RealtimeReader::Pumped RealtimeReader::pump(IStreamingAsrSession &into)
{
    Pumped did;

    RealtimeRing::Frame frame;
    if (!ring_.read(frame, read_)) {
        return did;
    }

    if (frame.lostBefore > 0) {
        lost_ += frame.lostBefore;
        did.lost = asTime(frame.lostBefore, sourceRate_);
        did.lostAt = sessionClock(frame.position);

        held_ = ResampleState{};
    }

    if (frame.count == 0) {
        return did;
    }

    const auto taken = std::span<const std::int16_t>{read_.data(), frame.count};

    converted_.clear();
    if (wire_.sampleRate == sourceRate_) {
        converted_.assign(taken.begin(), taken.end());
    } else {
        resample(held_, taken, sourceRate_, wire_.sampleRate, converted_);
    }

    if (converted_.empty()) {
        return did;
    }

    const auto at = sessionClock(frame.position);
    if (!offer(into, converted_, at)) {
        did.refused = true;
        return did;
    }

    sent_ += converted_.size();
    did.sent = converted_.size();

    reached_ = std::max(reached_, at + asTime(converted_.size(), wire_.sampleRate));

    heard_ = std::max(heard_, at + asTime(converted_.size(), wire_.sampleRate));
    justHeard_ = true;
    return did;
}

bool RealtimeReader::offer(IStreamingAsrSession &into, std::span<const std::int16_t> samples,
                           std::chrono::milliseconds at)
{

    if (least_ <= std::chrono::milliseconds::zero()) {
        return into.send(at, samples);
    }

    if (waiting_.empty()) {
        waitingAt_ = at;
    }
    waiting_.insert(waiting_.end(), samples.begin(), samples.end());

    if (asTime(waiting_.size(), wire_.sampleRate) < least_) {
        return true;
    }

    const auto ok = into.send(waitingAt_, waiting_);
    waiting_.clear();
    return ok;
}

bool RealtimeReader::flush(IStreamingAsrSession &into)
{
    if (waiting_.empty()) {
        return false;
    }

    const auto least = static_cast<std::size_t>(least_.count()) * wire_.sampleRate / 1000;
    if (waiting_.size() < least) {
        hush_sent_ += least - waiting_.size();
        waiting_.resize(least, 0);
    }

    const auto ok = into.send(waitingAt_, waiting_);
    waiting_.clear();
    return ok;
}

bool RealtimeReader::quiet(IStreamingAsrSession &into, std::chrono::milliseconds upTo)
{
    if (ring_.begins() == RealtimeRing::kNothingYet) {
        return false;
    }

    if (justHeard_) {
        justHeard_ = false;
        base_ = std::max(base_, upTo > heard_ ? upTo - heard_ : std::chrono::milliseconds::zero());
    }

    const auto target = upTo > base_ ? upTo - base_ : std::chrono::milliseconds::zero();
    if (target <= reached_) {
        return false;
    }

    const auto owed = std::min(target - reached_, kMostAtOnce);
    const auto samples = static_cast<std::size_t>(owed.count()) * wire_.sampleRate / 1000;
    if (samples == 0) {
        return false;
    }

    hush_.assign(samples, 0);
    if (!offer(into, hush_, reached_)) {
        return false;
    }

    hush_sent_ += samples;
    reached_ += owed;
    return true;
}

std::chrono::milliseconds RealtimeReader::sessionClock(std::uint64_t position) const noexcept
{

    if (origin_ == RealtimeRing::kNothingYet) {
        origin_ = position;
    }
    if (position < origin_) {

        return std::chrono::milliseconds::zero();
    }
    return asTime(position - origin_, sourceRate_);
}

std::chrono::milliseconds RealtimeReader::lostAltogether() const noexcept
{
    return asTime(lost_, sourceRate_);
}

}
