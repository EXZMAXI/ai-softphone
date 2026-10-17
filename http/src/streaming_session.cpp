// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "streaming_session.hpp"

#include "streaming_json.hpp"

#include <softphone/core/logging.hpp>
#include <softphone/core/web_socket.hpp>

#include <algorithm>
#include <cctype>
#include <condition_variable>
#include <ranges>
#include <span>
#include <thread>
#include <utility>

namespace softphone::http {
namespace {

using namespace std::chrono_literals;

constexpr auto kDrainGrace = 600ms;

constexpr auto kDrainTick = 20ms;

constexpr std::string_view kComplaintFields[] = {"message", "error", "detail", "reason"};

constexpr std::string_view kRefusalFields[] = {"error", "detail"};

constexpr std::string_view kRefusalShapes[] = {"error",  "invalid", "fail",     "refus",
                                               "reject", "denied",  "unauthor", "forbidden"};

constexpr std::string_view kRemarkShapes[] = {"warn", "info", "notice", "debug", "hint"};

constexpr std::size_t kUnreadTail = 240;

constexpr std::size_t kTraceRoom = 4096;

void trace(std::string_view type, std::string_view direction, std::string_view payload)
{

    if (core::Logger::instance().level() < core::LogLevel::Trace) {
        return;
    }
    auto body = std::string{payload.substr(0, std::min(payload.size(), kTraceRoom))};
    if (payload.size() > kTraceRoom) {
        body += "… (" + std::to_string(payload.size()) + " bytes)";
    }
    std::ranges::replace(body, '\n', ' ');
    std::ranges::replace(body, '\r', ' ');
    core::Logger::instance().log(core::LogLevel::Trace, "realtime",
                                 std::string{type} + ' ' + std::string{direction} + ' ' + body);
}

struct Complaint {

    std::string said;

    bool refusal{false};
};

[[nodiscard]] bool shapeMentions(std::string_view shape,
                                 std::span<const std::string_view> words) noexcept
{
    const auto alike = [](char left, char right) {
        return std::tolower(static_cast<unsigned char>(left)) ==
               std::tolower(static_cast<unsigned char>(right));
    };
    return std::ranges::any_of(words, [shape, alike](std::string_view word) {
        return !std::ranges::search(shape, word, alike).empty();
    });
}

[[nodiscard]] Complaint complaintIn(std::string_view payload, std::string_view shape)
{
    Complaint complaint;

    const auto remark = shapeMentions(shape, kRemarkShapes);
    complaint.refusal = !remark && shapeMentions(shape, kRefusalShapes);

    const auto parsed = nlohmann::json::parse(payload, nullptr, false);
    if (parsed.is_discarded() || !parsed.is_object()) {
        return complaint;
    }

    for (const auto &name : kComplaintFields) {
        const auto found = parsed.find(std::string{name});
        if (found == parsed.end()) {
            continue;
        }

        const auto itself =
            !remark && std::ranges::find(kRefusalFields, name) != std::ranges::end(kRefusalFields);

        if (found->is_string()) {
            complaint.said = found->template get<std::string>();
            complaint.refusal = complaint.refusal || itself;
            return complaint;
        }

        if (found->is_object()) {
            for (const auto &inner : kComplaintFields) {
                const auto deeper = found->find(std::string{inner});
                if (deeper != found->end() && deeper->is_string()) {
                    complaint.said = deeper->template get<std::string>();
                    complaint.refusal = complaint.refusal || itself;
                    return complaint;
                }
            }
        }
    }
    return complaint;
}

[[nodiscard]] std::string unreadTail(const std::string &said)
{
    if (said.empty()) {
        return {};
    }
    auto tail = said.substr(0, kUnreadTail);

    std::ranges::replace(tail, '\n', ' ');
    std::ranges::replace(tail, '\r', ' ');
    return ". It said: " + tail;
}

[[nodiscard]] core::AsrStatus asAsrStatus(core::WebSocketStatus status) noexcept
{
    switch (status) {
    case core::WebSocketStatus::Ok:
        return core::AsrStatus::Ok;
    case core::WebSocketStatus::Unauthorised:
        return core::AsrStatus::Unauthorised;
    case core::WebSocketStatus::Refused:

        return core::AsrStatus::Unsupported;
    case core::WebSocketStatus::Untrusted:
        return core::AsrStatus::Failed;
    case core::WebSocketStatus::Unreachable:
    case core::WebSocketStatus::Dropped:
        return core::AsrStatus::Unavailable;
    case core::WebSocketStatus::Timeout:
        return core::AsrStatus::Timeout;
    case core::WebSocketStatus::Cancelled:
        return core::AsrStatus::Cancelled;
    case core::WebSocketStatus::Failed:
        break;
    }
    return core::AsrStatus::Failed;
}

}

StreamingSession::StreamingSession(core::IWebSocketFactory &sockets,
                                   std::unique_ptr<IStreamingProtocol> protocol,
                                   const core::AsrEndpoint &where, core::StreamingOptions options,
                                   core::IStreamingAsrSink &sink, core::AsrCancel cancelled)
    : protocol_{std::move(protocol)}, where_{where}, options_{std::move(options)}, sink_{sink},
      cancelled_{std::move(cancelled)}
{
    core::WebSocketEndpoint socket;
    socket.url = protocol_->url(where_, options_);
    socket.headers = protocol_->headers(where_);

    socket.secret = where_.token;

    core::Logger::instance().log(core::LogLevel::Info, "realtime",
                                 std::string{protocol_->type()} + ": opening " + socket.redacted() +
                                     " at " + std::to_string(options_.wire.sampleRate) + " Hz " +
                                     std::string{toString(options_.wire.encoding)} + ", " +
                                     std::string{toString(options_.voice)});

    socket_ = sockets.connect(socket, *this);
}

StreamingSession::~StreamingSession()
{

    if (socket_) {
        socket_->close(500ms);
        socket_.reset();
    }

    finish(core::AsrStatus::Cancelled, "the session was closed");
}

bool StreamingSession::send(std::chrono::milliseconds position,
                            std::span<const std::int16_t> samples)
{
    if (samples.empty()) {
        return true;
    }

    if (cancelled_ && cancelled_()) {
        return false;
    }

    {
        const std::lock_guard<std::mutex> held{lock_};
        if (over_ || !up_ || !socket_) {
            return false;
        }
        furthest_ = std::max(furthest_, position);
    }

    {
        const std::lock_guard<std::mutex> held{lock_};
        if (protocol_->waitsToBeTold() && !told_) {
            if (!saidWaiting_) {
                saidWaiting_ = true;
                core::Logger::instance().log(
                    core::LogLevel::Debug, "realtime",
                    std::string{protocol_->type()} +
                        ": holding audio back until the recogniser says it is ready");
            }
            return true;
        }
    }

    const auto bytes = std::as_bytes(samples);

    const auto went = [this] {
        const std::lock_guard<std::mutex> held{lock_};
        ++framesSent_;
    };

    if (protocol_->audioIsBinary()) {
        if (!socket_->send(bytes)) {
            return false;
        }
        went();
        return true;
    }

    wrapped_ = protocol_->wrapAudio(bytes);
    if (wrapped_.empty() || !socket_->sendText(wrapped_)) {
        return false;
    }
    went();
    return true;
}

void StreamingSession::turnEnded()
{
    {
        const std::lock_guard<std::mutex> held{lock_};
        if (over_) {
            return;
        }
    }

    {
        const std::lock_guard<std::mutex> held{lock_};
        heardFrom_ = furthest_;
    }

    if (!socket_ || !socket_->open()) {
        return;
    }
    for (const auto &message : protocol_->endOfTurn()) {
        trace(protocol_->type(), "→", message);
        (void)socket_->sendText(message);
    }
}

void StreamingSession::drain()
{
    {
        const std::lock_guard<std::mutex> held{lock_};
        if (over_) {
            return;
        }
    }

    std::size_t repliesBefore = 0;
    if (socket_ && socket_->open()) {
        std::size_t sent = 0;
        {
            const std::lock_guard<std::mutex> held{lock_};
            sent = framesSent_;
            repliesBefore = turns_.size();
        }
        const auto asBytes = protocol_->closingIsBinary();
        for (const auto &message : protocol_->closing(sent)) {
            if (!asBytes) {
                trace(protocol_->type(), "→", message);
            }
            if (asBytes) {
                (void)socket_->send(std::as_bytes(std::span{message.data(), message.size()}));
            } else {
                (void)socket_->sendText(message);
            }
        }
    }

    const auto deadline = std::chrono::steady_clock::now() + kDrainGrace;
    for (;;) {
        {
            const std::lock_guard<std::mutex> held{lock_};
            if (over_) {
                return;
            }

            if (turns_.size() > repliesBefore) {
                break;
            }
        }
        if (std::chrono::steady_clock::now() >= deadline) {
            break;
        }
        std::this_thread::sleep_for(kDrainTick);
    }

    if (socket_) {
        socket_->close(500ms);
    }
    finish(core::AsrStatus::Ok, {});
}

bool StreamingSession::listening() const
{
    const std::lock_guard<std::mutex> held{lock_};
    return up_ && !over_;
}

void StreamingSession::opened()
{
    {
        const std::lock_guard<std::mutex> held{lock_};
        if (over_) {
            return;
        }
        up_ = true;
    }

    for (const auto &message : protocol_->opening(where_, options_)) {
        trace(protocol_->type(), "→", message);
        if (!socket_->sendText(message)) {
            break;
        }
    }

    sink_.listening();
}

void StreamingSession::text(std::string_view payload)
{
    trace(protocol_->type(), "←", payload);

    StreamingHeard heard;
    protocol_->read(payload, heard);

    if (!heard.understood) {
        auto complaint = complaintIn(payload, heard.shape);
        if (!complaint.said.empty()) {
            heard.said = std::move(complaint.said);
        }
        if (complaint.refusal) {
            heard.fatal = core::AsrStatus::Failed;
        }
    }

    deliver(heard);
}

void StreamingSession::binary(std::span<const std::byte> payload)
{

    trace(protocol_->type(), "←", "(binary, " + std::to_string(payload.size()) + " bytes)");

    StreamingHeard heard;
    protocol_->readBinary(payload, heard);
    deliver(heard);
}

void StreamingSession::deliver(StreamingHeard &heard)
{
    {

        const std::lock_guard<std::mutex> held{lock_};
        const auto shape = heard.shape.empty() ? std::string{"(unnamed)"} : heard.shape;
        const auto already = std::ranges::find_if(
            shapesSeen_, [&shape](const auto &one) { return one.first == shape; });
        if (already == shapesSeen_.end()) {
            shapesSeen_.emplace_back(shape, 1U);
        } else {
            ++already->second;
        }
    }

    if (heard.remark && !heard.said.empty()) {

        const std::lock_guard<std::mutex> held{lock_};
        if (std::ranges::find(remarked_, heard.said) == remarked_.end()) {
            remarked_.push_back(heard.said);
            core::Logger::instance().log(core::LogLevel::Info, "realtime",
                                         std::string{protocol_->type()} +
                                             " remarked, and went on: " + heard.said);
        }
    }

    if (!heard.understood) {

        const std::lock_guard<std::mutex> held{lock_};
        if (std::ranges::find(unread_, heard.shape) == unread_.end()) {
            unread_.push_back(heard.shape);
            core::Logger::instance().log(
                core::LogLevel::Warning, "realtime",
                std::string{protocol_->type()} +
                    ": nothing in this adapter reads a message of "
                    "shape '" +
                    (heard.shape.empty() ? std::string{"(unnamed)"} : heard.shape) + "'" +

                    (heard.said.empty() ? std::string{" — the protocol may have moved"}
                                        : unreadTail(heard.said)));
        }
    }

    {
        const std::lock_guard<std::mutex> held{lock_};
        if (over_) {
            return;
        }
        if (heard.ready && !told_) {
            told_ = true;
            core::Logger::instance().log(core::LogLevel::Debug, "realtime",
                                         std::string{protocol_->type()} +
                                             ": the recogniser says it is ready for audio");
        }
        if (heard.costMicros) {
            costMicros_ = *heard.costMicros;
        }

        for (auto &said : heard.partials) {
            said.voice = options_.voice;
            if (said.end == std::chrono::milliseconds::zero()) {
                said.end = furthest_;
            }
            if (said.start == std::chrono::milliseconds::zero()) {
                said.start = heardFrom_;
            }
        }
        for (auto &said : heard.turns) {
            said.voice = options_.voice;
            if (said.end == std::chrono::milliseconds::zero()) {
                said.end = furthest_;
            }
            if (said.start == std::chrono::milliseconds::zero()) {
                said.start = heardFrom_;
            }

            heardFrom_ = said.end;
        }

        for (const auto &said : heard.turns) {

            const auto already = said.ordinal.has_value()
                                     ? std::ranges::find_if(turns_,
                                                            [&said](const core::AsrTurn &had) {
                                                                return had.ordinal == said.ordinal;
                                                            })
                                     : turns_.end();

            if (already != turns_.end()) {
                *already = said;
            } else {
                turns_.push_back(said);
            }

            if (detected_.empty() && !said.language.empty()) {
                detected_ = said.language;
            }
        }
    }

    for (const auto &said : heard.turns) {
        sink_.turn(said);
    }
    for (const auto &said : heard.partials) {
        sink_.partial(said);
    }
    if (heard.costMicros) {
        sink_.spent(*heard.costMicros);
    }

    if (heard.fatal) {
        finish(*heard.fatal, heard.said);
        if (socket_) {
            socket_->close(0ms);
        }
        return;
    }

    if (heard.ended) {
        finish(core::AsrStatus::Ok, {});
    }
}

void StreamingSession::reconnecting(unsigned attempt, std::chrono::milliseconds after)
{
    {
        const std::lock_guard<std::mutex> held{lock_};
        up_ = false;
    }

    core::Logger::instance().log(core::LogLevel::Warning, "realtime",
                                 std::string{protocol_->type()} +
                                     ": the socket went away, trying "
                                     "again in " +
                                     std::to_string(after.count()) + " ms (attempt " +
                                     std::to_string(attempt) + ")");
}

void StreamingSession::closed(core::WebSocketStatus status, std::string_view why)
{
    {
        const std::lock_guard<std::mutex> held{lock_};
        up_ = false;
    }

    const auto ordinary = status == core::WebSocketStatus::Ok;
    core::Logger::instance().log(
        ordinary ? core::LogLevel::Info : core::LogLevel::Warning, "realtime",
        std::string{protocol_->type()} + ": the session ended — " + std::string{toString(status)} +
            (why.empty() ? std::string{} : ": " + std::string{why}));

    finish(asAsrStatus(status), std::string{why});
}

void StreamingSession::finish(core::AsrStatus status, std::string why)
{
    core::AsrResult result;
    {
        const std::lock_guard<std::mutex> held{lock_};
        if (over_) {
            return;
        }
        over_ = true;
        up_ = false;

        result.status = status;
        result.error = std::move(why);
        result.detectedLanguage = detected_.empty() ? options_.language : detected_;
        result.model = where_.model;
        result.costMicros = costMicros_;
        result.audio = furthest_;

        if (!shapesSeen_.empty()) {
            std::string census;
            for (const auto &[shape, count] : shapesSeen_) {
                if (!census.empty()) {
                    census += ", ";
                }
                census += shape + "×" + std::to_string(count);
            }
            core::Logger::instance().log(core::LogLevel::Debug, "realtime",
                                         std::string{protocol_->type()} +
                                             ": what the far end sent — " + census);
        }

        result.segments.reserve(turns_.size());
        for (const auto &said : turns_) {
            core::AsrSegment segment;
            segment.start = said.start;
            segment.end = said.end;
            segment.text = said.text;
            segment.confidence = said.confidence;
            segment.words = said.words;
            result.segments.push_back(std::move(segment));
        }
    }

    sink_.finished(result);
}

ProtocolRecogniser::ProtocolRecogniser(core::IWebSocketFactory &sockets, Make make)
    : sockets_{sockets}, make_{make}, asked_{make()}
{}

std::string_view ProtocolRecogniser::type() const noexcept
{
    return asked_->type();
}

core::StreamingCapabilities ProtocolRecogniser::capabilities(const core::AsrEndpoint &where) const
{
    return asked_->capabilities(where);
}

std::unique_ptr<core::IStreamingAsrSession>
ProtocolRecogniser::open(const core::AsrEndpoint &where, const core::StreamingOptions &options,
                         core::IStreamingAsrSink &sink, const core::AsrCancel &cancelled)
{

    return std::make_unique<StreamingSession>(sockets_, make_(), where, options, sink, cancelled);
}

namespace {

class Probing final : public core::IStreamingAsrSink {
public:
    void listening() override
    {
        const std::lock_guard<std::mutex> held{lock_};
        opened = true;
        awake.notify_all();
    }

    void partial(const core::AsrPartial &) override {}

    void turn(const core::AsrTurn &) override {}

    void gap(std::chrono::milliseconds, std::chrono::milliseconds) override {}

    void spent(std::int64_t) override {}

    void finished(const core::AsrResult &said) override
    {
        const std::lock_guard<std::mutex> held{lock_};
        over = true;
        result = said;
        awake.notify_all();
    }

    std::mutex lock_;
    std::condition_variable awake;
    bool opened{false};
    bool over{false};
    core::AsrResult result;
};

}

core::AsrProbe ProtocolRecogniser::probe(const core::AsrEndpoint &where)
{

    core::StreamingOptions options;
    options.wire = core::wireFor(asked_->capabilities(where), 16000)
                       .value_or(core::AsrWireFormat{core::AsrEncoding::Pcm16, 16000, 1});

    Probing watching;
    auto session = std::make_unique<StreamingSession>(sockets_, make_(), where, options, watching,
                                                      core::AsrCancel{});

    {
        std::unique_lock<std::mutex> held{watching.lock_};
        (void)watching.awake.wait_for(held, 8s,
                                      [&watching] { return watching.opened || watching.over; });
    }

    core::AsrProbe answer;
    {
        const std::lock_guard<std::mutex> held{watching.lock_};
        answer.reachable = watching.opened;
        answer.status = watching.opened ? core::AsrStatus::Ok : watching.result.status;
        answer.detail = watching.opened
                            ? std::string{"the socket opened and the configuration was accepted"}
                            : watching.result.error;
    }

    session.reset();
    if (answer.detail.empty()) {
        answer.detail = answer.reachable ? "reachable" : "nothing answered";
    }
    return answer;
}

}
