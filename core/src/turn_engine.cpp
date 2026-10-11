// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/turn_engine.hpp>

#include <softphone/core/logging.hpp>
#include <softphone/core/text_search.hpp>

#include <algorithm>
#include <utility>

namespace softphone::core {
namespace {

constexpr std::size_t kShortestWithoutSpaces = 12;

[[nodiscard]] std::string bareWords(std::string_view text)
{
    const auto folded = casefolded(text);

    std::string out;
    out.reserve(folded.size());
    bool space = true;
    for (const auto letter : folded) {
        const auto raw = static_cast<unsigned char>(letter);
        if (raw < 0x80 && (std::isspace(raw) != 0 || std::ispunct(raw) != 0)) {
            if (!space) {
                out += ' ';
                space = true;
            }
            continue;
        }
        out += letter;
        space = false;
    }
    while (!out.empty() && out.back() == ' ') {
        out.pop_back();
    }
    return out;
}

[[nodiscard]] std::string joined(const std::vector<AsrTurn> &turns, std::string_view language,
                                 bool withSpeakers = false)
{
    std::string out;
    for (const auto &one : turns) {
        if (one.text.empty()) {
            continue;
        }
        if (!out.empty()) {
            out += "\n\n";
        }
        if (withSpeakers) {
            out += std::string{speakerWord(one.voice, language)} + ": ";
        }
        out += one.text;
    }
    return out;
}

}

std::string_view toString(Backlog backlog) noexcept
{
    switch (backlog) {
    case Backlog::Queue:
        return "queue";
    case Backlog::Newest:
        return "newest";
    case Backlog::Coalesce:
        break;
    }
    return "coalesce";
}

std::optional<Backlog> parseBacklog(std::string_view text) noexcept
{
    if (text == "queue") {
        return Backlog::Queue;
    }
    if (text == "newest") {
        return Backlog::Newest;
    }
    if (text == "coalesce") {
        return Backlog::Coalesce;
    }
    return std::nullopt;
}

std::size_t wordsIn(std::string_view text) noexcept
{
    std::size_t words = 0;
    bool inside = false;
    for (const auto letter : text) {
        const bool space = letter == ' ' || letter == '\t' || letter == '\n' || letter == '\r';
        if (space) {
            inside = false;
        } else if (!inside) {
            inside = true;
            ++words;
        }
    }
    return words;
}

bool longEnoughToAsk(std::string_view text, std::size_t shortestReply) noexcept
{
    if (wordsIn(text) >= shortestReply) {
        return true;
    }

    if (text.find(' ') != std::string_view::npos) {
        return false;
    }
    return text.size() >= kShortestWithoutSpaces;
}

std::string TurnReading::said() const
{
    const auto verdictName = [this] {
        switch (verdict) {
        case Verdict::TooShort:
            return "too short to ask about";
        case Verdict::NoModel:
            return "no model on this template";
        case Verdict::Pending:
            return "pending, the model is busy";
        case Verdict::Revised:
            return "revised a reply already closed";
        case Verdict::OurSide:
            return "our own side, kept as context";
        case Verdict::Echo:
            return "our own side repeating the other party: the speakers, not a person";
        case Verdict::Waiting:
            return "short, waiting to see whether the next one belongs with it";
        case Verdict::Joined:
            return "joined to the short reply that was waiting";
        case Verdict::Marked:
            return "a boundary of ours put on the wire; the reply comes from the far end";
        case Verdict::Copied:
            return "the far end's copy of a reply already kept";
        case Verdict::Asked:
            break;
        }
        return "asked";
    }();

    const auto boundaryName = [this] {
        switch (boundary) {
        case AsrTurn::Boundary::Silence:
            return "our silence";
        case AsrTurn::Boundary::Ours:
            return "asked for";
        case AsrTurn::Boundary::Provider:
            break;
        }
        return "the recogniser";
    }();

    return std::string{"at "} + std::to_string(at.count()) + " ms, " + boundaryName + " ended a " +
           std::to_string(words) + "-word reply after " + std::to_string(quietFor.count()) +
           " ms quiet: " + verdictName +
           (pending > 0 ? " (" + std::to_string(pending) + " waiting)" : std::string{});
}

TurnEngine::TurnEngine(TurnRules rules, Ask ask, Kept kept, Marked marked)
    : rules_{rules}, ask_{std::move(ask)}, kept_{std::move(kept)}, marked_{std::move(marked)}
{}

bool TurnEngine::watchingForSilence() const noexcept
{
    switch (rules_.policy) {
    case TurnPolicy::Manual:
        return false;
    case TurnPolicy::Silence:
        return true;
    case TurnPolicy::Provider:

        return !rules_.providerMarksTurns;
    }
    return false;
}

TurnEngine::Side &TurnEngine::sideOf(Voice voice) noexcept
{
    return voice == Voice::Near ? ours_ : theirs_;
}

bool TurnEngine::soundsLikeEcho(const AsrTurn &mine) const
{

    const auto mineBare = bareWords(mine.text);
    if (mineBare.empty() || wordsIn(mineBare) < kEchoFloor) {
        return false;
    }

    for (auto one = replies_.rbegin(); one != replies_.rend(); ++one) {
        if (one->voice != Voice::Far) {
            continue;
        }
        if (mine.end > one->end && mine.end - one->end > kEchoWindow) {

            break;
        }

        const auto theirsBare = bareWords(one->text);
        if (theirsBare.empty()) {
            continue;
        }
        if (mineBare == theirsBare || mineBare.find(theirsBare) != std::string::npos ||
            theirsBare.find(mineBare) != std::string::npos) {
            return true;
        }
    }
    return false;
}

void TurnEngine::heard(const AsrPartial &said)
{
    clock_ = std::max(clock_, said.end);

    const auto line = tidiedLine(said.text);
    if (line.empty()) {
        return;
    }

    Side &side = sideOf(said.voice);

    if (line != side.provisional) {
        if (side.provisional.empty()) {
            side.started = said.start;
        }
        side.provisional = line;
        side.lastWord = clock_;
    }
}

void TurnEngine::heard(const AsrTurn &said)
{
    clock_ = std::max(clock_, said.end);

    auto line = said;
    line.text = tidiedLine(line.text);

    Side &side = sideOf(line.voice);

    if (line.text.empty()) {
        side.lastWord = clock_;
        return;
    }

    if (side.awaitingCopy && line.boundary == AsrTurn::Boundary::Provider) {
        side.awaitingCopy = false;
        side.marks = 0;

        TurnReading copy;
        copy.verdict = TurnReading::Verdict::Copied;
        copy.boundary = line.boundary;
        copy.at = line.end;
        copy.words = wordsIn(line.text);
        copy.quietFor = std::chrono::milliseconds::zero();
        copy.pending = pending_.size();
        decided(copy);
        return;
    }

    side.provisional.clear();
    side.marks = 0;
    close(line, line.end);
    side.lastWord = clock_;
}

std::chrono::milliseconds TurnEngine::quietEndsIt() const noexcept
{

    if (rules_.providerTakesOurBoundary &&
        rules_.providerIdle > std::chrono::milliseconds::zero()) {
        return std::max(rules_.silence, rules_.providerIdle);
    }
    return rules_.silence;
}

void TurnEngine::now(std::chrono::milliseconds when)
{
    clock_ = std::max(clock_, when);

    if (joining()) {
        for (const auto voice : {Voice::Far, Voice::Near}) {
            Side &side = sideOf(voice);
            if (!side.waiting.text.empty() && clock_ - side.waitingSince >= rules_.join) {
                releaseWaiting(voice, clock_);
            }
        }
    }

    if (!watchingForSilence()) {

        if (rules_.policy != TurnPolicy::Manual &&
            rules_.providerIdle > std::chrono::milliseconds::zero()) {
            for (const auto voice : {Voice::Far, Voice::Near}) {
                Side &side = sideOf(voice);
                if (side.provisional.empty() || clock_ - side.lastWord < rules_.providerIdle) {
                    continue;
                }
                AsrTurn said;
                said.voice = voice;
                said.start = side.started;
                said.end = side.lastWord;
                said.text = side.provisional;
                said.boundary = AsrTurn::Boundary::Silence;

                side.provisional.clear();
                close(said, clock_);
                side.lastWord = clock_;
            }
        }
        return;
    }

    const auto quiet = quietEndsIt();
    for (const auto voice : {Voice::Far, Voice::Near}) {
        Side &side = sideOf(voice);
        if (side.provisional.empty() || clock_ - side.lastWord < quiet) {
            continue;
        }

        if (rules_.providerTakesOurBoundary && side.marks == 0) {
            TurnReading mark;
            mark.verdict = TurnReading::Verdict::Marked;
            mark.boundary = AsrTurn::Boundary::Silence;
            mark.at = clock_;
            mark.words = wordsIn(side.provisional);
            mark.quietFor = clock_ - side.lastWord;
            mark.pending = pending_.size();
            decided(mark);

            ++side.marks;
            side.lastWord = clock_;
            if (marked_) {
                marked_(voice);
            }
            continue;
        }

        AsrTurn said;
        said.voice = voice;
        said.start = side.started;
        said.end = side.lastWord;
        said.text = side.provisional;
        said.boundary = AsrTurn::Boundary::Silence;
        side.provisional.clear();

        side.marks = 0;
        side.awaitingCopy = rules_.providerTakesOurBoundary;
        close(said, clock_);
        side.lastWord = clock_;
    }
}

void TurnEngine::askNow()
{

    releaseEverything(clock_);

    if (!ours_.provisional.empty()) {
        AsrTurn mine;
        mine.voice = Voice::Near;
        mine.start = ours_.started;
        mine.end = clock_;
        mine.text = ours_.provisional;
        mine.boundary = AsrTurn::Boundary::Ours;
        ours_.provisional.clear();

        ours_.marks = 0;
        ours_.awaitingCopy = rules_.providerTakesOurBoundary;
        close(mine, clock_, true);
        ours_.lastWord = clock_;
    }

    if (theirs_.provisional.empty()) {

        dispatch();
        return;
    }

    AsrTurn said;
    said.voice = Voice::Far;
    said.start = theirs_.started;
    said.end = clock_;
    said.text = theirs_.provisional;
    said.boundary = AsrTurn::Boundary::Ours;
    theirs_.provisional.clear();
    theirs_.marks = 0;
    theirs_.awaitingCopy = rules_.providerTakesOurBoundary;
    close(said, clock_, true);
    theirs_.lastWord = clock_;
}

void TurnEngine::forget()
{
    replies_.clear();
    readings_.clear();

    theirs_ = Side{};
    ours_ = Side{};

}

namespace {

[[nodiscard]] std::string_view nameOf(TurnReading::Verdict verdict) noexcept
{
    switch (verdict) {
    case TurnReading::Verdict::Asked:
        return "asked";
    case TurnReading::Verdict::NoModel:
        return "no-model";
    case TurnReading::Verdict::Pending:
        return "pending";
    case TurnReading::Verdict::Revised:
        return "revised";
    case TurnReading::Verdict::TooShort:
        return "too-short";
    case TurnReading::Verdict::OurSide:
        return "our-side";
    case TurnReading::Verdict::Echo:
        return "echo";
    case TurnReading::Verdict::Waiting:
        return "waiting";
    case TurnReading::Verdict::Joined:
        return "joined";
    case TurnReading::Verdict::Marked:
        return "marked";
    case TurnReading::Verdict::Copied:
        return "copied";
    }
    return "unknown";
}

[[nodiscard]] std::string_view nameOf(AsrTurn::Boundary boundary) noexcept
{
    switch (boundary) {
    case AsrTurn::Boundary::Provider:
        return "provider";
    case AsrTurn::Boundary::Silence:
        return "silence";
    case AsrTurn::Boundary::Ours:
        return "ours";
    }
    return "unknown";
}

}

void TurnEngine::decided(const TurnReading &reading)
{
    readings_.push_back(reading);

    const auto at = std::to_string(reading.at.count());
    const auto quiet = std::to_string(reading.quietFor.count());
    const auto words = std::to_string(reading.words);
    const auto pending = std::to_string(reading.pending);
    const LogAttribute fields[] = {
        {"verdict", nameOf(reading.verdict)},
        {"boundary", nameOf(reading.boundary)},
        {"at_ms", at},
        {"quiet_ms", quiet},
        {"words", words},
        {"pending", pending},
    };
    Logger::instance().log(LogLevel::Trace, "realtime", "a reply closed", fields);
}

bool TurnEngine::joining() const noexcept
{
    return rules_.join > std::chrono::milliseconds::zero();
}

void TurnEngine::releaseWaiting(Voice voice, std::chrono::milliseconds at)
{
    Side &side = sideOf(voice);
    if (side.waiting.text.empty()) {
        return;
    }
    const auto held = side.waiting;
    side.waiting = AsrTurn{};
    settle(held, at, false);
}

void TurnEngine::releaseEverything(std::chrono::milliseconds at)
{

    releaseWaiting(Voice::Far, at);
    releaseWaiting(Voice::Near, at);
}

void TurnEngine::close(const AsrTurn &said, std::chrono::milliseconds at, bool askedFor)
{

    if (askedFor || !joining()) {
        releaseWaiting(said.voice, at);
        settle(said, at, askedFor);
        return;
    }

    Side &waiting = sideOf(said.voice);

    if (!waiting.waiting.text.empty() && clock_ - waiting.waitingSince >= rules_.join) {
        releaseWaiting(said.voice, at);
    }

    if (!waiting.waiting.text.empty()) {

        TurnReading joined;
        joined.verdict = TurnReading::Verdict::Joined;
        joined.boundary = said.boundary;
        joined.at = at;
        joined.words = wordsIn(waiting.waiting.text) + wordsIn(said.text);
        joined.quietFor = clock_ > waiting.waitingSince ? clock_ - waiting.waitingSince
                                                        : std::chrono::milliseconds::zero();
        joined.pending = pending_.size();
        decided(joined);

        AsrTurn one = said;
        one.start = waiting.waiting.start;
        one.text = waiting.waiting.text + " " + said.text;

        one.ordinal.reset();
        waiting.waiting = AsrTurn{};

        if (wordsIn(one.text) >= rules_.joinShortestWords) {
            settle(one, at, false);
            return;
        }
        waiting.waiting = one;
        waiting.waitingSince = clock_;
        return;
    }

    if (wordsIn(said.text) >= rules_.joinShortestWords) {
        settle(said, at, false);
        return;
    }

    TurnReading held;
    held.verdict = TurnReading::Verdict::Waiting;
    held.boundary = said.boundary;
    held.at = at;
    held.words = wordsIn(said.text);
    held.quietFor = std::chrono::milliseconds::zero();
    held.pending = pending_.size();
    decided(held);

    waiting.waiting = said;
    waiting.waitingSince = clock_;
}

void TurnEngine::settle(const AsrTurn &said, std::chrono::milliseconds at, bool askedFor)
{
    Side &side = sideOf(said.voice);
    const auto quietFor =
        clock_ > side.lastWord ? clock_ - side.lastWord : std::chrono::milliseconds::zero();

    TurnReading reading;
    reading.boundary = said.boundary;
    reading.at = at;
    reading.words = wordsIn(said.text);
    reading.quietFor = quietFor;
    reading.pending = pending_.size();

    if (said.ordinal.has_value()) {
        const auto already = std::ranges::find_if(replies_, [&said](const AsrTurn &had) {
            return had.ordinal == said.ordinal && had.voice == said.voice;
        });
        if (already != replies_.end()) {
            *already = said;
            reading.verdict = TurnReading::Verdict::Revised;
            decided(reading);

            const auto waiting = std::ranges::find_if(pending_, [&said](const AsrTurn &had) {
                return had.ordinal == said.ordinal && had.voice == said.voice;
            });
            if (waiting != pending_.end()) {
                *waiting = said;
            }
            return;
        }
    }

    if (said.voice == Voice::Near && soundsLikeEcho(said)) {
        reading.verdict = TurnReading::Verdict::Echo;
        decided(reading);
        return;
    }

    replies_.push_back(said);
    const auto where = replies_.size() - 1;

    if (kept_) {
        kept_(said, where);
    }

    if (said.voice == Voice::Near) {
        reading.verdict = TurnReading::Verdict::OurSide;
        decided(reading);
        return;
    }

    if (!rules_.asksAModel) {
        reading.verdict = TurnReading::Verdict::NoModel;
        decided(reading);
        return;
    }

    if (rules_.policy == TurnPolicy::Manual && !askedFor) {
        pending_.push_back(said);
        pendingAt_.push_back(where);
        reading.verdict = TurnReading::Verdict::Pending;
        reading.pending = pending_.size();
        decided(reading);
        return;
    }

    if (!askedFor && !longEnoughToAsk(said.text, rules_.shortestReply)) {
        reading.verdict = TurnReading::Verdict::TooShort;
        decided(reading);
        return;
    }

    pending_.push_back(said);
    pendingAt_.push_back(where);

    if (asking_) {
        reading.verdict = TurnReading::Verdict::Pending;
        reading.pending = pending_.size();
        decided(reading);
        return;
    }

    reading.verdict = TurnReading::Verdict::Asked;
    decided(reading);
    dispatch();
}

void TurnEngine::answered()
{
    asking_ = false;
    dispatch();
}

void TurnEngine::dispatch()
{
    if (asking_ || pending_.empty() || !ask_) {
        return;
    }

    AskedTurns asked;

    switch (rules_.backlog) {
    case Backlog::Queue:

        asked.covers.push_back(pending_.front());
        asked.at.push_back(pendingAt_.front());
        pending_.erase(pending_.begin());
        pendingAt_.erase(pendingAt_.begin());
        break;

    case Backlog::Newest:

        asked.covers.push_back(pending_.back());
        asked.at.push_back(pendingAt_.back());
        pending_.clear();
        pendingAt_.clear();
        break;

    case Backlog::Coalesce:
        asked.covers = pending_;
        asked.at = pendingAt_;
        pending_.clear();
        pendingAt_.clear();
        break;
    }

    asked.reply = joined(asked.covers, rules_.speakerLanguage);
    asked.conversation = contextBefore(asked.covers.size());

    if (!readings_.empty()) {
        asked.reading = readings_.back();
    }

    asking_ = true;
    ask_(asked);
}

std::string TurnEngine::contextBefore(std::size_t howManyToSkip) const
{

    if (replies_.size() <= howManyToSkip) {
        return {};
    }

    const auto before = replies_.size() - howManyToSkip;
    const auto from = before > rules_.repliesOfContext ? before - rules_.repliesOfContext : 0;

    std::vector<AsrTurn> some(replies_.begin() + static_cast<std::ptrdiff_t>(from),
                              replies_.begin() + static_cast<std::ptrdiff_t>(before));

    const auto bothSides =
        std::ranges::any_of(some, [](const AsrTurn &one) { return one.voice == Voice::Near; });
    return joined(some, rules_.speakerLanguage, bothSides);
}

}
