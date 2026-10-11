// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/streamed_transcription.hpp>

#include <algorithm>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <thread>
#include <utility>

namespace softphone::core {
namespace {

using namespace std::chrono_literals;

constexpr auto kChunk = 200ms;

constexpr auto kWaitForTheSocket = 30s;
constexpr auto kLookAgain = 20ms;

class Collected final : public IStreamingAsrSink {
public:
    void listening() override
    {
        const std::lock_guard<std::mutex> held{lock_};
        opened_ = true;
    }

    void partial(const AsrPartial &) override
    {

    }

    void turn(const AsrTurn &) override
    {

    }

    void gap(std::chrono::milliseconds, std::chrono::milliseconds lost) override
    {
        const std::lock_guard<std::mutex> held{lock_};
        lost_ += lost;
    }

    void spent(std::int64_t costMicros) override
    {
        const std::lock_guard<std::mutex> held{lock_};
        cost_ = costMicros;
    }

    void finished(const AsrResult &result) override
    {
        {
            const std::lock_guard<std::mutex> held{lock_};
            answer_ = result;
            if (cost_ != 0 && answer_->costMicros == 0) {
                answer_->costMicros = cost_;
            }
        }
        done_.notify_all();
    }

    [[nodiscard]] std::optional<AsrResult> waitFor(std::chrono::seconds patience,
                                                   const AsrCancel &cancelled)
    {
        const auto deadline = std::chrono::steady_clock::now() + patience;
        std::unique_lock<std::mutex> held{lock_};
        while (!answer_) {
            if (std::chrono::steady_clock::now() >= deadline) {
                return std::nullopt;
            }
            if (cancelled && cancelled()) {
                return std::nullopt;
            }
            done_.wait_for(held, kLookAgain);
        }
        return answer_;
    }

    [[nodiscard]] std::chrono::milliseconds lost() const
    {
        const std::lock_guard<std::mutex> held{lock_};
        return lost_;
    }

private:
    mutable std::mutex lock_;
    std::condition_variable done_;
    std::optional<AsrResult> answer_;
    std::chrono::milliseconds lost_{0};
    std::int64_t cost_{0};
    bool opened_{false};
};

[[nodiscard]] AsrResult refuse(AsrStatus status, std::string why)
{
    AsrResult result;
    result.status = status;
    result.error = std::move(why);
    return result;
}

}

StreamedTranscription::StreamedTranscription(std::string type, Opener opener, PcmReader reader)
    : type_{std::move(type)}, opener_{std::move(opener)}, reader_{std::move(reader)}
{}

AsrCapabilities StreamedTranscription::capabilities(const AsrEndpoint &where) const
{
    AsrCapabilities can;

    if (const auto recogniser = opener_ ? opener_() : nullptr) {
        const auto live = recogniser->capabilities(where);
        can.diarisation = live.diarisation;
        can.languageHint = live.languageHint;
        can.detectsLanguage = live.detectsLanguage;

        can.wordTimestamps = true;
    }

    can.multichannel = false;

    can.formats = {"audio/mpeg"};
    can.maxBytes = 0;
    can.maxDuration = std::chrono::seconds::zero();

    can.maxConcurrent = 1;
    return can;
}

AsrResult StreamedTranscription::transcribe(const AsrEndpoint &where, const AsrInput &input,
                                            const AsrOptions &options, const AsrCancel &cancelled)
{
    if (where.url.empty()) {
        return refuse(AsrStatus::Unsupported, "this provider has no address");
    }
    if (!opener_ || !reader_) {
        return refuse(AsrStatus::Unsupported, "this build cannot stream a recording");
    }

    auto recogniser = opener_();
    if (!recogniser) {
        return refuse(AsrStatus::Unsupported, "this build has no adapter for " + type_);
    }

    const auto can = recogniser->capabilities(where);
    const auto wire = wireFor(can, 16000);
    if (!wire) {
        return refuse(AsrStatus::Unsupported, "this recogniser takes no format this build makes");
    }

    std::vector<std::int16_t> mono;
    if (!reader_(input.file, wire->sampleRate, mono)) {
        return refuse(AsrStatus::Failed, "this recording could not be decoded");
    }
    if (mono.empty()) {
        return refuse(AsrStatus::Failed, "this recording held no audio");
    }

    StreamingOptions asking;
    asking.language = options.language;
    asking.voice = input.voice;
    asking.wire = *wire;
    asking.wordTimestamps = options.wordTimestamps;
    asking.diarise = options.diarise && can.diarisation;
    asking.params = where.params;

    Collected collected;
    auto session = recogniser->open(where, asking, collected, cancelled);
    if (!session) {
        return refuse(AsrStatus::Unavailable, "this recogniser would not open a session");
    }

    const auto perChunk =
        static_cast<std::size_t>(kChunk.count()) * wire->sampleRate / 1000 * wire->channels;
    const auto began = std::chrono::steady_clock::now();

    for (std::size_t at = 0; at < mono.size(); at += perChunk) {
        if (cancelled && cancelled()) {
            return refuse(AsrStatus::Cancelled, "cancelled");
        }

        const auto sentSoFar = std::chrono::milliseconds{
            static_cast<std::int64_t>(at * 1000 / wire->sampleRate / wire->channels)};
        const auto allowedAfter = sentSoFar / kStreamedFasterThanLife;
        while (std::chrono::steady_clock::now() - began < allowedAfter) {
            if (cancelled && cancelled()) {
                return refuse(AsrStatus::Cancelled, "cancelled");
            }
            std::this_thread::sleep_for(kLookAgain);
        }

        const auto giveUp = std::chrono::steady_clock::now() + kWaitForTheSocket;
        while (!session->listening()) {
            if (cancelled && cancelled()) {
                return refuse(AsrStatus::Cancelled, "cancelled");
            }
            if (std::chrono::steady_clock::now() >= giveUp) {
                return refuse(AsrStatus::Unavailable,
                              "this recogniser stopped taking audio part way through");
            }
            std::this_thread::sleep_for(kLookAgain);
        }

        const auto left = std::min(perChunk, mono.size() - at);
        static_cast<void>(
            session->send(sentSoFar, std::span<const std::int16_t>{mono.data() + at, left}));
    }

    session->drain();

    auto answer = collected.waitFor(where.timeout, cancelled);
    if (!answer) {
        if (cancelled && cancelled()) {
            return refuse(AsrStatus::Cancelled, "cancelled");
        }
        return refuse(AsrStatus::Unavailable, "this recogniser did not finish inside its deadline");
    }

    if (const auto lost = collected.lost(); lost > std::chrono::milliseconds::zero()) {
        answer->error = "audio was lost while streaming this recording to the recogniser";
    }
    if (answer->model.empty()) {
        answer->model = where.model;
    }
    return *answer;
}

AsrProbe StreamedTranscription::probe(const AsrEndpoint &where)
{

    if (!opener_) {
        AsrProbe answer;
        answer.status = AsrStatus::Unsupported;
        answer.detail = "this build has no adapter for " + type_;
        return answer;
    }
    const auto recogniser = opener_();
    if (!recogniser) {
        AsrProbe answer;
        answer.status = AsrStatus::Unsupported;
        answer.detail = "this build has no adapter for " + type_;
        return answer;
    }
    return recogniser->probe(where);
}

}
