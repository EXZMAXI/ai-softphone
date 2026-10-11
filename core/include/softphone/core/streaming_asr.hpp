// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#pragma once

#include <softphone/core/asr.hpp>
#include <softphone/core/providers.hpp>
#include <softphone/core/recordings.hpp>

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace softphone::core {

enum class AsrEncoding {

    Pcm16,

    Mulaw,

    Opus,
};

[[nodiscard]] std::string_view toString(AsrEncoding encoding) noexcept;

struct AsrWireFormat {
    AsrEncoding encoding{AsrEncoding::Pcm16};
    unsigned sampleRate{16000};

    unsigned channels{1};

    [[nodiscard]] bool operator==(const AsrWireFormat &) const = default;
};

struct StreamingCapabilities {

    bool marksTurns{false};

    bool takesOurTurnMark{false};

    bool translates{false};

    bool interim{true};

    bool languageHint{true};

    bool detectsLanguage{false};

    std::vector<std::string> languages;

    std::vector<AsrWireFormat> accepts{AsrWireFormat{}};

    bool reportsCost{false};

    std::chrono::seconds idleCloses{0};

    bool diarisation{false};

    bool revisesTurns{false};

    std::chrono::milliseconds leastPerMessage{0};
};

struct AsrPartial {

    Voice voice{Voice::Far};

    std::chrono::milliseconds start{0};
    std::chrono::milliseconds end{0};

    std::string text;

    std::vector<AsrWord> words;
};

struct AsrTurn {
    Voice voice{Voice::Far};

    std::chrono::milliseconds start{0};
    std::chrono::milliseconds end{0};

    std::string text;
    std::vector<AsrWord> words;
    double confidence{0.0};

    std::string language;

    std::string translation;

    enum class Boundary {

        Provider,

        Silence,

        Ours,
    };
    Boundary boundary{Boundary::Provider};

    std::optional<unsigned> ordinal;
};

class IStreamingAsrSink {
public:
    IStreamingAsrSink() = default;
    IStreamingAsrSink(const IStreamingAsrSink &) = delete;
    IStreamingAsrSink &operator=(const IStreamingAsrSink &) = delete;
    IStreamingAsrSink(IStreamingAsrSink &&) = delete;
    IStreamingAsrSink &operator=(IStreamingAsrSink &&) = delete;
    virtual ~IStreamingAsrSink() = default;

    virtual void listening() = 0;

    virtual void partial(const AsrPartial &said) = 0;

    virtual void turn(const AsrTurn &said) = 0;

    virtual void gap(std::chrono::milliseconds at, std::chrono::milliseconds lost) = 0;

    virtual void spent(std::int64_t costMicros) = 0;

    virtual void finished(const AsrResult &result) = 0;
};

struct StreamingOptions {

    std::string language;

    std::string into;

    Voice voice{Voice::Far};

    AsrWireFormat wire;

    bool wordTimestamps{true};

    bool diarise{false};

    std::vector<std::pair<std::string, std::string>> params;

    std::chrono::milliseconds endOfTurnSilence{0};
};

class IStreamingAsrSession {
public:
    IStreamingAsrSession() = default;
    IStreamingAsrSession(const IStreamingAsrSession &) = delete;
    IStreamingAsrSession &operator=(const IStreamingAsrSession &) = delete;
    IStreamingAsrSession(IStreamingAsrSession &&) = delete;
    IStreamingAsrSession &operator=(IStreamingAsrSession &&) = delete;

    virtual ~IStreamingAsrSession() = default;

    [[nodiscard]] virtual bool send(std::chrono::milliseconds position,
                                    std::span<const std::int16_t> samples) = 0;

    virtual void drain() = 0;

    virtual void turnEnded() {}

    [[nodiscard]] virtual bool listening() const = 0;
};

class IStreamingAsr {
public:
    IStreamingAsr() = default;
    IStreamingAsr(const IStreamingAsr &) = delete;
    IStreamingAsr &operator=(const IStreamingAsr &) = delete;
    IStreamingAsr(IStreamingAsr &&) = delete;
    IStreamingAsr &operator=(IStreamingAsr &&) = delete;
    virtual ~IStreamingAsr() = default;

    [[nodiscard]] virtual std::string_view type() const noexcept = 0;

    [[nodiscard]] virtual StreamingCapabilities capabilities(const AsrEndpoint &where) const = 0;

    [[nodiscard]] virtual std::unique_ptr<IStreamingAsrSession>
    open(const AsrEndpoint &where, const StreamingOptions &options, IStreamingAsrSink &sink,
         const AsrCancel &cancelled) = 0;

    [[nodiscard]] virtual AsrProbe probe(const AsrEndpoint &where) = 0;
};

enum class PromptingRefused {

    Nothing,

    NobodyWasTold,
};

[[nodiscard]] std::string_view toString(PromptingRefused why) noexcept;

[[nodiscard]] PromptingRefused promptingAllowed(ConsentPosture posture, bool announced) noexcept;

[[nodiscard]] std::optional<AsrWireFormat> wireFor(const StreamingCapabilities &what,
                                                   unsigned have) noexcept;

[[nodiscard]] const std::vector<std::string> &streamingTypes();

[[nodiscard]] bool canStreamType(std::string_view type);

[[nodiscard]] bool runsOnThisMachine(std::string_view type);

[[nodiscard]] const std::vector<std::string> &transcribingTypes();

[[nodiscard]] bool canTranscribeType(std::string_view type);

[[nodiscard]] std::string_view liveAddressFor(std::string_view type) noexcept;

[[nodiscard]] std::string_view liveModelFor(std::string_view type) noexcept;

[[nodiscard]] std::vector<std::string> knownModels(std::string_view type, bool live);

[[nodiscard]] std::string preferredModelFor(std::string_view type, bool live);

struct LiveRegion {

    std::string_view code;

    std::string_view address;

    std::string_view territories;
};

[[nodiscard]] std::vector<LiveRegion> liveRegionsFor(std::string_view type);

[[nodiscard]] std::string_view liveAddressHere(std::string_view type, std::string_view territory);

enum class RecogniserChoice {

    Chosen,

    NamedButUnusable,

    NothingUsable,
};

struct ChosenRecogniser {
    const ProviderConfig *row{nullptr};
    RecogniserChoice why{RecogniserChoice::NothingUsable};
};

[[nodiscard]] ChosenRecogniser
recogniserForPrompter(const std::vector<ProviderConfig> &rows, std::string_view named,
                      const std::function<bool(const ProviderConfig &)> &usable);

}
