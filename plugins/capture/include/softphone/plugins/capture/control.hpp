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

#include <softphone/capture/capture_session.hpp>
#include <softphone/capture/system_capture.hpp>

#include <softphone/core/audio_signal.hpp>
#include <softphone/core/recordings.hpp>

#include <chrono>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace softphone::plugins::capture {

namespace cap = softphone::capture;

enum class OnFinding {

    Never,

    Ask,

    Record,
};

[[nodiscard]] inline std::string_view asText(OnFinding answer)
{
    switch (answer) {
    case OnFinding::Never:
        return "never";
    case OnFinding::Ask:
        return "ask";
    case OnFinding::Record:
        return "record";
    }
    return "ask";
}

[[nodiscard]] inline std::optional<OnFinding> parseOnFinding(std::string_view word)
{
    if (word == "never") {
        return OnFinding::Never;
    }
    if (word == "ask") {
        return OnFinding::Ask;
    }
    if (word == "record") {
        return OnFinding::Record;
    }
    return std::nullopt;
}

struct ApplicationPolicy {
    std::string process;
    OnFinding answer{OnFinding::Ask};

    [[nodiscard]] bool operator==(const ApplicationPolicy &) const = default;
};

[[nodiscard]] OnFinding policyFor(OnFinding fallback,
                                  const std::vector<ApplicationPolicy> &exceptions,
                                  std::string_view process);

[[nodiscard]] std::vector<std::string>
neverReport(OnFinding fallback, const std::vector<ApplicationPolicy> &exceptions);

class ICaptureControl {
public:

    struct Settings {

        bool enabled{false};

        OnFinding onFinding{OnFinding::Ask};

        std::vector<ApplicationPolicy> perApplication;

        unsigned preRollSeconds{15};

        bool remind{true};

        std::string outputDevice;
        std::string inputDevice;

        std::uint32_t ownProcessId{0};
    };

    struct State {

        bool available{false};

        bool watching{false};

        bool enabled{false};
        bool running{false};

        cap::CaptureError problem{cap::CaptureError::None};

        bool heldOff{false};

        std::string what;

        std::chrono::milliseconds elapsed{0};

        std::string recordingId;
    };

    struct Levels {
        unsigned near{0};
        unsigned far{0};
    };

    using StateCallback = std::function<void()>;

    virtual ~ICaptureControl() = default;

    ICaptureControl() = default;
    ICaptureControl(const ICaptureControl &) = delete;
    ICaptureControl &operator=(const ICaptureControl &) = delete;
    ICaptureControl(ICaptureControl &&) = delete;
    ICaptureControl &operator=(ICaptureControl &&) = delete;

    virtual void setSettings(const Settings &settings) = 0;
    [[nodiscard]] virtual Settings settings() const = 0;

    [[nodiscard]] virtual core::RecordingFeed *listenTo() = 0;

    virtual void setListener(StateCallback listener) = 0;

    virtual void watchWith(const cap::Endings &when) = 0;

    using ExcludedCallback = std::function<void(std::vector<ApplicationPolicy>)>;
    virtual void setExcluded(ExcludedCallback excluded) = 0;

    using FoundCallback = std::function<void(std::string what, std::string process)>;
    virtual void setFound(FoundCallback found) = 0;

    using LostCallback = std::function<void()>;
    virtual void setLost(LostCallback lost) = 0;

    using PhraseSource = std::function<std::string()>;
    virtual void setMeetingPhrase(PhraseSource words) = 0;

    using WrittenCallback = std::function<void(const core::Recording &)>;
    virtual void setWritten(WrittenCallback written) = 0;

    [[nodiscard]] virtual cap::CaptureError start(std::string what, std::string process = {}) = 0;

    [[nodiscard]] virtual bool notACall(const std::string &recordingId) = 0;

    virtual bool rename(std::string what) = 0;

    virtual void stop() = 0;

    [[nodiscard]] virtual State state() const = 0;

    [[nodiscard]] virtual Levels takeLevels() = 0;

    [[nodiscard]] virtual core::ISignalTaps &drawings() noexcept = 0;

    virtual void suppress(bool suppressed) = 0;

    [[nodiscard]] virtual bool suppressed() const = 0;

    virtual void yieldTheSoundCard(bool wanted) = 0;
};

}
