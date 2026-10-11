// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/plugins/realtime/session.hpp>

#include <softphone/core/logging.hpp>
#include <softphone/core/text_search.hpp>

#include <algorithm>
#include <thread>
#include <utility>

namespace softphone::plugins::realtime {
namespace {

using namespace std::chrono_literals;

constexpr std::string_view kRecogniserRefused = "recogniser-refused";

}

class Session::Sink final : public core::IStreamingAsrSink {
public:
    explicit Sink(Session &session) : session_{session} {}

    void listening() override
    {
        if (session_.listener_.listening) {
            session_.listener_.listening();
        }
    }

    void partial(const core::AsrPartial &said) override
    {
        {
            const std::lock_guard<std::mutex> held{session_.lock_};
            if (session_.engine_) {
                session_.engine_->heard(said);
            }
        }
        if (session_.listener_.partial) {

            session_.listener_.partial(said.voice, core::tidiedLine(said.text));
        }
    }

    void turn(const core::AsrTurn &said) override
    {

        const std::lock_guard<std::mutex> held{session_.lock_};
        if (session_.engine_) {
            session_.engine_->heard(said);
        }
    }

    void gap(std::chrono::milliseconds at, std::chrono::milliseconds howMuch) override
    {
        {
            const std::lock_guard<std::mutex> held{session_.lock_};
            session_.lost_ += howMuch;
        }
        if (session_.listener_.gap) {
            session_.listener_.gap(at, howMuch);
        }
    }

    void spent(std::int64_t costMicros) override
    {

        const std::lock_guard<std::mutex> held{session_.lock_};
        session_.purse_.modelSpent(0);
        session_.measured_ = costMicros;
    }

    void finished(const core::AsrResult &result) override
    {

        if (session_.ending_.fetch_add(1, std::memory_order_acq_rel) + 1 <
            session_.streams_.load(std::memory_order_acquire)) {
            if (!core::succeeded(result.status) && result.status != core::AsrStatus::Cancelled &&
                session_.listener_.trouble) {
                session_.listener_.trouble(std::string{kRecogniserRefused}, result.error);
            }
            return;
        }

        const auto refused =
            !core::succeeded(result.status) && result.status != core::AsrStatus::Cancelled;
        if (refused && session_.listener_.trouble) {
            session_.listener_.trouble(std::string{kRecogniserRefused}, result.error);
        }
        if (session_.listener_.finished) {
            session_.listener_.finished(result);
        }
        session_.running_.store(false, std::memory_order_release);
    }

private:
    Session &session_;
};

Session::Session(Opening opening, std::unique_ptr<core::IStreamingAsr> recogniser, Ask ask,
                 Listener listener)
    : opening_{std::move(opening)}, recogniser_{std::move(recogniser)}, ask_{std::move(ask)},
      listener_{std::move(listener)}, purse_{opening_.purse}
{
    purse_.thisMonth(opening_.recogniserMicrosThisMonth, opening_.modelMicrosThisMonth);

    engine_ = std::make_unique<core::TurnEngine>(
        opening_.turns, [this](const core::AskedTurns &asked) { heard(asked); },
        [this](const core::AsrTurn &said, std::size_t) {

            if (said.boundary != core::AsrTurn::Boundary::Provider &&
                !leaving_.load(std::memory_order_acquire)) {
                auto *const which =
                    said.voice == core::Voice::Near ? ourStream_.get() : stream_.get();
                if (which != nullptr) {
                    which->turnEnded();
                }
            }

            if (listener_.turn) {
                auto onTheConversation = said;
                onTheConversation.start += opening_.since;
                onTheConversation.end += opening_.since;
                listener_.turn(onTheConversation);
            }
        },
        [this](core::Voice voice) {

            if (leaving_.load(std::memory_order_acquire)) {
                return;
            }
            auto *const which = voice == core::Voice::Near ? ourStream_.get() : stream_.get();
            if (which != nullptr) {
                which->turnEnded();
            }
        });

    if (recogniser_ == nullptr || opening_.farRing == nullptr) {

        if (listener_.trouble) {
            listener_.trouble(std::string{kRecogniserRefused}, {});
        }
        if (listener_.finished) {
            core::AsrResult nothing;
            nothing.status = core::AsrStatus::Unsupported;
            nothing.error = "no recogniser, or nothing to listen to";
            listener_.finished(nothing);
        }
        return;
    }

    const auto can = recogniser_->capabilities(opening_.recogniser);
    const auto wire = core::wireFor(can, opening_.sourceRate);
    if (!wire.has_value()) {
        if (listener_.trouble) {
            listener_.trouble(std::string{kRecogniserRefused}, {});
        }
        if (listener_.finished) {
            core::AsrResult nothing;
            nothing.status = core::AsrStatus::Unsupported;
            nothing.error = "this recogniser takes no audio this build can produce";
            listener_.finished(nothing);
        }
        return;
    }

    core::StreamingOptions options;
    options.language = opening_.language;
    options.into = can.translates ? opening_.into : std::string{};
    options.voice = core::Voice::Far;
    options.wire = *wire;
    wireRate_ = wire->sampleRate == 0 ? 16000u : wire->sampleRate;
    options.diarise = can.diarisation;
    options.params = opening_.recogniser.params;

    options.endOfTurnSilence = opening_.turns.silence;

    sink_ = std::make_unique<Sink>(*this);

    streams_.store(opening_.nearRing != nullptr ? 2 : 1, std::memory_order_release);

    stream_ = recogniser_->open(opening_.recogniser, options, *sink_,
                                [this] { return leaving_.load(std::memory_order_acquire); });

    fromFar_ = std::make_unique<core::RealtimeReader>(*opening_.farRing, opening_.sourceRate, *wire,
                                                      can.leastPerMessage);

    if (opening_.nearRing != nullptr) {
        auto mine = options;
        mine.voice = core::Voice::Near;
        ourStream_ = recogniser_->open(opening_.recogniser, mine, *sink_,
                                       [this] { return leaving_.load(std::memory_order_acquire); });

        if (ourStream_ == nullptr) {
            streams_.store(1, std::memory_order_release);
        }
        fromNear_ = std::make_unique<core::RealtimeReader>(*opening_.nearRing, opening_.sourceRate,
                                                           *wire, can.leastPerMessage);

        core::Logger::instance().log(core::LogLevel::Info, "realtime",
                                     "both sides are being recognised, which is two "
                                     "sessions on this recogniser and twice its bill");
    }

    {
        const auto policy = std::string{core::toString(opening_.turns.policy)};
        const auto silence = std::to_string(opening_.turns.silence.count());

        const auto idle = std::to_string(opening_.turns.providerIdle.count());
        const auto join = std::to_string(opening_.turns.join.count());
        const core::LogAttribute fields[] = {
            {"assistant", opening_.prompt.name},
            {"turns", policy},
            {"provider_marks", opening_.turns.providerMarksTurns ? "yes" : "no"},
            {"silence_ms", silence},
            {"idle_ms", idle},
            {"join_ms", join},
            {"asks_a_model", opening_.turns.asksAModel ? "yes" : "no"},
            {"sides", ourStream_ != nullptr ? "near+far" : "far"},
        };
        core::Logger::instance().log(core::LogLevel::Info, "realtime", "the prompter is listening",
                                     fields);
    }
    running_.store(true, std::memory_order_release);

    reader_ = core::JoiningThread{[this] { read(); }};
    asker_ = core::JoiningThread{[this] { askTheModel(); }};
}

Session::~Session()
{
    stop();
}

void Session::stop()
{

    const bool first = !leaving_.exchange(true, std::memory_order_acq_rel);

    const auto step = [](const char *what) {
        core::Logger::instance().log(core::LogLevel::Trace, "realtime", what);
    };
    step(first ? "stopping: this is the caller that will conclude" : "stopping: already leaving");

    wake_.notify_all();

    reader_.join();
    step("stopping: the reader has finished and flushed what it was holding");

    if (first) {
        conclude();
        step("stopping: the sockets have been told there is no more audio");
    }

    asker_.join();
    step("stopping: the model thread has finished");
    running_.store(false, std::memory_order_release);
}

void Session::conclude()
{

    core::JoiningThread nearSide;
    if (ourStream_) {
        nearSide = core::JoiningThread{[this] {
            core::Logger::instance().log(core::LogLevel::Trace, "realtime",
                                         "concluding: draining the near socket");
            ourStream_->drain();
        }};
    }
    if (stream_) {
        core::Logger::instance().log(core::LogLevel::Trace, "realtime",
                                     "concluding: draining the far socket");
        stream_->drain();
    }
    nearSide.join();
    core::Logger::instance().log(core::LogLevel::Trace, "realtime",
                                 "concluding: both sockets have finished");

    {
        core::Logger::instance().log(core::LogLevel::Trace, "realtime",
                                     "concluding: closing the reply in hand");
        const std::lock_guard<std::mutex> held{lock_};
        if (engine_) {
            engine_->askNow();
        }
    }
    running_.store(false, std::memory_order_release);
}

void Session::forget()
{
    const std::lock_guard<std::mutex> held{lock_};
    if (engine_) {
        engine_->forget();
    }
}

void Session::askNow()
{
    const std::lock_guard<std::mutex> held{lock_};
    if (engine_) {
        engine_->askNow();
    }
}

void Session::flushTheReaders()
{

    if (stream_ && fromFar_) {
        static_cast<void>(fromFar_->flush(*stream_));
    }
    if (ourStream_ && fromNear_) {
        static_cast<void>(fromNear_->flush(*ourStream_));
    }
}

void Session::read()
{
    auto next = std::chrono::steady_clock::now();

    while (!leaving_.load(std::memory_order_acquire)) {
        next += kReadEvery;

        if (stream_) {
            const auto did = fromFar_->pump(*stream_);
            if (fromNear_ && ourStream_) {

                static_cast<void>(fromNear_->pump(*ourStream_));
            }

            if (did.lost > 0ms) {
                {
                    const std::lock_guard<std::mutex> held{lock_};
                    lost_ += did.lost;
                }
                if (listener_.gap) {
                    listener_.gap(did.lostAt, did.lost);
                }
            }
        }

        std::chrono::milliseconds listened{0};
        {
            const std::lock_guard<std::mutex> held{lock_};
            listened_ += kReadEvery;
            listened = listened_;
            if (engine_) {
                engine_->now(listened_);
            }
            purse_.listenedFor(listened_);
        }

        if (stream_) {
            static_cast<void>(fromFar_->quiet(*stream_, listened));
        }
        if (fromNear_ && ourStream_) {
            static_cast<void>(fromNear_->quiet(*ourStream_, listened));
        }

        if (listened.count() % 1000 == 0 &&
            core::Logger::instance().level() >= core::LogLevel::Trace) {
            const auto asMs = [this](std::uint64_t samples) {
                return static_cast<std::int64_t>(samples * 1000 / wireRate_);
            };
            auto line = "audio sent " + std::to_string(asMs(fromFar_->sentSamples())) + " ms + " +
                        std::to_string(asMs(fromFar_->hushSamples())) + " ms of silence, against " +
                        std::to_string(listened.count()) + " ms listened, far";
            if (fromNear_) {
                line += "; near " + std::to_string(asMs(fromNear_->sentSamples())) + " ms + " +
                        std::to_string(asMs(fromNear_->hushSamples())) + " ms";
            }
            core::Logger::instance().log(core::LogLevel::Trace, "realtime", line);
        }

        if (listened.count() % 1000 == 0) {
            core::RealtimeSpend verdict{};
            core::RealtimeSpent sofar;
            bool priced = false;
            std::string trouble;
            {
                const std::lock_guard<std::mutex> held{lock_};
                verdict = purse_.verdict();
                sofar = purse_.spent();
                priced = purse_.pricedRecogniser();
                trouble = std::string{purse_.trouble()};
            }

            if (listener_.spent) {
                listener_.spent(sofar.recogniserMicros, sofar.modelMicros, !priced);
            }

            if (verdict == core::RealtimeSpend::ModelOnly &&
                !modelStopped_.exchange(true, std::memory_order_acq_rel)) {

                if (listener_.trouble) {
                    listener_.trouble(trouble, {});
                }
            }

            if (verdict == core::RealtimeSpend::Stop) {
                if (listener_.trouble) {
                    listener_.trouble(trouble, {});
                }
                core::Logger::instance().log(core::LogLevel::Info, "realtime",
                                             "the recogniser's monthly ceiling is reached; "
                                             "stopping at the end of this reply");

                if (!leaving_.exchange(true, std::memory_order_acq_rel)) {
                    wake_.notify_all();
                    flushTheReaders();
                    conclude();
                }
                return;
            }
        }

        std::this_thread::sleep_until(next);
    }

    flushTheReaders();
}

void Session::heard(const core::AskedTurns &asked)
{

    if (modelStopped_.load(std::memory_order_acquire)) {
        return;
    }
    waiting_ = asked;
    ++asked_;
    wake_.notify_all();
}

void Session::askTheModel()
{
    for (;;) {
        core::AskedTurns asking;
        {
            std::unique_lock<std::mutex> held{lock_};
            wake_.wait(held, [this] {
                return waiting_.has_value() || leaving_.load(std::memory_order_acquire);
            });
            if (!waiting_.has_value()) {
                return;
            }
            asking = *std::exchange(waiting_, std::nullopt);
        }

        if (!ask_ || !opening_.turns.asksAModel) {

            const std::lock_guard<std::mutex> held{lock_};
            if (engine_) {
                engine_->answered();
            }
            continue;
        }

        core::PromptContext context;
        context.reply = asking.reply;
        context.conversation = asking.conversation;

        context.language = opening_.language;

        const auto rendered = core::renderPrompt(opening_.prompt, context, opening_.interface);
        const auto answer =
            ask_(rendered, [this] { return leaving_.load(std::memory_order_acquire); });

        {
            const std::lock_guard<std::mutex> held{lock_};
            purse_.modelSpent(answer.costMicros);
            if (engine_) {
                engine_->answered();
            }
        }

        if (answer.worked) {
            if (listener_.answered && !asking.at.empty()) {

                const auto at = asking.covers.empty()
                                    ? opening_.since
                                    : asking.covers.front().start + opening_.since;
                listener_.answered(answer.text, asking.at.front(), asking.at.back(), at);
            }
        } else if (leaving_.load(std::memory_order_acquire)) {

        } else if (listener_.trouble) {

            listener_.trouble(std::string{"model-refused"}, answer.text);
        }

        if (leaving_.load(std::memory_order_acquire)) {
            return;
        }
    }
}

std::chrono::milliseconds Session::listenedFor() const
{
    const std::lock_guard<std::mutex> held{lock_};
    return listened_;
}

std::chrono::milliseconds Session::lost() const
{
    const std::lock_guard<std::mutex> held{lock_};
    return lost_;
}

core::RealtimeSpent Session::spent() const
{
    const std::lock_guard<std::mutex> held{lock_};
    auto sofar = purse_.spent();
    if (measured_ > 0) {

        sofar.recogniserMicros = measured_;
    }
    return sofar;
}

std::size_t Session::turns() const
{
    const std::lock_guard<std::mutex> held{lock_};
    return engine_ ? engine_->replies().size() : 0;
}

std::size_t Session::asked() const
{
    const std::lock_guard<std::mutex> held{lock_};
    return asked_;
}

std::vector<core::AsrTurn> Session::replies() const
{
    const std::lock_guard<std::mutex> held{lock_};
    if (!engine_) {
        return {};
    }

    auto out = engine_->replies();
    for (core::AsrTurn &one : out) {
        one.start += opening_.since;
        one.end += opening_.since;
    }
    return out;
}

std::vector<core::TurnReading> Session::readings() const
{
    const std::lock_guard<std::mutex> held{lock_};
    return engine_ ? engine_->readings() : std::vector<core::TurnReading>{};
}

}
