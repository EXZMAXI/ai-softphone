// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/http/httplib_http_client.hpp>

#include "machine_trust.hpp"
#include "split_url.hpp"

#include <softphone/core/http.hpp>
#include <softphone/core/version.hpp>

#include <httplib.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <fstream>
#include <memory>
#include <mutex>
#include <string>
#include <system_error>
#include <thread>

namespace softphone::http {
namespace {

constexpr std::size_t kChunk = 64 * 1024;

constexpr auto kAskAgain = std::chrono::milliseconds{100};

struct Flight {
    explicit Flight(const std::string &origin) : client{origin} {}

    httplib::Client client;

    std::mutex guard;
    std::condition_variable done;
    bool finished{false};
    httplib::Result answer{nullptr, httplib::Error::Unknown};

    std::atomic<bool> abandoned{false};
};

[[nodiscard]] core::HttpResponse refuse(std::string reason)
{
    core::HttpResponse response;
    response.status = 0;
    response.error = std::move(reason);
    return response;
}

[[nodiscard]] std::uint64_t sizeOf(const std::filesystem::path &file)
{
    std::error_code failed;
    const auto size = std::filesystem::file_size(file, failed);
    return failed ? 0 : static_cast<std::uint64_t>(size);
}

[[nodiscard]] httplib::ContentProviderWithoutLength readerFor(const std::filesystem::path &path,
                                                              const core::HttpCancel &cancelled)
{
    const auto total = sizeOf(path);
    auto stream = std::make_shared<std::ifstream>();

    return [stream, path, total, cancelled](std::size_t offset, httplib::DataSink &sink) -> bool {
        if (cancelled && cancelled()) {
            return false;
        }
        if (!stream->is_open()) {
            stream->open(path, std::ios::binary);
            if (!stream->is_open()) {
                return false;
            }
        }
        if (static_cast<std::uint64_t>(offset) >= total) {
            sink.done();
            return true;
        }
        stream->seekg(static_cast<std::streamoff>(offset));
        std::array<char, kChunk> buffer{};
        const auto want =
            static_cast<std::streamsize>(std::min<std::uint64_t>(buffer.size(), total - offset));
        stream->read(buffer.data(), want);
        const auto got = stream->gcount();
        if (got <= 0) {
            sink.done();
            return true;
        }
        sink.write(buffer.data(), static_cast<std::size_t>(got));
        return true;
    };
}

}

HttplibHttpClient::HttplibHttpClient() = default;

HttplibHttpClient::~HttplibHttpClient() = default;

bool HttplibHttpClient::supportsTls() noexcept
{
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    return true;
#else
    return false;
#endif
}

struct Piece {
    std::string text;
    std::filesystem::path file;
    std::uint64_t size{0};
};

struct Multipart {
    std::vector<Piece> pieces;
    std::uint64_t length{0};
    std::string contentType;
};

[[nodiscard]] Multipart multipartOf(const std::vector<core::HttpPart> &parts)
{
    const auto boundary = httplib::detail::make_multipart_data_boundary();

    Multipart body;
    body.contentType = httplib::detail::serialize_multipart_formdata_get_content_type(boundary);

    const auto add = [&body](Piece piece) {
        piece.size = piece.file.empty() ? piece.text.size() : piece.size;
        body.pieces.push_back(std::move(piece));
    };

    httplib::MultipartFormDataItems values;
    for (const auto &part : parts) {
        if (part.file.empty()) {
            values.push_back({part.name, part.value, part.fileName, part.contentType});
        }
    }
    if (!values.empty()) {
        add({httplib::detail::serialize_multipart_formdata(values, boundary, false), {}, 0});
    }

    for (const auto &part : parts) {
        if (part.file.empty()) {
            continue;
        }

        const httplib::MultipartFormData named{part.name, {}, part.fileName, part.contentType};
        add({httplib::detail::serialize_multipart_formdata_item_begin(named, boundary), {}, 0});
        add({{}, part.file, sizeOf(part.file)});
        add({httplib::detail::serialize_multipart_formdata_item_end(), {}, 0});
    }

    add({httplib::detail::serialize_multipart_formdata_finish(boundary), {}, 0});

    for (const auto &piece : body.pieces) {
        body.length += piece.size;
    }
    return body;
}

[[nodiscard]] httplib::ContentProvider providerFor(Multipart body, core::HttpCancel cancelled)
{
    auto stream = std::make_shared<std::ifstream>();
    auto open = std::make_shared<std::filesystem::path>();

    return [body = std::move(body), cancelled = std::move(cancelled), stream,
            open](std::size_t offset, std::size_t, httplib::DataSink &sink) -> bool {
        if (cancelled && cancelled()) {
            return false;
        }

        std::uint64_t at = 0;
        for (const auto &piece : body.pieces) {
            if (static_cast<std::uint64_t>(offset) >= at + piece.size) {
                at += piece.size;
                continue;
            }

            const auto within = static_cast<std::size_t>(static_cast<std::uint64_t>(offset) - at);

            if (piece.file.empty()) {
                sink.write(piece.text.data() + within, piece.text.size() - within);
                return true;
            }

            if (*open != piece.file) {
                stream->close();
                stream->clear();
                stream->open(piece.file, std::ios::binary);
                *open = piece.file;
            }
            if (!stream->is_open()) {
                return false;
            }

            stream->seekg(static_cast<std::streamoff>(within));
            std::array<char, kChunk> buffer{};
            const auto want = static_cast<std::streamsize>(
                std::min<std::uint64_t>(buffer.size(), piece.size - within));
            stream->read(buffer.data(), want);
            const auto got = stream->gcount();
            if (got <= 0) {

                return false;
            }
            sink.write(buffer.data(), static_cast<std::size_t>(got));
            return true;
        }

        return true;
    };
}

core::HttpResponse HttplibHttpClient::send(const core::HttpRequest &request,
                                           const core::HttpCancel &cancelled)
{
    if (cancelled && cancelled()) {
        return refuse("cancelled before it was sent");
    }

    const auto parts = split(request.url);
    if (!parts.ok) {
        auto refusal = refuse("not an address this phone can send to: " + request.url);
        refusal.status = 400;
        return refusal;
    }
    if (parts.tls && !supportsTls()) {
        auto refusal = refuse("this build has no TLS, so it cannot reach " + request.url);
        refusal.status = 400;
        return refusal;
    }

    auto flight = std::make_shared<Flight>(parts.origin);

    trustTheMachine(flight->client, parts.host);

    flight->client.set_connection_timeout(std::chrono::seconds{30});
    flight->client.set_read_timeout(request.timeout);
    flight->client.set_write_timeout(request.timeout);

    flight->client.set_follow_location(false);
    flight->client.set_default_headers({{"User-Agent", std::string{core::userAgent()}}});

    const core::HttpCancel stop = [flight]() {
        return flight->abandoned.load(std::memory_order_acquire);
    };

    std::thread{[flight, target = parts.target, request, stop]() {

        httplib::Headers headers;
        for (const auto &[name, value] : request.headers) {
            headers.emplace(core::headerSafeName(name), core::headerSafeValue(value));
        }

        httplib::Result answer{nullptr, httplib::Error::Unknown};

        if (!request.parts.empty()) {

            auto body = multipartOf(request.parts);
            const auto length = static_cast<std::size_t>(body.length);
            const auto type = body.contentType;
            answer = flight->client.Post(target, headers, length,
                                         providerFor(std::move(body), stop), type);
        } else if (!request.bodyFile.empty()) {

            const auto length = static_cast<std::size_t>(sizeOf(request.bodyFile));
            auto reader = readerFor(request.bodyFile, stop);
            answer = flight->client.Post(
                target, headers, length,
                [reader = std::move(reader)](std::size_t offset, std::size_t,
                                             httplib::DataSink &sink) mutable {
                    return reader(offset, sink);
                },
                request.contentType);
        } else {
            switch (request.verb) {
            case core::HttpVerb::Get:
                answer = flight->client.Get(target, headers);
                break;
            case core::HttpVerb::Put:
                answer = flight->client.Put(target, headers, request.body, request.contentType);
                break;
            case core::HttpVerb::Remove:
                answer = flight->client.Delete(target, headers, request.body, request.contentType);
                break;
            case core::HttpVerb::Post:
                answer = flight->client.Post(target, headers, request.body, request.contentType);
                break;
            }
        }

        {
            const std::lock_guard held{flight->guard};
            flight->answer = std::move(answer);
            flight->finished = true;
        }
        flight->done.notify_all();
    }}.detach();

    httplib::Result answer{nullptr, httplib::Error::Unknown};
    {
        std::unique_lock held{flight->guard};
        while (!flight->finished) {
            if (cancelled && cancelled()) {
                flight->abandoned.store(true, std::memory_order_release);
                held.unlock();

                flight->client.stop();
                return refuse("cancelled while it was in flight");
            }
            flight->done.wait_for(held, kAskAgain);
        }
        answer = std::move(flight->answer);
    }

    if (cancelled && cancelled()) {
        return refuse("cancelled while it was in flight");
    }
    if (!answer) {
        return refuse(httplib::to_string(answer.error()));
    }

    core::HttpResponse response;
    response.status = answer->status;
    response.body = answer->body;
    response.headers.reserve(answer->headers.size());
    for (const auto &[name, value] : answer->headers) {
        response.headers.emplace_back(name, value);
    }
    return response;
}

}
