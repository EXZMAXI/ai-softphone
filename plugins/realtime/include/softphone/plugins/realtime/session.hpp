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

#include <softphone/core/joining_thread.hpp>
#include <softphone/core/llm.hpp>
#include <softphone/core/realtime_purse.hpp>
#include <softphone/core/realtime_reader.hpp>
#include <softphone/core/realtime_ring.hpp>
#include <softphone/core/streaming_asr.hpp>
#include <softphone/core/templates.hpp>
#include <softphone/core/turn_engine.hpp>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace softphone::plugins::realtime {

class Session {
public:

    struct Listener {

        std::function<void()> listening;

        std::function<void(core::Voice, std::string, std::chrono::milliseconds)> partial;

        std::function<void(core::AsrTurn, std::size_t where)> turn;

        std::function<void(std::string answer, std::size_t from, std::size_t to,
                           std::chrono::milliseconds at)>
            answered;

        std::function<void(std::chrono::milliseconds at, std::chrono::milliseconds lost)> gap;

        std::function<void(std::int64_t recogniserMicros, std::int64_t modelMicros, bool estimated)>
            spent;

        std::function<void(std::string what, std::string said)> trouble;

        std::function<void(core::AsrResult)> finished;
    };

    struct Opening {

        core::PromptTemplate prompt;

        core::AsrEndpoint recogniser;

        core::RealtimeRing *farRing{nullptr};

        core::RealtimeRing *nearRing{nullptr};

        unsigned sourceRate{16000};

        core::TurnRules turns;

        core::RealtimeSpendSettings purse;

        std::int64_t recogniserMicrosThisMonth{0};
        std::int64_t modelMicrosThisMonth{0};

        std::string language;
        std::string into;

        std::string interface;

        std::chrono::milliseconds since{0};
    };

    struct Answered {
        bool worked{false};

        std::string text;

        std::int64_t costMicros{0};
    };

    using Ask =
        std::function<Answered(const core::RenderedPrompt &, const core::LlmCancel &cancelled)>;

    Session(Opening opening, std::unique_ptr<core::IStreamingAsr> recogniser, Ask ask,
            Listener listener);

    Session(const Session &) = delete;
    Session &operator=(const Session &) = delete;
    Session(Session &&) = delete;
    Session &operator=(Session &&) = delete;

    ~Session();

    void stop();

    void askNow();

    void forget();

    [[nodiscard]] bool running() const noexcept
    {
        return running_.load(std::memory_order_acquire);
    }

    [[nodiscard]] std::chrono::milliseconds listenedFor() const;
    [[nodiscard]] std::chrono::milliseconds lost() const;
    [[nodiscard]] core::RealtimeSpent spent() const;
    [[nodiscard]] std::size_t turns() const;
    [[nodiscard]] std::size_t asked() const;

    [[nodiscard]] std::vector<core::AsrTurn> replies() const;

    [[nodiscard]] std::vector<core::TurnReading> readings() const;

    static constexpr auto kReadEvery = std::chrono::milliseconds{50};

private:
    class Sink;

    void read();

    void flushTheReaders();

    unsigned wireRate_{16000};

    void conclude();
    void askTheModel();
    void heard(const core::AskedTurns &asked);

    Opening opening_;
    std::unique_ptr<core::IStreamingAsr> recogniser_;
    Ask ask_;
    Listener listener_;

    std::unique_ptr<Sink> sink_;

    std::unique_ptr<core::IStreamingAsrSession> stream_;

    std::unique_ptr<core::IStreamingAsrSession> ourStream_;

    std::unique_ptr<core::RealtimeReader> fromFar_;
    std::unique_ptr<core::RealtimeReader> fromNear_;

    core::RealtimePurse purse_;

    mutable std::mutex lock_;
    std::unique_ptr<core::TurnEngine> engine_;

    std::chrono::milliseconds listened_{0};
    std::chrono::milliseconds lost_{0};
    std::size_t asked_{0};

    std::int64_t measured_{0};

    std::optional<core::AskedTurns> waiting_;
    std::condition_variable wake_;

    std::atomic<int> streams_{0};
    std::atomic<int> ending_{0};

    std::atomic<bool> running_{false};
    std::atomic<bool> leaving_{false};
    std::atomic<bool> modelStopped_{false};

    core::JoiningThread reader_;
    core::JoiningThread asker_;
};

}
