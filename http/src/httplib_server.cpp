// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/http/httplib_server.hpp>

#include <softphone/core/logging.hpp>

#include <httplib.h>

#include <atomic>
#include <thread>
#include <utility>

namespace softphone::http {

struct HttplibServer::Impl {
    explicit Impl(Answer handler) : answer{std::move(handler)} {}

    Answer answer;
    httplib::Server server;
    std::thread thread;
    std::atomic<std::uint16_t> port{0};
};

namespace {

[[nodiscard]] core::ApiRequest asRequest(const httplib::Request &request)
{
    core::ApiRequest wanted;
    wanted.method = request.method;
    wanted.path = request.path;
    wanted.body = request.body;

    const auto question = request.target.find('?');
    if (question != std::string::npos) {
        wanted.query = request.target.substr(question + 1);
    }

    for (const auto &[name, value] : request.headers) {
        wanted.headers.emplace_back(name, value);
    }
    return wanted;
}

}

HttplibServer::HttplibServer(Answer answer) : impl_{std::make_unique<Impl>(std::move(answer))} {}

HttplibServer::~HttplibServer()
{
    stop();
}

bool HttplibServer::start(std::uint16_t port)
{
    stop();

    const auto forward = [this](const httplib::Request &request, httplib::Response &response) {
        const auto answered = impl_->answer(asRequest(request));

        if (!answered.reader || answered.status != 200) {
            response.status = answered.status;
        }

        for (const auto &[name, value] : answered.headers) {
            response.set_header(name, value);
        }

        if (answered.reader) {

            const auto read = answered.reader;
            response.set_content_provider(
                answered.size, answered.contentType,
                [read](std::size_t offset, std::size_t length, httplib::DataSink &sink) {
                    std::string piece;
                    if (!read(offset, length, piece)) {
                        return false;
                    }
                    return sink.write(piece.data(), piece.size());
                });
            return;
        }

        response.set_content(answered.body, answered.contentType);
    };

    impl_->server.Get(".*", forward);
    impl_->server.Post(".*", forward);
    impl_->server.Put(".*", forward);
    impl_->server.Delete(".*", forward);

    impl_->server.set_socket_options([](socket_t socket) {
        const int yes = 1;

#ifdef _WIN32
        const int option = SO_EXCLUSIVEADDRUSE;
#else
        const int option = SO_REUSEADDR;
#endif

        static_cast<void>(::setsockopt(socket, SOL_SOCKET, option,
                                       reinterpret_cast<const char *>(&yes),
                                       static_cast<int>(sizeof(yes))));
    });

    std::uint16_t bound = port;
    if (port == 0) {
        const auto given = impl_->server.bind_to_any_port("127.0.0.1");
        bound = given > 0 ? static_cast<std::uint16_t>(given) : 0;
    } else if (!impl_->server.bind_to_port("127.0.0.1", port)) {
        bound = 0;
    }

    if (bound == 0) {
        core::Logger::instance().log(core::LogLevel::Warning, "api",
                                     "port " + std::to_string(port) +
                                         " is taken; the local API is not running");
        return false;
    }

    impl_->port = bound;
    impl_->thread = std::thread{[this]() { impl_->server.listen_after_bind(); }};
    impl_->server.wait_until_ready();

    core::Logger::instance().log(core::LogLevel::Info, "api",
                                 "listening on 127.0.0.1:" + std::to_string(bound));
    return true;
}

void HttplibServer::stop()
{
    if (!impl_->thread.joinable()) {
        return;
    }

    impl_->server.stop();
    impl_->thread.join();
    impl_->port = 0;
}

bool HttplibServer::running() const noexcept
{
    return impl_->port != 0;
}

std::uint16_t HttplibServer::port() const noexcept
{
    return impl_->port;
}

}
