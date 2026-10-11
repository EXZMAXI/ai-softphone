// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/provider_knobs.hpp>

#include <algorithm>

namespace softphone::core {
namespace {

[[nodiscard]] ProviderKnob sent(std::string name, KnobKind kind, std::string fallback,
                                bool optional = true)
{
    return {std::move(name), kind, std::move(fallback), true, optional};
}

[[nodiscard]] ProviderKnob ours(std::string name, KnobKind kind, std::string fallback)
{
    return {std::move(name), kind, std::move(fallback), false, false};
}

[[nodiscard]] std::vector<ProviderKnob> openAiChat()
{
    return {
        sent("temperature", KnobKind::Decimal, "0"),
        sent("max_output", KnobKind::Number, "4096"),
        sent("json_mode", KnobKind::Flag, "true", false),
        ours("context_tokens", KnobKind::Number, "8192"),
        ours("max_concurrent", KnobKind::Number, "2"),
    };
}

[[nodiscard]] std::vector<ProviderKnob> xaiChat()
{
    auto knobs = openAiChat();
    for (auto &knob : knobs) {
        if (knob.name == "context_tokens") {
            knob.fallback = "256000";
        }
    }
    return knobs;
}

[[nodiscard]] std::vector<ProviderKnob> yandexChat()
{
    auto knobs = openAiChat();
    for (auto &knob : knobs) {
        if (knob.name == "context_tokens") {
            knob.fallback = "32000";
        }
    }
    knobs.insert(knobs.begin(), sent("folder_id", KnobKind::Text, ""));
    return knobs;
}

[[nodiscard]] std::vector<ProviderKnob> anthropicChat()
{
    return {
        sent("temperature", KnobKind::Decimal, "0"),
        sent("max_output", KnobKind::Number, "8192", false),
        ours("context_tokens", KnobKind::Number, "200000"),
        ours("max_concurrent", KnobKind::Number, "2"),
    };
}

[[nodiscard]] std::vector<ProviderKnob> everyLiveRecogniser()
{
    return {
        ours("turn_idle_ms", KnobKind::Number, "2500"),
        ours("turn_join_ms", KnobKind::Number, "900"),
    };
}

[[nodiscard]] std::vector<ProviderKnob> everyRecogniser(const char *maxBytes, const char *atOnce)
{
    auto knobs = everyLiveRecogniser();
    knobs.insert(knobs.begin(), {
                                    ours("max_bytes", KnobKind::Number, maxBytes),
                                    ours("max_duration", KnobKind::Number, "0"),
                                    ours("max_concurrent", KnobKind::Number, atOnce),
                                });
    return knobs;
}

[[nodiscard]] std::vector<ProviderKnob> openAiRecogniser()
{
    auto knobs = everyRecogniser("26214400", "1");
    knobs.insert(knobs.begin(), {
                                    sent("word_timestamps", KnobKind::Flag, "true", false),
                                    sent("multichannel", KnobKind::Flag, "false", false),
                                    sent("diarisation", KnobKind::Flag, "false", false),
                                });
    return knobs;
}

[[nodiscard]] std::vector<ProviderKnob> deepgramRecogniser()
{
    auto knobs = everyRecogniser("2147483648", "2");
    knobs.insert(knobs.begin(), {
                                    sent("multichannel", KnobKind::Flag, "true", false),
                                    sent("diarisation", KnobKind::Flag, "true", false),
                                });
    return knobs;
}

[[nodiscard]] std::vector<ProviderKnob> elevenLabsRecogniser()
{
    auto knobs = everyRecogniser("1073741824", "2");
    knobs.insert(knobs.begin(), sent("diarisation", KnobKind::Flag, "true", false));
    knobs.push_back(ours("turn_gap_ms", KnobKind::Number, "1500"));
    return knobs;
}

[[nodiscard]] std::vector<ProviderKnob> speechmaticsRecogniser()
{
    auto knobs = everyRecogniser("4294967296", "2");
    knobs.insert(knobs.begin(), sent("diarisation", KnobKind::Flag, "true", false));
    knobs.push_back(ours("turn_gap_ms", KnobKind::Number, "1500"));

    knobs.push_back(sent("operating_point", KnobKind::Text, ""));
    return knobs;
}

[[nodiscard]] std::vector<ProviderKnob> xaiRecogniser()
{
    auto knobs = everyRecogniser("500000000", "2");
    knobs.insert(knobs.begin(), {
                                    sent("multichannel", KnobKind::Flag, "true", false),
                                    sent("diarisation", KnobKind::Flag, "true", false),
                                    sent("format", KnobKind::Flag, "true", false),
                                });
    knobs.push_back(ours("turn_gap_ms", KnobKind::Number, "1500"));
    return knobs;
}

[[nodiscard]] std::vector<ProviderKnob> yandexRecogniser()
{
    auto knobs = everyRecogniser("45000000", "2");
    for (auto &knob : knobs) {
        if (knob.name == "max_duration") {
            knob.fallback = "14400";
        }
    }
    knobs.insert(knobs.begin(), {
                                    sent("folder_id", KnobKind::Text, ""),
                                    sent("text_normalization", KnobKind::Flag, "true", false),
                                });
    knobs.push_back(
        ours("operations_url", KnobKind::Text, "https://operation.api.cloud.yandex.net"));
    return knobs;
}

}

std::vector<ProviderKnob> knobsFor(ProviderKind kind, std::string_view type)
{
    if (kind == ProviderKind::Llm) {
        if (type == "anthropic") {
            return anthropicChat();
        }
        if (type == "openai") {
            return openAiChat();
        }
        if (type == "xai") {
            return xaiChat();
        }
        if (type == "yandex") {
            return yandexChat();
        }
        return {};
    }

    if (type == "deepgram") {
        return deepgramRecogniser();
    }
    if (type == "elevenlabs") {
        return elevenLabsRecogniser();
    }
    if (type == "speechmatics") {
        return speechmaticsRecogniser();
    }
    if (type == "xai") {
        return xaiRecogniser();
    }
    if (type == "yandex") {
        return yandexRecogniser();
    }
    if (type == "openai") {
        return openAiRecogniser();
    }

    return everyLiveRecogniser();
}

const ProviderKnob *knobIn(const std::vector<ProviderKnob> &knobs, std::string_view name)
{
    const auto found = std::find_if(knobs.begin(), knobs.end(),
                                    [name](const ProviderKnob &one) { return one.name == name; });
    return found == knobs.end() ? nullptr : &*found;
}

std::string knobValue(const std::vector<std::pair<std::string, std::string>> &params,
                      const std::vector<ProviderKnob> &knobs, std::string_view name)
{
    const auto stored = std::find_if(params.begin(), params.end(),
                                     [name](const auto &pair) { return pair.first == name; });
    if (stored != params.end()) {
        return stored->second;
    }
    const auto *const knob = knobIn(knobs, name);
    return knob == nullptr ? std::string{} : knob->fallback;
}

bool isKnob(const std::vector<ProviderKnob> &knobs, std::string_view name)
{
    return knobIn(knobs, name) != nullptr;
}

}
