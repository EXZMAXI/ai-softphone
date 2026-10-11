// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/http/streaming_yandex.hpp>

#include <softphone/core/logging.hpp>

#include "grpc_frame.hpp"
#include "protobuf_wire.hpp"

#include <algorithm>
#include <mutex>
#include <optional>
#include <ranges>
#include <utility>

namespace softphone::http {
namespace {

using namespace std::chrono_literals;
using namespace protobuf;

constexpr int kLinear16 = 1;

constexpr int kRealTime = 1;

constexpr int kWhitelist = 1;

constexpr int kNormalise = 1;

[[nodiscard]] std::string paramOf(const core::AsrEndpoint &where, std::string_view name)
{
    const auto found =
        std::ranges::find_if(where.params, [name](const auto &pair) { return pair.first == name; });
    return found == where.params.end() ? std::string{} : found->second;
}

[[nodiscard]] core::AsrWord readWord(std::string_view message)
{
    core::AsrWord word;
    Reader reader{message};
    while (const auto field = reader.next()) {
        switch (field->number) {
        case 1:
            word.text = std::string{field->bytes};
            break;
        case 2:
            word.start = std::chrono::milliseconds{asInt(field->number64)};
            break;
        case 3:
            word.end = std::chrono::milliseconds{asInt(field->number64)};
            break;
        default:
            break;
        }
    }
    return word;
}

void readAlternative(std::string_view message, yandex::Heard &into)
{
    Reader reader{message};
    while (const auto field = reader.next()) {
        switch (field->number) {
        case 1:
            into.words.push_back(readWord(field->bytes));
            break;
        case 2:

            into.text += std::string{field->bytes};
            break;
        case 3:
            if (!into.begun) {
                into.start = std::chrono::milliseconds{asInt(field->number64)};
                into.begun = true;
            }
            break;
        case 4:
            into.end = std::chrono::milliseconds{asInt(field->number64)};
            break;
        case 5:
            if (field->wire == Wire::Fixed64) {
                into.confidence = asDouble(field->number64);
            }
            break;
        case 6: {

            Reader inner{field->bytes};
            std::string code;
            double probability = 0.0;
            while (const auto part = inner.next()) {
                if (part->number == 1) {
                    code = std::string{part->bytes};
                } else if (part->number == 2 && part->wire == Wire::Fixed64) {
                    probability = asDouble(part->number64);
                }
            }
            if (!code.empty() && (into.language.empty() || probability > into.confidence)) {
                into.language = code;
            }
            break;
        }
        default:
            break;
        }
    }
}

void readUpdate(std::string_view message, yandex::Heard &into)
{
    Reader reader{message};
    while (const auto field = reader.next()) {
        if (field->number == 1) {
            readAlternative(field->bytes, into);
        }
    }
}

}

namespace yandex {

std::string sessionOptions(const core::AsrEndpoint &where, const core::StreamingOptions &options)
{
    std::string raw;
    putEnum(raw, 1, kLinear16);
    putInt(raw, 2, static_cast<std::int64_t>(options.wire.sampleRate));

    putInt(raw, 3, 1);

    std::string format;
    putMessage(format, 1, raw);

    std::string model;

    if (!where.model.empty()) {
        putBytes(model, 1, where.model);
    }
    putMessage(model, 2, format);

    std::string normalisation;
    putEnum(normalisation, 1, kNormalise);
    putMessage(model, 3, normalisation);

    if (!options.language.empty()) {

        std::string restriction;
        putEnum(restriction, 1, kWhitelist);
        putBytes(restriction, 2, options.language);
        putMessage(model, 4, restriction);
    }

    putEnum(model, 5, kRealTime);

    std::string session;
    putMessage(session, 1, model);

    std::string request;
    putMessage(request, 1, session);
    return request;
}

std::string audioChunk(std::string_view pcm)
{
    std::string chunk;
    putBytes(chunk, 1, pcm);

    std::string request;
    putMessage(request, 2, chunk);
    return request;
}

Heard readResponse(std::string_view message)
{
    Heard heard;
    Reader reader{message};

    while (const auto field = reader.next()) {
        switch (field->number) {
        case 4:
            heard.shape = "partial";
            heard.understood = true;
            heard.partial = true;
            readUpdate(field->bytes, heard);
            break;

        case 5:
            heard.shape = "final";
            heard.understood = true;
            heard.settled = true;
            readUpdate(field->bytes, heard);
            break;

        case 7: {

            heard.shape = "final_refinement";
            heard.understood = true;

            heard.refines = 0u;

            Reader inner{field->bytes};
            while (const auto part = inner.next()) {
                if (part->number == 1 && part->wire == Wire::Varint) {
                    heard.refines = static_cast<unsigned>(part->number64);
                } else if (part->number == 2) {
                    heard.settled = true;
                    readUpdate(part->bytes, heard);
                }
            }
            break;
        }

        case 6:

            heard.shape = "eou_update";
            heard.understood = true;
            break;

        case 8: {

            heard.shape = "status_code";
            heard.understood = true;
            Reader inner{field->bytes};
            while (const auto part = inner.next()) {
                if (part->number == 2) {
                    heard.said = std::string{part->bytes};
                }
            }
            break;
        }

        case 1:
        case 2:
        case 3:
        case 9:

            heard.understood = true;
            if (heard.shape.empty()) {
                heard.shape = "progress";
            }
            break;

        default:

            if (heard.shape.empty()) {
                heard.shape = "field " + std::to_string(field->number);
            }
            break;
        }
    }

    if (reader.broken()) {
        heard.understood = false;
        heard.shape = "(not protobuf)";
    }
    return heard;
}

}

namespace {

class YandexSession final : public core::IStreamingAsrSession, public core::IGrpcStreamSink {
public:
    YandexSession(core::IGrpcTransport &transport, const core::AsrEndpoint &where,
                  core::StreamingOptions options, core::IStreamingAsrSink &sink,
                  core::AsrCancel cancelled)
        : sink_{sink}, options_{std::move(options)}, cancelled_{std::move(cancelled)},
          model_{where.model}
    {
        core::IGrpcTransport::Call call;

        call.host =
            where.url.empty() ? std::string{"https://stt.api.cloud.yandex.net:443"} : where.url;
        call.method = "/speechkit.stt.v3.Recognizer/RecognizeStreaming";

        if (!where.token.empty()) {

            const auto scheme = paramOf(where, "auth") == "iam" ? "Bearer " : "Api-Key ";
            call.headers.emplace_back("authorization", scheme + where.token);
        }
        if (const auto folder = paramOf(where, "folder_id"); !folder.empty()) {
            call.headers.emplace_back("x-folder-id", folder);
        }

        opening_ = yandex::sessionOptions(where, options_);
        stream_ = transport.open(call, *this);
    }

    ~YandexSession() override
    {

        stream_.reset();
        tell(core::AsrStatus::Cancelled, "closed from here");
    }

    [[nodiscard]] bool send(std::chrono::milliseconds position,
                            std::span<const std::int16_t> samples) override
    {
        if (cancelled_ && cancelled_()) {
            return false;
        }
        const std::lock_guard<std::mutex> held{lock_};
        if (!stream_ || !ready_) {
            return false;
        }
        furthest_ = std::max(furthest_, position);

        const auto bytes = std::as_bytes(samples);
        wrapped_ = yandex::audioChunk(
            std::string_view{reinterpret_cast<const char *>(bytes.data()), bytes.size()});
        return stream_->send(wrapped_);
    }

    void drain() override
    {
        {
            const std::lock_guard<std::mutex> held{lock_};
            if (!stream_ || drained_) {
                return;
            }
            drained_ = true;
        }

        stream_->finishSending();
    }

    [[nodiscard]] bool listening() const override
    {
        const std::lock_guard<std::mutex> held{lock_};
        return stream_ != nullptr && ready_ && !over_;
    }

    void opened() override
    {
        std::string first;
        {
            const std::lock_guard<std::mutex> held{lock_};
            if (!stream_) {
                return;
            }
            first = opening_;
        }

        if (stream_->send(first)) {
            const std::lock_guard<std::mutex> held{lock_};
            ready_ = true;
        }
        sink_.listening();
    }

    void message(std::string_view payload) override
    {
        const auto heard = yandex::readResponse(payload);

        if (core::Logger::instance().level() >= core::LogLevel::Trace) {
            auto line = "yandex ← " + heard.shape;
            if (!heard.text.empty()) {
                line += ": " + heard.text;
            }
            if (heard.refines.has_value()) {
                line += " (refines " + std::to_string(*heard.refines) + ")";
            }
            core::Logger::instance().log(core::LogLevel::Trace, "realtime", line);
        }

        if (!heard.understood) {
            sayOnce(heard.shape);
            return;
        }
        if (!heard.said.empty()) {
            sayRemark(heard.said);
        }

        std::optional<unsigned> ordinal;
        if (heard.settled) {
            ordinal = heard.refines;
            if (!ordinal.has_value()) {
                ordinal = finals_++;
            }
        }

        if (heard.text.empty()) {
            return;
        }

        if (heard.partial && !heard.settled) {
            core::AsrPartial said;

            said.voice = options_.voice;
            said.start = heard.start;
            said.end = heard.end;
            said.text = heard.text;
            said.words = heard.words;
            sink_.partial(said);
            return;
        }

        core::AsrTurn said;
        said.voice = options_.voice;
        said.start = heard.start;
        said.end = heard.end;
        said.text = heard.text;
        said.words = heard.words;
        said.language = heard.language;
        said.confidence = heard.confidence;

        said.boundary = core::AsrTurn::Boundary::Provider;

        {
            const std::lock_guard<std::mutex> held{lock_};

            said.ordinal = ordinal;

            const auto already = std::ranges::find_if(
                turns_, [&said](const core::AsrTurn &had) { return had.ordinal == said.ordinal; });
            if (already != turns_.end()) {
                *already = said;
            } else {
                turns_.push_back(said);
            }
        }
        sink_.turn(said);
    }

    void closed(int status, std::string_view said) override
    {

        const auto read = [status] {
            switch (status) {
            case 0:
                return core::AsrStatus::Ok;
            case 1:
                return core::AsrStatus::Cancelled;
            case 3:
            case 12:
                return core::AsrStatus::Unsupported;
            case 7:
            case 16:
                return core::AsrStatus::Unauthorised;
            case 4:
                return core::AsrStatus::Timeout;
            case 8:
            case 14:
                return core::AsrStatus::Unavailable;
            default:
                return core::AsrStatus::Failed;
            }
        }();
        tell(read, said);
    }

private:

    void tell(core::AsrStatus status, std::string_view said)
    {
        core::AsrResult result;
        {
            const std::lock_guard<std::mutex> held{lock_};
            if (told_) {
                return;
            }
            told_ = true;
            over_ = true;
            ready_ = false;

            result.status = status;
            result.error = std::string{said};
            result.detectedLanguage = options_.language;
            result.model = model_;
            result.audio = furthest_;
            result.segments.reserve(turns_.size());
            for (const auto &turn : turns_) {
                core::AsrSegment segment;
                segment.start = turn.start;
                segment.end = turn.end;
                segment.text = turn.text;
                segment.confidence = turn.confidence;
                segment.words = turn.words;
                result.segments.push_back(std::move(segment));
            }
        }
        sink_.finished(result);
    }

private:
    void sayOnce(const std::string &shape)
    {
        {
            const std::lock_guard<std::mutex> held{lock_};
            if (std::ranges::find(unread_, shape) != unread_.end()) {
                return;
            }
            unread_.push_back(shape);
        }
        core::Logger::instance().log(core::LogLevel::Warning, "realtime",
                                     "yandex: nothing in this adapter reads a message of shape '" +
                                         shape + "' — the protocol may have moved");
    }

    void sayRemark(const std::string &said)
    {
        {
            const std::lock_guard<std::mutex> held{lock_};
            if (std::ranges::find(remarked_, said) != remarked_.end()) {
                return;
            }
            remarked_.push_back(said);
        }
        core::Logger::instance().log(core::LogLevel::Info, "realtime",
                                     "yandex remarked, and went on: " + said);
    }

    core::IStreamingAsrSink &sink_;
    core::StreamingOptions options_;
    core::AsrCancel cancelled_;
    std::string model_;
    std::string opening_;
    std::string wrapped_;

    mutable std::mutex lock_;
    std::unique_ptr<core::IGrpcStream> stream_;
    std::vector<core::AsrTurn> turns_;
    std::vector<std::string> unread_;
    std::vector<std::string> remarked_;
    std::chrono::milliseconds furthest_{0};
    bool ready_{false};
    bool drained_{false};
    bool over_{false};

    unsigned finals_{0};

    bool told_{false};
};

class YandexStreaming final : public core::IStreamingAsr {
public:
    explicit YandexStreaming(core::IGrpcTransport &transport) : transport_{transport} {}

    [[nodiscard]] std::string_view type() const noexcept override
    {
        return "yandex";
    }

    [[nodiscard]] core::StreamingCapabilities capabilities(const core::AsrEndpoint &) const override
    {
        core::StreamingCapabilities can;

        can.marksTurns = true;
        can.interim = true;
        can.translates = false;
        can.languageHint = true;
        can.detectsLanguage = true;

        can.revisesTurns = true;

        can.diarisation = false;

        can.accepts = {{core::AsrEncoding::Pcm16, 16000, 1},
                       {core::AsrEncoding::Pcm16, 8000, 1},
                       {core::AsrEncoding::Pcm16, 48000, 1}};

        can.reportsCost = false;
        can.idleCloses = 0s;

        can.languages = {"ru", "en", "kk", "uz", "de", "fr", "it", "es", "pt", "nl",
                         "pl", "tr", "fi", "sv", "da", "nb", "he", "ar", "hy", "ka"};
        return can;
    }

    [[nodiscard]] std::unique_ptr<core::IStreamingAsrSession>
    open(const core::AsrEndpoint &where, const core::StreamingOptions &options,
         core::IStreamingAsrSink &sink, const core::AsrCancel &cancelled) override
    {
        return std::make_unique<YandexSession>(transport_, where, options, sink, cancelled);
    }

    [[nodiscard]] core::AsrProbe probe(const core::AsrEndpoint &where) override
    {

        class Quiet final : public core::IStreamingAsrSink {
        public:
            void listening() override
            {
                opened = true;
            }

            void partial(const core::AsrPartial &) override {}

            void turn(const core::AsrTurn &) override {}

            void gap(std::chrono::milliseconds, std::chrono::milliseconds) override {}

            void spent(std::int64_t) override {}

            void finished(const core::AsrResult &said) override
            {
                answer = said;
            }

            bool opened{false};
            std::optional<core::AsrResult> answer;
        };

        Quiet quiet;
        core::StreamingOptions asking;
        asking.wire = {core::AsrEncoding::Pcm16, 16000, 1};
        {
            auto session = open(where, asking, quiet, {});
            session->drain();
        }

        core::AsrProbe probe;
        if (quiet.answer && !core::succeeded(quiet.answer->status) &&
            quiet.answer->status != core::AsrStatus::Cancelled) {
            probe.status = quiet.answer->status;
            probe.detail = quiet.answer->error;
            if (probe.detail.empty()) {
                probe.detail = "the recogniser would not open a stream";
            }
            return probe;
        }
        probe.reachable = quiet.opened;
        probe.status = quiet.opened ? core::AsrStatus::Ok : core::AsrStatus::Unavailable;
        probe.detail = quiet.opened ? "SpeechKit accepted the stream"
                                    : "nothing accepted a stream at that address";
        return probe;
    }

private:
    core::IGrpcTransport &transport_;
};

}

std::unique_ptr<core::IStreamingAsr> makeYandexStreaming(core::IGrpcTransport &transport)
{
    return std::make_unique<YandexStreaming>(transport);
}

}
