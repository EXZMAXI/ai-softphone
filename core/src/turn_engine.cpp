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
        case AsrTurn::Boundary::Overlong:
            return "a line past being a phrase";
        case AsrTurn::Boundary::Provider:
            break;
        }
        return "the recogniser";
    }();

    return std::string{"at "} + std::to_string(at.count()) + " ms, " + boundaryName + " ended a " +
           std::to_string(words) + "-word " + voiceText(voice) + " reply of " +
           std::to_string(from.count()) + ".." + std::to_string(to.count()) + " ms after " +
           std::to_string(quietFor.count()) + " ms quiet: " + verdictName +
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

bool TurnEngine::otherSideTookOver(Voice voice) const noexcept
{
    const Side &mine = voice == Voice::Near ? ours_ : theirs_;
    const Side &theirs = voice == Voice::Near ? theirs_ : ours_;

    return theirs.lastWord > mine.lastWord;
}

std::string_view TurnEngine::whatIsNewIn(Side &side, std::string_view line)
{
    if (side.handedOut.empty()) {
        return line;
    }

    if (!line.starts_with(side.handedOut)) {

        const LogAttribute fields[] = {{"drew", side.handedOut}, {"now", line}};
        Logger::instance().log(LogLevel::Debug, "realtime",
                               "the line stopped continuing what a boundary of ours drew, so "
                               "that subtraction is forgotten",
                               fields);
        side.handedOut.clear();
        return line;
    }

    auto rest = line.substr(side.handedOut.size());
    while (!rest.empty() && (rest.front() == ' ' || rest.front() == '\t')) {
        rest.remove_prefix(1);
    }
    return rest;
}

void TurnEngine::tookItOutOfTheLine(Side &side, const AsrTurn &said)
{
    if (!said.text.empty()) {
        side.handedOut = side.handedOut.empty() ? said.text : side.handedOut + " " + said.text;
        side.handedAt = said.end;
    }
    side.provisional.clear();
}

void TurnEngine::takeWhatIsOwed(Side &side, AsrTurn &said)
{
    if (side.owed.empty()) {
        return;
    }
    said.text = said.text.empty() ? side.owed : side.owed + " " + said.text;
    said.start = side.owedFrom;

    said.ordinal.reset();
    side.owed.clear();
}

void TurnEngine::giveBackWhatWasHeld(Side &side, Voice voice)
{
    if (side.waiting.text.empty() || clock_ - side.waitingSince >= rules_.join) {
        return;
    }

    TurnReading joined;
    joined.verdict = TurnReading::Verdict::Joined;
    joined.boundary = side.waiting.boundary;
    joined.voice = voice;
    joined.from = side.waiting.start;
    joined.to = side.started;
    joined.at = clock_;
    joined.words = wordsIn(side.waiting.text);
    joined.quietFor =
        clock_ > side.waitingSince ? clock_ - side.waitingSince : std::chrono::milliseconds::zero();
    joined.pending = pending_.size();
    decided(joined);

    side.owed = side.owed.empty() ? side.waiting.text : side.owed + " " + side.waiting.text;
    side.owedFrom = side.waiting.start;
    side.waiting = AsrTurn{};
}

std::chrono::milliseconds TurnEngine::lineBeganAt(Voice voice) const noexcept
{
    const Side &side = voice == Voice::Near ? ours_ : theirs_;
    if (side.provisional.empty() && side.owed.empty()) {
        return std::chrono::milliseconds::zero();
    }
    return side.owed.empty() ? side.started : side.owedFrom;
}

std::string TurnEngine::lineInHand(Voice voice) const
{
    const Side &side = voice == Voice::Near ? ours_ : theirs_;
    if (side.owed.empty()) {
        return side.provisional;
    }
    return side.provisional.empty() ? side.owed : side.owed + " " + side.provisional;
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
        if (theirsBare.empty() || wordsIn(theirsBare) < kEchoFloor) {
            continue;
        }
        if (mineBare == theirsBare) {
            return true;
        }
        const auto shorter = std::min(mineBare.size(), theirsBare.size());
        const auto longer = std::max(mineBare.size(), theirsBare.size());
        if (shorter * 3 < longer * 2) {
            continue;
        }
        if (mineBare.find(theirsBare) != std::string::npos ||
            theirsBare.find(mineBare) != std::string::npos) {
            return true;
        }
    }
    return false;
}

void TurnEngine::heard(const AsrPartial &said)
{
    clock_ = std::max(clock_, said.end);

    Side &heardFrom = sideOf(said.voice);

    heardFrom.lastHeard = clock_;

    const auto whole = tidiedLine(said.text);
    if (whole.empty()) {
        return;
    }

    Side &side = heardFrom;

    const std::string line{whatIsNewIn(side, whole)};
    if (line.empty()) {
        return;
    }

    if (line != side.provisional) {
        if (side.provisional.empty()) {

            side.started = std::max(said.end, side.handedAt);

            if (joining()) {
                giveBackWhatWasHeld(side, said.voice);
            }
        }
        side.provisional = line;
        side.lastWord = clock_;
        handTheTurnOver(said.voice);
    }
}

void TurnEngine::handTheTurnOver(Voice voice)
{

    if (joining()) {
        releaseWaiting(voice == Voice::Near ? Voice::Far : Voice::Near, clock_);
    }
}

void TurnEngine::heard(const AsrTurn &said)
{
    clock_ = std::max(clock_, said.end);
    sideOf(said.voice).lastHeard = clock_;

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
        copy.voice = line.voice;
        copy.at = line.end;
        copy.from = line.start;
        copy.to = line.end;
        copy.words = wordsIn(line.text);
        copy.quietFor = std::chrono::milliseconds::zero();
        copy.pending = pending_.size();
        decided(copy);
        return;
    }

    if (line.ordinal.has_value() && !side.waiting.text.empty() &&
        side.waiting.ordinal == line.ordinal) {
        TurnReading better;
        better.verdict = TurnReading::Verdict::Revised;
        better.boundary = line.boundary;
        better.voice = line.voice;
        better.at = line.end;
        better.from = side.waiting.start;
        better.to = line.end;
        better.words = wordsIn(line.text);
        better.quietFor = std::chrono::milliseconds::zero();
        better.pending = pending_.size();
        decided(better);

        const auto began = side.waiting.start;
        side.waiting = line;
        side.waiting.start = began;
        return;
    }

    if (line.ordinal.has_value() && side.trimmed == line.ordinal) {
        TurnReading cut;
        cut.verdict = TurnReading::Verdict::Copied;
        cut.boundary = line.boundary;
        cut.voice = line.voice;
        cut.at = line.end;
        cut.from = line.start;
        cut.to = line.end;
        cut.words = wordsIn(line.text);
        cut.quietFor = std::chrono::milliseconds::zero();
        cut.pending = pending_.size();
        decided(cut);
        return;
    }

    const bool continued = !side.handedOut.empty() && line.text.starts_with(side.handedOut);
    line.text = std::string{whatIsNewIn(side, line.text)};

    if (line.boundary == AsrTurn::Boundary::Provider) {

        side.trimmed = continued ? line.ordinal : std::optional<unsigned>{};
        side.handedOut.clear();
    }

    if (line.text.empty()) {
        side.lastWord = clock_;
        return;
    }

    if (!side.provisional.empty()) {
        line.start = std::min(side.started, line.end);
    }

    side.provisional.clear();

    takeWhatIsOwed(side, line);
    side.marks = 0;
    handTheTurnOver(line.voice);
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

    const auto grownPastAPhrase = [this](const Side &side) {
        return rules_.longestReply > 0 && rules_.policy != TurnPolicy::Manual &&
               wordsIn(side.owed) + wordsIn(side.provisional) >= rules_.longestReply;
    };

    const auto quiet = quietEndsIt();
    for (const auto voice : {Voice::Far, Voice::Near}) {
        Side &side = sideOf(voice);

        if (side.provisional.empty() && side.owed.empty()) {
            continue;
        }

        const auto overlong = grownPastAPhrase(side);

        const auto handedOver = otherSideTookOver(voice);
        if (!watchingForSilence() && !overlong) {

            if (rules_.policy == TurnPolicy::Manual ||
                rules_.providerIdle <= std::chrono::milliseconds::zero() ||
                clock_ - side.lastHeard < rules_.providerIdle) {
                continue;
            }
            AsrTurn said;
            said.voice = voice;
            said.start = side.started;
            said.end = side.lastWord;
            said.text = side.provisional;
            said.boundary = AsrTurn::Boundary::Silence;

            tookItOutOfTheLine(side, said);
            takeWhatIsOwed(side, said);
            close(said, clock_);
            side.lastWord = clock_;
            continue;
        }

        if (clock_ - side.lastWord < (overlong || handedOver ? rules_.silence : quiet)) {
            continue;
        }

        if (rules_.providerTakesOurBoundary && side.marks == 0 && !overlong) {
            TurnReading mark;
            mark.verdict = TurnReading::Verdict::Marked;
            mark.boundary = AsrTurn::Boundary::Silence;
            mark.voice = voice;
            mark.at = clock_;
            mark.from = side.started;
            mark.to = side.lastWord;
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
        said.boundary = overlong ? AsrTurn::Boundary::Overlong : AsrTurn::Boundary::Silence;
        tookItOutOfTheLine(side, said);
        takeWhatIsOwed(side, said);

        side.marks = 0;
        side.awaitingCopy = rules_.providerTakesOurBoundary;
        close(said, clock_);
        side.lastWord = clock_;
    }
}

void TurnEngine::askNow()
{

    releaseEverything(clock_);

    if (!lineInHand(Voice::Near).empty()) {
        AsrTurn mine;
        mine.voice = Voice::Near;
        mine.start = ours_.started;
        mine.end = clock_;
        mine.text = ours_.provisional;
        mine.boundary = AsrTurn::Boundary::Ours;
        tookItOutOfTheLine(ours_, mine);
        takeWhatIsOwed(ours_, mine);

        ours_.marks = 0;
        ours_.awaitingCopy = rules_.providerTakesOurBoundary;
        close(mine, clock_, true);
        ours_.lastWord = clock_;
    }

    if (lineInHand(Voice::Far).empty()) {

        dispatch();
        return;
    }

    AsrTurn said;
    said.voice = Voice::Far;
    said.start = theirs_.started;
    said.end = clock_;
    said.text = theirs_.provisional;
    said.boundary = AsrTurn::Boundary::Ours;
    tookItOutOfTheLine(theirs_, said);
    takeWhatIsOwed(theirs_, said);
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
    case AsrTurn::Boundary::Overlong:
        return "overlong";
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
    const auto span =
        std::to_string(reading.from.count()) + ".." + std::to_string(reading.to.count());

    const auto side = voiceText(reading.voice);
    const LogAttribute fields[] = {
        {"verdict", nameOf(reading.verdict)},
        {"boundary", nameOf(reading.boundary)},
        {"side", side},
        {"span_ms", span},
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

        joined.voice = said.voice;
        joined.from = waiting.waiting.start;
        joined.to = said.end;
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
    held.voice = said.voice;
    held.from = said.start;
    held.to = said.end;
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
    reading.voice = said.voice;
    reading.at = at;
    reading.from = said.start;
    reading.to = said.end;
    reading.words = wordsIn(said.text);
    reading.quietFor = quietFor;
    reading.pending = pending_.size();

    if (said.ordinal.has_value()) {
        const auto already = std::ranges::find_if(replies_, [&said](const AsrTurn &had) {
            return had.ordinal == said.ordinal && had.voice == said.voice;
        });
        if (already != replies_.end()) {

            const auto began = already->start;
            const auto where = static_cast<std::size_t>(std::distance(replies_.begin(), already));
            *already = said;
            already->start = began;
            reading.verdict = TurnReading::Verdict::Revised;
            reading.from = began;
            decided(reading);

            if (kept_) {
                kept_(*already, where);
            }

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
