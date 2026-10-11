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

#include <softphone/core/streaming_asr.hpp>
#include <softphone/core/templates.hpp>

#include <chrono>
#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace softphone::core {

enum class Backlog {

    Queue,

    Newest,

    Coalesce,
};

[[nodiscard]] std::string_view toString(Backlog backlog) noexcept;
[[nodiscard]] std::optional<Backlog> parseBacklog(std::string_view text) noexcept;

inline constexpr auto kSilenceEndsAReply = std::chrono::milliseconds{700};

inline constexpr std::size_t kShortestReply = 3;

inline constexpr std::size_t kRepliesOfContext = 6;

inline constexpr auto kProviderIdle = std::chrono::milliseconds{2'500};

inline constexpr auto kJoinWithin = std::chrono::milliseconds{900};

inline constexpr std::size_t kJoinShortest = 5;

struct TurnRules {

    TurnPolicy policy{TurnPolicy::Provider};

    bool providerMarksTurns{false};

    bool providerTakesOurBoundary{false};

    std::chrono::milliseconds silence{kSilenceEndsAReply};
    std::size_t shortestReply{kShortestReply};
    std::size_t repliesOfContext{kRepliesOfContext};

    std::chrono::milliseconds providerIdle{kProviderIdle};

    std::chrono::milliseconds join{kJoinWithin};

    std::size_t joinShortestWords{kJoinShortest};

    Backlog backlog{Backlog::Coalesce};

    bool asksAModel{true};

    std::string speakerLanguage;
};

struct TurnReading {

    enum class Verdict {

        Asked,

        TooShort,

        NoModel,

        Pending,

        Revised,

        OurSide,

        Echo,

        Waiting,

        Joined,

        Marked,

        Copied,
    };
    Verdict verdict{Verdict::Asked};

    AsrTurn::Boundary boundary{AsrTurn::Boundary::Provider};

    std::chrono::milliseconds at{0};

    std::size_t words{0};
    std::chrono::milliseconds quietFor{0};

    std::size_t pending{0};

    [[nodiscard]] std::string said() const;
};

struct AskedTurns {

    std::vector<AsrTurn> covers;

    std::string reply;

    std::string conversation;

    std::vector<std::size_t> at;

    TurnReading reading;
};

class TurnEngine {
public:
    using Ask = std::function<void(const AskedTurns &)>;

    using Kept = std::function<void(const AsrTurn &said, std::size_t where)>;

    using Marked = std::function<void(Voice voice)>;

    TurnEngine(TurnRules rules, Ask ask, Kept kept = {}, Marked marked = {});

    void heard(const AsrPartial &said);

    void heard(const AsrTurn &said);

    void now(std::chrono::milliseconds when);

    void answered();

    void askNow();

    [[nodiscard]] const std::vector<AsrTurn> &replies() const noexcept
    {
        return replies_;
    }

    [[nodiscard]] const std::vector<TurnReading> &readings() const noexcept
    {
        return readings_;
    }

    [[nodiscard]] bool asking() const noexcept
    {
        return asking_;
    }

    [[nodiscard]] std::size_t pending() const noexcept
    {
        return pending_.size();
    }

    [[nodiscard]] bool watchingForSilence() const noexcept;

    static constexpr auto kEchoWindow = std::chrono::milliseconds{10'000};

    static constexpr std::size_t kEchoFloor = 2;

    void forget();

private:

    struct Side {
        std::string provisional;
        std::chrono::milliseconds lastWord{0};
        std::chrono::milliseconds started{0};

        AsrTurn waiting;

        std::chrono::milliseconds waitingSince{0};

        std::size_t marks{0};

        bool awaitingCopy{false};
    };

    [[nodiscard]] Side &sideOf(Voice voice) noexcept;

    [[nodiscard]] bool soundsLikeEcho(const AsrTurn &mine) const;

    void close(const AsrTurn &said, std::chrono::milliseconds at, bool askedFor = false);

    void settle(const AsrTurn &said, std::chrono::milliseconds at, bool askedFor);

    [[nodiscard]] bool joining() const noexcept;

    [[nodiscard]] std::chrono::milliseconds quietEndsIt() const noexcept;

    void releaseWaiting(Voice voice, std::chrono::milliseconds at);

    void releaseEverything(std::chrono::milliseconds at);
    void dispatch();
    [[nodiscard]] std::string contextBefore(std::size_t howManyToSkip) const;

    TurnRules rules_;
    Ask ask_;
    Kept kept_;
    Marked marked_;

    std::vector<AsrTurn> replies_;
    std::vector<TurnReading> readings_;

    void decided(const TurnReading &reading);

    std::vector<AsrTurn> pending_;
    std::vector<std::size_t> pendingAt_;

    bool asking_{false};

    Side theirs_;
    Side ours_;

    std::chrono::milliseconds clock_{0};
};

[[nodiscard]] std::size_t wordsIn(std::string_view text) noexcept;

[[nodiscard]] bool longEnoughToAsk(std::string_view text, std::size_t shortestReply) noexcept;

}
