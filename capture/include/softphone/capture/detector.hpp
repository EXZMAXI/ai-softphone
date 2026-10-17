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

#include <softphone/capture/system_capture.hpp>

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace softphone::capture {

struct Conversation {

    std::string application;

    std::string title;

    [[nodiscard]] bool operator==(const Conversation &) const = default;
};

struct DetectorSettings {

    std::chrono::milliseconds settle{std::chrono::seconds{3}};

    std::chrono::milliseconds holdOff{std::chrono::seconds{6}};

    bool conversing{false};

    std::vector<std::string> excluded;

    std::uint32_t ownProcessId{0};
};

class ConversationGate {
public:
    explicit ConversationGate(DetectorSettings settings);

    enum class Move {
        Nothing,
        Began,
        Ended,
    };

    Move saw(bool held, bool duplex, bool audible, std::chrono::steady_clock::time_point now);

    [[nodiscard]] bool talking() const noexcept
    {
        return talking_;
    }

    void setSettings(DetectorSettings settings);

private:
    DetectorSettings settings_;
    bool talking_{false};

    bool heard_{false};

    std::chrono::steady_clock::time_point since_{};
    bool pending_{false};
};

class IConversationDetector {
public:
    using Began = std::function<void(const Conversation &)>;
    using Ended = std::function<void()>;

    virtual ~IConversationDetector() = default;

    IConversationDetector() = default;
    IConversationDetector(const IConversationDetector &) = delete;
    IConversationDetector &operator=(const IConversationDetector &) = delete;
    IConversationDetector(IConversationDetector &&) = delete;
    IConversationDetector &operator=(IConversationDetector &&) = delete;

    [[nodiscard]] virtual CaptureError start(const DetectorSettings &settings, Began began,
                                             Ended ended) = 0;
    virtual void stop() = 0;

    [[nodiscard]] virtual bool talking() const = 0;

    [[nodiscard]] virtual bool available() const = 0;

    [[nodiscard]] virtual std::string trail() const = 0;
};

[[nodiscard]] std::unique_ptr<IConversationDetector> makeConversationDetector();

}
