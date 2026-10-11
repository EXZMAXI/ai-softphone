// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/http/nghttp2_grpc.hpp>

#include <softphone/core/logging.hpp>

#include "grpc_frame.hpp"
#include "machine_trust.hpp"
#include "split_url.hpp"

#include <nghttp2/nghttp2.h>

#include <openssl/err.h>
#include <openssl/ssl.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <netdb.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include <algorithm>
#include <array>
#include <cctype>
#include <condition_variable>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace softphone::http {
namespace {

using namespace std::chrono_literals;

constexpr int kGrpcOk = 0;
constexpr int kGrpcUnknown = 2;
constexpr int kGrpcUnavailable = 14;

constexpr int kPollMs = 50;

constexpr unsigned char kAlpnH2[] = {2, 'h', '2'};

#ifdef _WIN32
using Socket = SOCKET;
using AddressLength = int;
constexpr Socket kNoSocket = INVALID_SOCKET;
constexpr int kBothWays = SD_BOTH;

inline int pollSockets(pollfd *watching, unsigned long count, int waitMs)
{
    return ::WSAPoll(watching, count, waitMs);
}

inline void closeSocket(Socket which)
{
    ::closesocket(which);
}

[[nodiscard]] inline bool madeSocket(Socket which)
{
    return which != INVALID_SOCKET;
}

struct Winsock {
    Winsock()
    {
        WSADATA about{};
        ready = ::WSAStartup(MAKEWORD(2, 2), &about) == 0;
    }

    ~Winsock()
    {
        if (ready) {
            ::WSACleanup();
        }
    }

    bool ready{false};
};

[[nodiscard]] bool winsockReady()
{
    static const Winsock started;
    return started.ready;
}
#else
using Socket = int;
using AddressLength = socklen_t;
constexpr Socket kNoSocket = -1;
constexpr int kBothWays = SHUT_RDWR;

inline int pollSockets(pollfd *watching, unsigned long count, int waitMs)
{
    return ::poll(watching, static_cast<nfds_t>(count), waitMs);
}

inline void closeSocket(Socket which)
{
    ::close(which);
}

[[nodiscard]] inline bool madeSocket(Socket which)
{
    return which >= 0;
}

[[nodiscard]] bool winsockReady()
{
    return true;
}
#endif

[[nodiscard]] std::string opensslSaid()
{
    const auto code = ERR_get_error();
    if (code == 0) {
        return {};
    }
    char buffer[256] = {};
    ERR_error_string_n(code, buffer, sizeof(buffer));
    return std::string{buffer};
}

class Stream final : public core::IGrpcStream {
public:
    Stream(const core::IGrpcTransport::Call &call, core::IGrpcStreamSink &sink)
        : sink_{sink}, call_{call}
    {
        pump_ = std::thread{[this] { run(); }};
    }

    ~Stream() override
    {
        {
            const std::lock_guard<std::mutex> held{lock_};
            leaving_ = true;
        }
        wake_.notify_all();

        if (madeSocket(socket_)) {
            ::shutdown(socket_, kBothWays);
        }
        if (pump_.joinable()) {
            pump_.join();
        }
        if (session_ != nullptr) {
            nghttp2_session_del(session_);
            session_ = nullptr;
        }
        if (ssl_ != nullptr) {
            SSL_free(ssl_);
            ssl_ = nullptr;
        }
        if (context_ != nullptr) {
            SSL_CTX_free(context_);
            context_ = nullptr;
        }
        if (madeSocket(socket_)) {
            closeSocket(socket_);
            socket_ = kNoSocket;
        }
    }

    [[nodiscard]] bool send(std::string_view payload) override
    {
        {
            const std::lock_guard<std::mutex> held{lock_};
            if (leaving_ || over_ || sendingDone_) {
                return false;
            }
            outbound_.push_back(grpcwire::framed(payload));
        }
        wake_.notify_all();
        return true;
    }

    void finishSending() override
    {
        {
            const std::lock_guard<std::mutex> held{lock_};
            if (sendingDone_) {
                return;
            }
            sendingDone_ = true;
        }
        wake_.notify_all();
    }

    [[nodiscard]] bool open() const override
    {
        const std::lock_guard<std::mutex> held{lock_};
        return running_ && !over_ && !leaving_;
    }

private:

    static nghttp2_ssize readBody(nghttp2_session *, std::int32_t, std::uint8_t *into,
                                  std::size_t room, std::uint32_t *flags,
                                  nghttp2_data_source *source, void *)
    {
        auto *const self = static_cast<Stream *>(source->ptr);
        std::unique_lock<std::mutex> held{self->lock_};

        if (self->leaving_) {
            *flags |= NGHTTP2_DATA_FLAG_EOF;
            return 0;
        }
        if (self->outbound_.empty()) {
            if (self->sendingDone_) {

                *flags |= NGHTTP2_DATA_FLAG_EOF;
                return 0;
            }

            self->deferred_ = true;
            return NGHTTP2_ERR_DEFERRED;
        }

        auto &front = self->outbound_.front();
        const auto taking = std::min(room, front.size() - self->sentOfFront_);
        std::memcpy(into, front.data() + self->sentOfFront_, taking);
        self->sentOfFront_ += taking;
        if (self->sentOfFront_ >= front.size()) {
            self->outbound_.pop_front();
            self->sentOfFront_ = 0;
        }
        return static_cast<nghttp2_ssize>(taking);
    }

    static int onHeader(nghttp2_session *, const nghttp2_frame *frame, const std::uint8_t *name,
                        std::size_t nameLength, const std::uint8_t *value, std::size_t valueLength,
                        std::uint8_t, void *user)
    {
        auto *const self = static_cast<Stream *>(user);
        if (frame->hd.type != NGHTTP2_HEADERS) {
            return 0;
        }
        const auto field = std::string_view{reinterpret_cast<const char *>(name), nameLength};
        const auto said = std::string_view{reinterpret_cast<const char *>(value), valueLength};

        if (field == ":status") {
            self->httpStatus_ = std::atoi(std::string{said}.c_str());
        } else if (field == "grpc-status") {
            self->grpcStatus_ = std::atoi(std::string{said}.c_str());
        } else if (field == "grpc-message") {
            self->grpcSaid_ = unescape(said);
        } else if (field == "content-type" &&
                   said.find("application/grpc") == std::string_view::npos) {
            self->contentTypeWrong_ = std::string{said};
        }
        return 0;
    }

    static int onFrame(nghttp2_session *, const nghttp2_frame *frame, void *user)
    {
        auto *const self = static_cast<Stream *>(user);
        if (frame->hd.type == NGHTTP2_HEADERS && frame->headers.cat == NGHTTP2_HCAT_RESPONSE) {
            self->accepted();
        }
        return 0;
    }

    static int onData(nghttp2_session *, std::uint8_t, std::int32_t, const std::uint8_t *data,
                      std::size_t length, void *user)
    {
        auto *const self = static_cast<Stream *>(user);
        self->inbound_.append(reinterpret_cast<const char *>(data), length);

        const auto took = grpcwire::take(self->inbound_);
        if (took.compressed) {
            self->refuse(kGrpcUnknown,
                         "the recogniser sent a compressed message and this build asked for none");
            return 0;
        }
        for (const auto &message : took.messages) {
            self->sink_.message(message);
        }
        return 0;
    }

    static int onClose(nghttp2_session *, std::int32_t, std::uint32_t code, void *user)
    {
        auto *const self = static_cast<Stream *>(user);
        self->ended(code);
        return 0;
    }

    [[nodiscard]] static std::string unescape(std::string_view from)
    {
        std::string out;
        out.reserve(from.size());
        for (std::size_t at = 0; at < from.size(); ++at) {
            if (from[at] != '%' || at + 2 >= from.size()) {
                out.push_back(from[at]);
                continue;
            }
            const auto digit = [](char which) -> int {
                if (which >= '0' && which <= '9') {
                    return which - '0';
                }
                if (which >= 'a' && which <= 'f') {
                    return which - 'a' + 10;
                }
                if (which >= 'A' && which <= 'F') {
                    return which - 'A' + 10;
                }
                return -1;
            };
            const auto high = digit(from[at + 1]);
            const auto low = digit(from[at + 2]);
            if (high < 0 || low < 0) {
                out.push_back(from[at]);
                continue;
            }
            out.push_back(static_cast<char>(high * 16 + low));
            at += 2;
        }
        return out;
    }

    void accepted()
    {
        bool tell = false;
        {
            const std::lock_guard<std::mutex> held{lock_};
            if (!told_) {
                told_ = true;
                tell = true;
            }
        }
        if (tell) {
            sink_.opened();
        }
    }

    void refuse(int status, std::string said)
    {
        {
            const std::lock_guard<std::mutex> held{lock_};
            if (over_) {
                return;
            }
            refusal_ = status;
            refusalSaid_ = std::move(said);
        }
    }

    void ended(std::uint32_t code)
    {
        const std::lock_guard<std::mutex> held{lock_};
        closedBy_ = code;
        streamGone_ = true;
    }

    void run()
    {
        if (!connect()) {
            return;
        }

        {
            const std::lock_guard<std::mutex> held{lock_};
            running_ = true;
        }

        std::string pending;
        for (;;) {
            {
                const std::lock_guard<std::mutex> held{lock_};
                if (leaving_ || streamGone_ || refusal_.has_value()) {
                    break;
                }
                if (deferred_ && (!outbound_.empty() || sendingDone_)) {
                    deferred_ = false;
                    nghttp2_session_resume_data(session_, streamId_);
                }
            }

            if (nghttp2_session_want_write(session_) != 0 && !writeOut()) {
                break;
            }

            pollfd watching{};
            watching.fd = socket_;
            watching.events = POLLIN;
            if (nghttp2_session_want_write(session_) != 0) {
                watching.events |= POLLOUT;
            }
            const auto ready = pollSockets(&watching, 1, kPollMs);
            if (ready < 0) {
                refuse(kGrpcUnavailable, "the connection to the recogniser was lost");
                break;
            }
            if (ready > 0 && (watching.revents & (POLLIN | POLLHUP | POLLERR)) != 0) {
                if (!readIn()) {
                    break;
                }
            }

            if (nghttp2_session_want_read(session_) == 0 &&
                nghttp2_session_want_write(session_) == 0) {
                break;
            }
        }

        finish();
    }

    [[nodiscard]] bool connect()
    {
        const auto parts = split(call_.host);
        if (!parts.ok || parts.host.empty()) {
            say(kGrpcUnavailable, "this recogniser has no address");
            return false;
        }

        auto port = parts.tls ? 443 : 80;
        {
            const auto afterScheme = parts.origin.find("://");
            const auto from = afterScheme == std::string::npos ? 0 : afterScheme + 3;
            const auto bracket = parts.origin.rfind(']');
            const auto colon = parts.origin.rfind(':');
            if (colon != std::string::npos && colon > from &&
                (bracket == std::string::npos || colon > bracket)) {
                const auto digits = parts.origin.substr(colon + 1);
                if (!digits.empty() &&
                    std::all_of(digits.begin(), digits.end(),
                                [](unsigned char which) { return std::isdigit(which) != 0; })) {
                    port = std::atoi(digits.c_str());
                }
            }
        }

        if (!parts.tls) {

            say(kGrpcUnsupportedScheme, "a gRPC recogniser needs an https:// address");
            return false;
        }

        if (!winsockReady()) {
            say(kGrpcUnavailable, "this machine's socket library could not be started");
            return false;
        }

        addrinfo wanted{};
        wanted.ai_family = AF_UNSPEC;
        wanted.ai_socktype = SOCK_STREAM;
        addrinfo *found = nullptr;
        if (::getaddrinfo(parts.host.c_str(), std::to_string(port).c_str(), &wanted, &found) != 0 ||
            found == nullptr) {
            say(kGrpcUnavailable, "this address could not be resolved");
            return false;
        }

        for (auto *one = found; one != nullptr; one = one->ai_next) {
            socket_ = ::socket(one->ai_family, one->ai_socktype, one->ai_protocol);
            if (!madeSocket(socket_)) {
                continue;
            }
            if (::connect(socket_, one->ai_addr, static_cast<AddressLength>(one->ai_addrlen)) ==
                0) {
                break;
            }
            closeSocket(socket_);
            socket_ = kNoSocket;
        }
        ::freeaddrinfo(found);

        if (!madeSocket(socket_)) {
            say(kGrpcUnavailable, "nothing answered at that address");
            return false;
        }

        context_ = SSL_CTX_new(TLS_client_method());
        if (context_ == nullptr) {
            say(kGrpcUnavailable, "this build could not start TLS");
            return false;
        }
        SSL_CTX_set_min_proto_version(context_, TLS1_2_VERSION);
        SSL_CTX_set_verify(context_, SSL_VERIFY_PEER, nullptr);
        SSL_CTX_set_alpn_protos(context_, kAlpnH2, sizeof(kAlpnH2));

        core::Logger::instance().log(core::LogLevel::Debug, "grpc",
                                     "certificates: " + trustTheMachine(context_, parts.host));

        ssl_ = SSL_new(context_);
        if (ssl_ == nullptr) {
            say(kGrpcUnavailable, "this build could not start TLS");
            return false;
        }
        SSL_set_fd(ssl_, static_cast<int>(socket_));

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
        SSL_set_tlsext_host_name(ssl_, parts.host.c_str());
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
        SSL_set1_host(ssl_, parts.host.c_str());

        if (SSL_connect(ssl_) != 1) {
            auto said = opensslSaid();
            say(kGrpcUnavailable, said.empty()
                                      ? "the TLS handshake with this recogniser failed"
                                      : "the TLS handshake with this recogniser failed: " + said);
            return false;
        }

        const unsigned char *chosen = nullptr;
        unsigned chosenLength = 0;
        SSL_get0_alpn_selected(ssl_, &chosen, &chosenLength);
        if (chosen == nullptr || chosenLength != 2 || std::memcmp(chosen, "h2", 2) != 0) {
            say(kGrpcUnavailable, "this address does not speak HTTP/2, which gRPC requires");
            return false;
        }

        return startSession(parts.host);
    }

    [[nodiscard]] bool startSession(const std::string &host)
    {
        nghttp2_session_callbacks *callbacks = nullptr;
        if (nghttp2_session_callbacks_new(&callbacks) != 0) {
            say(kGrpcUnknown, "this build could not start an HTTP/2 session");
            return false;
        }
        nghttp2_session_callbacks_set_on_header_callback(callbacks, &Stream::onHeader);
        nghttp2_session_callbacks_set_on_frame_recv_callback(callbacks, &Stream::onFrame);
        nghttp2_session_callbacks_set_on_data_chunk_recv_callback(callbacks, &Stream::onData);
        nghttp2_session_callbacks_set_on_stream_close_callback(callbacks, &Stream::onClose);

        const auto made = nghttp2_session_client_new(&session_, callbacks, this);
        nghttp2_session_callbacks_del(callbacks);
        if (made != 0) {
            say(kGrpcUnknown, "this build could not start an HTTP/2 session");
            return false;
        }

        const nghttp2_settings_entry settings[] = {
            {NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, 1},
            {NGHTTP2_SETTINGS_INITIAL_WINDOW_SIZE, 1 << 20},
        };
        if (nghttp2_submit_settings(session_, NGHTTP2_FLAG_NONE, settings,
                                    sizeof(settings) / sizeof(settings[0])) != 0) {
            say(kGrpcUnknown, "this build could not configure an HTTP/2 session");
            return false;
        }

        std::vector<nghttp2_nv> headers;
        const auto add = [&headers](std::string_view name, std::string_view value) {
            nghttp2_nv one{};
            one.name =
                const_cast<std::uint8_t *>(reinterpret_cast<const std::uint8_t *>(name.data()));
            one.namelen = name.size();
            one.value =
                const_cast<std::uint8_t *>(reinterpret_cast<const std::uint8_t *>(value.data()));
            one.valuelen = value.size();
            one.flags = NGHTTP2_NV_FLAG_NONE;
            headers.push_back(one);
        };

        authority_ = host;
        add(":method", "POST");
        add(":scheme", "https");
        add(":authority", authority_);
        add(":path", call_.method);
        add("content-type", "application/grpc");
        add("te", "trailers");
        add("grpc-accept-encoding", "identity");
        add("user-agent", "ai-softphone/grpc");
        for (const auto &[name, value] : call_.headers) {
            add(name, value);
        }

        nghttp2_data_provider2 body{};
        body.source.ptr = this;
        body.read_callback = &Stream::readBody;

        streamId_ =
            nghttp2_submit_request2(session_, nullptr, headers.data(), headers.size(), &body, this);
        if (streamId_ < 0) {
            say(kGrpcUnknown, "this build could not open an HTTP/2 stream");
            return false;
        }
        return true;
    }

    [[nodiscard]] bool writeOut()
    {
        const std::uint8_t *out = nullptr;
        for (;;) {
            const auto length = nghttp2_session_mem_send2(session_, &out);
            if (length < 0) {
                refuse(kGrpcUnknown, "this HTTP/2 session could not be written");
                return false;
            }
            if (length == 0) {
                return true;
            }
            const auto whole = static_cast<std::size_t>(length);
            std::size_t written = 0;
            while (written < whole) {
                const auto went = SSL_write(ssl_, out + written, static_cast<int>(whole - written));
                if (went <= 0) {
                    refuse(kGrpcUnavailable, "the connection to the recogniser was lost");
                    return false;
                }
                written += static_cast<std::size_t>(went);
            }
        }
    }

    [[nodiscard]] bool readIn()
    {
        std::array<std::uint8_t, 16384> buffer{};
        const auto read = SSL_read(ssl_, buffer.data(), static_cast<int>(buffer.size()));
        if (read <= 0) {
            const auto why = SSL_get_error(ssl_, read);
            if (why == SSL_ERROR_WANT_READ || why == SSL_ERROR_WANT_WRITE) {
                return true;
            }

            const std::lock_guard<std::mutex> held{lock_};
            streamGone_ = true;
            return false;
        }
        const auto taken =
            nghttp2_session_mem_recv2(session_, buffer.data(), static_cast<std::size_t>(read));
        if (taken < 0) {
            refuse(kGrpcUnknown, "this recogniser sent something HTTP/2 could not read");
            return false;
        }
        return true;
    }

    void finish()
    {
        int status = kGrpcOk;
        std::string said;
        {
            const std::lock_guard<std::mutex> held{lock_};
            if (leaving_) {
                return;
            }
            if (refusal_) {
                status = *refusal_;
                said = refusalSaid_;
            } else if (!contentTypeWrong_.empty()) {
                status = kGrpcUnknown;
                said = "this address answered " + contentTypeWrong_ + " rather than gRPC";
            } else if (httpStatus_ != 0 && httpStatus_ != 200) {
                status = kGrpcUnknown;
                said = "this recogniser answered HTTP " + std::to_string(httpStatus_);
            } else if (grpcStatus_) {
                status = *grpcStatus_;
                said = grpcSaid_;
            } else if (told_) {

                status = kGrpcUnknown;
                said = "this recogniser closed the stream without saying why";
            } else {
                status = kGrpcUnavailable;
                said = "this recogniser did not accept the stream";
            }
            over_ = true;
        }
        if (status != kGrpcOk && !said.empty()) {
            core::Logger::instance().log(core::LogLevel::Warning, "realtime",
                                         "grpc: " + said + " (status " + std::to_string(status) +
                                             ")");
        }
        sink_.closed(status, said);
    }

    void say(int status, std::string said)
    {
        {
            const std::lock_guard<std::mutex> held{lock_};
            if (over_ || leaving_) {
                return;
            }
            over_ = true;
        }
        core::Logger::instance().log(core::LogLevel::Warning, "realtime", "grpc: " + said);
        sink_.closed(status, said);
    }

    static constexpr int kGrpcUnsupportedScheme = 12;

    core::IGrpcStreamSink &sink_;
    core::IGrpcTransport::Call call_;
    std::string authority_;

    Socket socket_{kNoSocket};
    SSL_CTX *context_{nullptr};
    SSL *ssl_{nullptr};
    nghttp2_session *session_{nullptr};
    std::int32_t streamId_{-1};

    std::string inbound_;

    mutable std::mutex lock_;
    std::condition_variable wake_;
    std::deque<std::string> outbound_;
    std::size_t sentOfFront_{0};
    std::optional<int> refusal_;
    std::string refusalSaid_;
    std::optional<int> grpcStatus_;
    std::string grpcSaid_;
    std::string contentTypeWrong_;
    int httpStatus_{0};
    std::uint32_t closedBy_{0};
    bool deferred_{false};
    bool sendingDone_{false};
    bool running_{false};
    bool streamGone_{false};
    bool told_{false};
    bool over_{false};
    bool leaving_{false};

    std::thread pump_;
};

}

Nghttp2GrpcTransport::Nghttp2GrpcTransport() = default;
Nghttp2GrpcTransport::~Nghttp2GrpcTransport() = default;

std::unique_ptr<core::IGrpcStream> Nghttp2GrpcTransport::open(const Call &call,
                                                              core::IGrpcStreamSink &sink)
{
    return std::make_unique<Stream>(call, sink);
}

std::string Nghttp2GrpcTransport::libraryVersion() const
{
    const auto *const about = nghttp2_version(0);
    return about != nullptr && about->version_str != nullptr ? std::string{about->version_str}
                                                             : std::string{};
}

}
