// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/conversation_recorder.hpp>

#include <softphone/core/logging.hpp>

#include <algorithm>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__linux__)
#include <sys/resource.h>
#include <unistd.h>
#endif

namespace softphone::core {
namespace {

void stepAside() noexcept
{
#if defined(_WIN32)
    static_cast<void>(SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL));
#elif defined(__linux__)

    static_cast<void>(setpriority(PRIO_PROCESS, static_cast<id_t>(gettid()), 5));
#endif
}

}

ConversationRecorder::ConversationRecorder(RecordingFeed &feed, EncoderFactory factory)
    : feed_{feed}, factory_{std::move(factory)}
{

    scratch_.resize(static_cast<std::size_t>(feed_.frameSamples()) * 2);
}

ConversationRecorder::~ConversationRecorder()
{
    static_cast<void>(stop());
}

EncoderStatus ConversationRecorder::start()
{
    if (running_.load(std::memory_order_acquire)) {
        return EncoderStatus::Ok;
    }

    encoder_ = factory_ ? factory_(std::string{}) : nullptr;
    if (!encoder_) {
        return EncoderStatus::Unwritable;
    }

    writer_ = std::make_unique<TimelineWriter>(2, *encoder_);

    feed_.restart();

    origin_ = 0;
    begun_ = false;
    stems_.clear();
    keepingNear_ = true;
    keepingFar_ = true;
    {
        const std::lock_guard<std::mutex> held{switchMutex_};
        switches_.clear();
    }
    stopping_ = false;
    stopped_ = false;
    outcome_ = RecordingOutcome{};
    written_.store(0, std::memory_order_relaxed);

    running_.store(true, std::memory_order_release);
    thread_ = std::thread{[this]() { run(); }};

    return EncoderStatus::Ok;
}

RecordingOutcome ConversationRecorder::stop()
{
    if (stopped_) {
        return outcome_;
    }
    if (!running_.load(std::memory_order_acquire)) {
        return outcome_;
    }

    {
        const std::lock_guard<std::mutex> held{mutex_};
        stopping_ = true;
    }
    wake_.notify_all();

    if (thread_.joinable()) {
        thread_.join();
    }

    running_.store(false, std::memory_order_release);
    stopped_ = true;
    return outcome_;
}

std::chrono::milliseconds ConversationRecorder::elapsed() const noexcept
{
    const std::uint64_t samples = written_.load(std::memory_order_acquire);
    const auto rate = static_cast<std::uint64_t>(feed_.sampleRate());
    return std::chrono::milliseconds{rate == 0 ? 0 : (samples * 1000) / rate};
}

bool ConversationRecorder::findTheStart()
{
    if (begun_) {
        return true;
    }

    const std::uint64_t here = feed_.nearSide().begins();
    const std::uint64_t there = feed_.farSide().begins();
    const std::uint64_t first = std::min(here, there);
    if (first == RecordingRing::kNothingYet) {
        return false;
    }

    origin_ = first;
    begun_ = true;
    return true;
}

void ConversationRecorder::keepAudio(bool keeping)
{
    const std::lock_guard<std::mutex> held{switchMutex_};
    switches_.push_back(Switch{feed_.nearSide().clock(), keeping, Switch::Side::Both});
}

void ConversationRecorder::keepNearSide(bool keeping)
{
    const std::lock_guard<std::mutex> held{switchMutex_};
    switches_.push_back(Switch{feed_.nearSide().clock(), keeping, Switch::Side::Near});
}

void ConversationRecorder::keepFarSide(bool keeping)
{
    const std::lock_guard<std::mutex> held{switchMutex_};
    switches_.push_back(Switch{feed_.nearSide().clock(), keeping, Switch::Side::Far});
}

void ConversationRecorder::openStems()
{
    for (const RecordingFeed::Stem &offered : feed_.stems()) {
        const bool known = std::any_of(stems_.begin(), stems_.end(), [&](const Stem &stem) {
            return stem.callId == offered.callId;
        });
        if (known || offered.ring == nullptr) {
            continue;
        }

        const std::uint64_t begins = offered.ring->begins();
        if (begins == RecordingRing::kNothingYet) {
            continue;
        }

        auto encoder = factory_ ? factory_(offered.callId) : nullptr;
        if (!encoder) {
            const LogAttribute fields[] = {{"call", offered.callId}};
            Logger::instance().log(LogLevel::Warning, "recording",
                                   "no stem for this leg: the file would not open", fields);

            stems_.push_back(Stem{offered.callId, offered.ring, nullptr, nullptr, 0});
            continue;
        }

        const std::uint64_t offset = begins > origin_ ? begins - origin_ : 0;

        Stem stem;
        stem.callId = offered.callId;
        stem.ring = offered.ring;
        stem.writer = std::make_unique<TimelineWriter>(1, *encoder, offset);
        stem.encoder = std::move(encoder);
        stem.offset = offset;

        stem.writer->mute(offset, !keepingFar_);

        stems_.push_back(std::move(stem));
    }
}

void ConversationRecorder::drain()
{

    const std::uint64_t clock = std::max(feed_.nearSide().clock(), feed_.farSide().clock());

    const std::uint64_t settled = feed_.nearSide().clock();

    if (!findTheStart()) {
        return;
    }

    writer_->settled(settled > origin_ ? settled - origin_ : 0);

    openStems();

    std::vector<Switch> changes;
    {
        const std::lock_guard<std::mutex> held{switchMutex_};
        changes.swap(switches_);
    }

    for (const Switch &change : changes) {
        const std::uint64_t at = change.position > origin_ ? change.position - origin_ : 0;

        if (change.side != Switch::Side::Far) {
            writer_->mute(0, at, !change.keeping);
            keepingNear_ = change.keeping;
        }

        if (change.side != Switch::Side::Near) {
            writer_->mute(1, at, !change.keeping);
            for (Stem &stem : stems_) {
                if (stem.writer) {
                    stem.writer->mute(at, !change.keeping);
                }
            }
            keepingFar_ = change.keeping;
        }
    }

    RecordingRing::Frame frame;

    while (feed_.nearSide().read(frame, scratch_)) {
        if (frame.position < origin_) {
            continue;
        }
        writer_->offer(0, frame.position - origin_,
                       std::span<const std::int16_t>{scratch_.data(), frame.count});
    }

    while (feed_.farSide().read(frame, scratch_)) {
        if (frame.position < origin_) {
            continue;
        }
        writer_->offer(1, frame.position - origin_,
                       std::span<const std::int16_t>{scratch_.data(), frame.count});
    }

    for (Stem &stem : stems_) {
        if (!stem.writer) {
            continue;
        }
        while (stem.ring->read(frame, scratch_)) {
            if (frame.position < origin_) {
                continue;
            }
            stem.writer->offer(0, frame.position - origin_,
                               std::span<const std::int16_t>{scratch_.data(), frame.count});
        }
    }

    if (clock <= origin_) {
        return;
    }

    const auto lag = static_cast<std::uint64_t>(feed_.sampleRate()) *
                     static_cast<std::uint64_t>(kLag.count()) / 1000;
    const std::uint64_t reached = clock - origin_;
    if (reached > lag) {
        advanceTo(upTo(reached - lag));
    }
}

std::uint64_t ConversationRecorder::upTo(std::uint64_t wanted) const
{

    const auto quiet = writer_->quietFrom();
    return quiet ? std::min(wanted, *quiet) : wanted;
}

void ConversationRecorder::advanceTo(std::uint64_t position)
{
    static_cast<void>(writer_->advance(position));
    written_.store(writer_->written(), std::memory_order_release);

    for (Stem &stem : stems_) {
        if (stem.writer && position > stem.offset) {
            static_cast<void>(stem.writer->advance(position));
        }
    }
}

void ConversationRecorder::run()
{
    stepAside();

    for (;;) {
        drain();

        std::unique_lock<std::mutex> held{mutex_};
        if (stopping_) {
            break;
        }
        wake_.wait_for(held, kCadence, [this]() { return stopping_; });
        if (stopping_) {
            held.unlock();
            break;
        }
    }

    drain();
    if (begun_) {
        const std::uint64_t last = std::max(feed_.nearSide().clock(), feed_.farSide().clock());
        if (last > origin_) {
            advanceTo(upTo(last - origin_));
        }
    }

    const auto rateForMs = [this](std::uint64_t samples) {
        const auto rate = static_cast<std::uint64_t>(feed_.sampleRate());
        return std::chrono::milliseconds{rate == 0 ? 0 : (samples * 1000) / rate};
    };

    for (Stem &stem : stems_) {
        if (!stem.writer) {
            continue;
        }
        StemOutcome written;
        written.callId = stem.callId;
        written.offset = rateForMs(stem.offset);
        written.status = stem.writer->finish();
        written.duration = rateForMs(stem.writer->written());
        written.bytes = stem.encoder->bytes();
        outcome_.stems.push_back(std::move(written));
    }

    outcome_.status = writer_->finish();
    outcome_.bytes = encoder_->bytes();

    const auto rate = static_cast<std::uint64_t>(feed_.sampleRate());
    outcome_.duration =
        std::chrono::milliseconds{rate == 0 ? 0 : (writer_->written() * 1000) / rate};

    outcome_.gapSamples = feed_.nearSide().dropped() + feed_.farSide().dropped();
    for (const Stem &stem : stems_) {
        if (stem.ring != nullptr) {
            outcome_.gapSamples += stem.ring->dropped();
        }
    }

    written_.store(writer_->written(), std::memory_order_release);

    if (!succeeded(outcome_.status)) {
        Logger::instance().log(LogLevel::Warning, "recording",
                               "the recording stopped early: " +
                                   std::string{toString(outcome_.status)});
    } else if (outcome_.hasGaps()) {
        Logger::instance().log(
            LogLevel::Warning, "recording",
            "the recording has gaps in it: " + std::to_string(outcome_.gapSamples) + " samples");
    }
}

}
