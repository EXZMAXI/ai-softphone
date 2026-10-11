// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/streaming_asr.hpp>

#include <algorithm>

namespace softphone::core {

std::string_view toString(AsrEncoding encoding) noexcept
{
    switch (encoding) {
    case AsrEncoding::Mulaw:
        return "mulaw";
    case AsrEncoding::Opus:
        return "opus";
    case AsrEncoding::Pcm16:
        break;
    }
    return "pcm16";
}

std::string_view toString(PromptingRefused why) noexcept
{
    switch (why) {
    case PromptingRefused::NobodyWasTold:
        return "nobody-was-told";
    case PromptingRefused::Nothing:
        break;
    }
    return {};
}

PromptingRefused promptingAllowed(ConsentPosture posture, bool announced) noexcept
{
    switch (posture) {
    case ConsentPosture::Announce:
    case ConsentPosture::Tone:

        return announced ? PromptingRefused::Nothing : PromptingRefused::NobodyWasTold;

    case ConsentPosture::Confirm:
    case ConsentPosture::Silent:

        return PromptingRefused::Nothing;
    }
    return PromptingRefused::Nothing;
}

std::optional<AsrWireFormat> wireFor(const StreamingCapabilities &what, unsigned have) noexcept
{

    const auto makeable = [](const AsrWireFormat &wire) {
        return wire.encoding == AsrEncoding::Pcm16 || wire.encoding == AsrEncoding::Mulaw;
    };

    const auto best = std::ranges::find_if(what.accepts, makeable);
    if (best == what.accepts.end()) {
        return std::nullopt;
    }

    const auto asIs = std::ranges::find_if(what.accepts, [&](const AsrWireFormat &wire) {
        return makeable(wire) && wire.sampleRate == have && wire.encoding == best->encoding;
    });
    if (asIs != what.accepts.end()) {
        return *asIs;
    }
    return *best;
}

const std::vector<std::string> &streamingTypes()
{

    static const std::vector<std::string> all{
        "xai",        "elevenlabs", "deepgram", "openai",       "speechmatics", "soniox",
        "assemblyai", "yandex",     "vosk",     "whisper-live", "riva"};
    return all;
}

bool runsOnThisMachine(std::string_view type)
{

    return type == "vosk" || type == "whisper-live" || type == "riva";
}

bool canStreamType(std::string_view type)
{
    return std::ranges::find(streamingTypes(), type) != streamingTypes().end();
}

const std::vector<std::string> &transcribingTypes()
{

    static const std::vector<std::string> all{"openai", "deepgram",   "elevenlabs", "speechmatics",
                                              "xai",    "yandex",     "vosk",       "whisper-live",
                                              "riva",   "assemblyai", "soniox"};
    return all;
}

bool canTranscribeType(std::string_view type)
{
    return std::ranges::find(transcribingTypes(), type) != transcribingTypes().end();
}

std::string_view liveAddressFor(std::string_view type) noexcept
{
    if (type == "deepgram") {
        return "wss://api.deepgram.com";
    }

    if (type == "openai") {
        return "wss://api.openai.com";
    }
    if (type == "elevenlabs") {
        return "wss://api.elevenlabs.io";
    }
    if (type == "xai") {
        return "wss://api.x.ai";
    }
    if (type == "soniox") {
        return "wss://stt-rt.soniox.com";
    }
    if (type == "yandex") {

        return "https://stt.api.cloud.yandex.net:443";
    }
    if (type == "assemblyai") {
        return "wss://streaming.assemblyai.com";
    }

    return {};
}

std::string_view liveModelFor(std::string_view type) noexcept
{

    if (type == "openai") {

        return "gpt-live-transcribe";
    }
    if (type == "soniox") {
        return "stt-rt-v5";
    }
    if (type == "elevenlabs") {
        return "scribe_v2_realtime";
    }
    if (type == "xai") {

        return "grok-voice-transcribe-2.0";
    }
    if (type == "whisper-live") {
        return "small";
    }

    return {};
}

std::string preferredModelFor(std::string_view type, bool live)
{

    if (runsOnThisMachine(type) || type == "openai") {
        return {};
    }

    const auto known = knownModels(type, live);
    return known.empty() ? std::string{} : known.front();
}

std::vector<LiveRegion> liveRegionsFor(std::string_view type)
{
    if (type != "speechmatics") {
        return {};
    }

    return {
        {"global", "wss://global.rt.speechmatics.com", {}},
        {"eu", "wss://eu.rt.speechmatics.com",
         "AT BE BG HR CY CZ DK EE FI FR DE GR HU IE IT LV LT LU MT NL PL PT RO SK SI ES SE "
         "GB CH NO IS LI"},
        {"us", "wss://us.rt.speechmatics.com", "US CA MX BR AR CL CO PE"},
        {"au", "wss://au.rt.speechmatics.com", "AU NZ SG MY ID PH TH VN JP KR IN"},
    };
}

std::string_view liveAddressHere(std::string_view type, std::string_view territory)
{
    const auto regions = liveRegionsFor(type);
    if (regions.empty()) {
        return {};
    }

    for (const auto &region : regions) {
        if (region.territories.empty()) {
            continue;
        }

        for (std::size_t at = 0; at < region.territories.size(); at += 3) {
            if (region.territories.substr(at, 2) == territory) {
                return region.address;
            }
        }
    }

    for (const auto &region : regions) {
        if (region.territories.empty()) {
            return region.address;
        }
    }
    return {};
}

std::vector<std::string> knownModels(std::string_view type, bool live)
{

    if (type == "deepgram") {
        return {"nova-3", "nova-2", "nova-2-phonecall", "flux-general-multi", "flux-general-en"};
    }

    if (type == "openai") {
        return live ? std::vector<std::string>{"gpt-live-transcribe", "gpt-transcribe"}
                    : std::vector<std::string>{};
    }

    if (type == "speechmatics") {
        return {"standard", "enhanced", "melia-1"};
    }

    if (type == "assemblyai") {
        return live ? std::vector<std::string>{}
                    : std::vector<std::string>{"universal-3-5-pro", "universal-2"};
    }

    if (type == "xai") {
        return {"grok-voice-transcribe-2.0", "grok-voice-transcribe-1.0"};
    }

    if (type == "yandex") {
        return live ? std::vector<std::string>{"general"}
                    : std::vector<std::string>{"general", "deferred-general"};
    }

    if (type == "elevenlabs") {
        return live ? std::vector<std::string>{"scribe_v2_realtime"} : std::vector<std::string>{};
    }
    if (type == "soniox") {
        return live ? std::vector<std::string>{"stt-rt-v5"} : std::vector<std::string>{};
    }

    return {};
}

ChosenRecogniser recogniserForPrompter(const std::vector<ProviderConfig> &rows,
                                       std::string_view named,
                                       const std::function<bool(const ProviderConfig &)> &usable)
{
    const auto fit = [&usable](const ProviderConfig &one) { return one.enabled && usable(one); };

    if (!named.empty()) {
        const auto asked = std::ranges::find_if(
            rows, [named](const ProviderConfig &one) { return one.id == named; });
        if (asked != rows.end() && fit(*asked)) {
            return {&*asked, RecogniserChoice::Chosen};
        }
        return {nullptr, RecogniserChoice::NamedButUnusable};
    }

    const auto forPrompting = std::ranges::find_if(
        rows, [&fit](const ProviderConfig &one) { return one.isRealtimeDefault && fit(one); });
    if (forPrompting != rows.end()) {
        return {&*forPrompting, RecogniserChoice::Chosen};
    }

    const auto standing = std::ranges::find_if(
        rows, [&fit](const ProviderConfig &one) { return one.isDefault && fit(one); });
    if (standing != rows.end()) {
        return {&*standing, RecogniserChoice::Chosen};
    }

    const auto any = std::ranges::find_if(rows, fit);
    if (any != rows.end()) {
        return {&*any, RecogniserChoice::Chosen};
    }
    return {nullptr, RecogniserChoice::NothingUsable};
}

}
