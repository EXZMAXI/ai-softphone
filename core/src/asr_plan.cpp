// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/asr_plan.hpp>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace softphone::core {
namespace {

using namespace std::chrono_literals;

constexpr std::string_view kMpeg{"audio/mpeg"};
constexpr std::string_view kWav{"audio/wav"};

[[nodiscard]] bool weCanWrite(std::string_view mime) noexcept
{
    return mime == kMpeg || mime == kWav;
}

[[nodiscard]] bool accepts(const AsrCapabilities &can, std::string_view mime)
{
    return std::find(can.formats.begin(), can.formats.end(), mime) != can.formats.end();
}

[[nodiscard]] std::string chooseFormat(const AsrCapabilities &can, std::string_view source,
                                       bool rewritten)
{
    if (!rewritten && accepts(can, source)) {
        return std::string{source};
    }
    for (const auto &offered : can.formats) {
        if (weCanWrite(offered)) {
            return offered;
        }
    }
    return {};
}

[[nodiscard]] std::chrono::milliseconds
pieceLength(const AsrCapabilities &can, std::chrono::milliseconds whole, double bytesPerMs)
{
    auto limit = whole;
    if (can.maxDuration > 0s) {
        limit =
            std::min(limit, std::chrono::duration_cast<std::chrono::milliseconds>(can.maxDuration));
    }
    if (can.maxBytes > 0 && bytesPerMs > 0.0) {
        const auto byBytes =
            static_cast<std::int64_t>(static_cast<double>(can.maxBytes) / bytesPerMs);
        limit = std::min(limit, std::chrono::milliseconds{std::max<std::int64_t>(byBytes, 0)});
    }
    return limit;
}

void offer(AsrPlan &plan, const AsrCapabilities &can, const AsrOptions &options,
           const std::filesystem::path &file, std::string_view sourceMime, Voice voice,
           std::string_view stemId, std::optional<unsigned> sourceChannel,
           std::chrono::milliseconds offset, std::chrono::milliseconds duration, double bytesPerMs,
           unsigned channels, std::vector<Voice> channelVoices)
{
    const auto rewritten = sourceChannel.has_value();
    const auto whole = pieceLength(can, duration, bytesPerMs);
    if (whole <= 0ms) {
        plan.refusal = "the provider takes less audio than one piece of this recording";
        return;
    }

    const auto cut = whole < duration;
    const auto mime = chooseFormat(can, sourceMime, rewritten || cut);
    if (mime.empty()) {
        plan.refusal = "no audio format this build writes is one the provider accepts";
        return;
    }

    for (auto at = 0ms; at < duration; at += whole) {
        AsrStep step;
        step.input.file = file;
        step.input.mimeType = mime;
        step.input.voice = voice;
        step.input.stemId = std::string{stemId};
        step.input.offset = offset + at;
        step.input.from = at;
        step.input.duration = std::min(whole, duration - at);
        step.input.sourceChannel = sourceChannel;
        step.input.channels = channels;
        step.input.channelVoices = channelVoices;
        step.input.rewrite = rewritten || cut;
        step.options = options;
        plan.steps.push_back(std::move(step));
    }
}

}

AsrPlan planTranscription(const AsrCapabilities &can, const AsrSubject &subject,
                          const AsrOptions &wanted)
{
    AsrPlan plan;

    if (subject.file.empty() || subject.duration <= 0ms) {
        plan.refusal = "the recording has no audio to send";
        return plan;
    }

    AsrOptions shared = wanted;
    shared.wordTimestamps = wanted.wordTimestamps && can.wordTimestamps;
    if (!can.languageHint) {
        shared.language.clear();
    }
    shared.diarise = false;

    const auto bytesPerMs =
        subject.duration > 0ms && subject.bytes > 0
            ? static_cast<double>(subject.bytes) / static_cast<double>(subject.duration.count())
            : 0.0;

    AsrOptions mixedInput = shared;
    mixedInput.diarise = can.diarisation;

    const auto separable = subject.nearFar && subject.channels >= 2;

    const auto stemsUsable =
        !subject.stems.empty() &&
        std::all_of(subject.stems.begin(), subject.stems.end(), [](const AsrSubject::Stem &s) {
            return s.present && !s.file.empty() && s.duration > 0ms;
        });

    if (!separable) {

        offer(plan, can, mixedInput, subject.file, subject.mimeType, Voice::Mixed, {}, std::nullopt,
              0ms, subject.duration, bytesPerMs, subject.channels, {});
        return plan;
    }

    const auto nearBytesPerMs = bytesPerMs / static_cast<double>(std::max(subject.channels, 1U));

    if (stemsUsable) {
        offer(plan, can, shared, subject.file, subject.mimeType, Voice::Near, {}, 0U, 0ms,
              subject.duration, nearBytesPerMs, 1, {});
        for (const auto &stem : subject.stems) {
            if (!plan.refusal.empty()) {
                break;
            }
            const auto perMs =
                stem.duration > 0ms && stem.bytes > 0
                    ? static_cast<double>(stem.bytes) / static_cast<double>(stem.duration.count())
                    : 0.0;
            offer(plan, can, shared, stem.file, subject.mimeType, Voice::Stem, stem.id,
                  std::nullopt, stem.offset, stem.duration, perMs, 1, {});
        }
        return plan;
    }

    if (can.multichannel) {
        auto options = shared;

        options.diarise = can.diarisation && subject.parties > 2;
        offer(plan, can, options, subject.file, subject.mimeType, Voice::Mixed, {}, std::nullopt,
              0ms, subject.duration, bytesPerMs, subject.channels, {Voice::Near, Voice::Far});
        return plan;
    }

    auto farOptions = shared;
    farOptions.diarise = can.diarisation && subject.parties > 2;

    offer(plan, can, shared, subject.file, subject.mimeType, Voice::Near, {}, 0U, 0ms,
          subject.duration, nearBytesPerMs, 1, {});
    if (plan.refusal.empty()) {
        offer(plan, can, farOptions, subject.file, subject.mimeType, Voice::Far, {}, 1U, 0ms,
              subject.duration, nearBytesPerMs, 1, {});
    }
    return plan;
}

}
