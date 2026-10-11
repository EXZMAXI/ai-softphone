// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/http/httplib_update_transport.hpp>

#include "machine_trust.hpp"
#include "split_url.hpp"

#include <softphone/core/paths.hpp>
#include <softphone/core/version.hpp>

#include <httplib.h>

#include <cstdio>
#include <fstream>
#include <system_error>

namespace softphone::http {
namespace {

[[nodiscard]] httplib::Client open(const SplitUrl &address, std::chrono::seconds timeout,
                                   const std::filesystem::path &caBundle)
{
    httplib::Client client{address.origin};

    if (!caBundle.empty()) {
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
        client.set_ca_cert_path(core::asUtf8(caBundle).c_str());
        client.enable_server_certificate_verification(true);
#endif
    }

    else {
        trustTheMachine(client, address.host);
    }

    client.set_connection_timeout(timeout);
    client.set_read_timeout(timeout);
    client.set_write_timeout(timeout);
    client.set_follow_location(true);

    client.set_default_headers({{"User-Agent", std::string{core::userAgent()}}});
    return client;
}

}

HttplibUpdateTransport::HttplibUpdateTransport(std::chrono::seconds timeout,
                                               std::filesystem::path caBundle)
    : timeout_{timeout}, caBundle_{std::move(caBundle)}
{}

HttplibUpdateTransport::~HttplibUpdateTransport() = default;

core::Fetched HttplibUpdateTransport::fetch(const std::string &url)
{
    core::Fetched fetched;

    const auto address = split(url);
    if (!address.ok) {
        fetched.error = "the update address is not a URL this can open: " + url;
        return fetched;
    }

#ifndef CPPHTTPLIB_OPENSSL_SUPPORT
    if (address.tls) {
        fetched.error = "this build has no TLS and the update address needs it";
        return fetched;
    }
#endif

    auto client = open(address, timeout_, caBundle_);

    std::string body;
    bool overflowed = false;

    const auto answer =
        client.Get(address.target, [&body, &overflowed](const char *data, std::size_t length) {
            if (body.size() + length > kManifestCeiling) {
                overflowed = true;
                return false;
            }
            body.append(data, length);
            return true;
        });

    if (overflowed) {
        fetched.error = "the update server answered with more than a manifest can be";
        return fetched;
    }

    if (!answer) {
        fetched.error = "no answer from the update server: " + httplib::to_string(answer.error());
        return fetched;
    }

    fetched.status = answer->status;
    fetched.body = std::move(body);
    fetched.ok = answer->status >= 200 && answer->status < 300;
    if (!fetched.ok) {
        fetched.error = "the update server answered " + std::to_string(answer->status);
    }
    return fetched;
}

bool HttplibUpdateTransport::download(
    const std::string &url, const std::filesystem::path &into,
    const std::function<bool(std::uint64_t, std::uint64_t)> &progress, std::string &complaint)
{
    complaint.clear();

    const auto address = split(url);
    if (!address.ok) {
        complaint = "the download address is not a URL this can open";
        return false;
    }

#ifndef CPPHTTPLIB_OPENSSL_SUPPORT
    if (address.tls) {
        complaint = "this build has no TLS and the download address needs it";
        return false;
    }
#endif

    std::ofstream file{into, std::ios::binary | std::ios::trunc};
    if (!file) {
        complaint = "could not open " + into.string() + " to write";
        return false;
    }

    auto client = open(address, timeout_, caBundle_);

    bool abandoned = false;
    bool wrote = true;

    const auto answer = client.Get(
        address.target,
        [&file, &wrote](const char *data, std::size_t length) {
            file.write(data, static_cast<std::streamsize>(length));
            wrote = static_cast<bool>(file);
            return wrote;
        },
        [&progress, &abandoned](std::uint64_t received, std::uint64_t total) {
            if (!progress(received, total)) {
                abandoned = true;
                return false;
            }
            return true;
        });

    file.close();

    const auto giveUp = [&](std::string reason) {
        std::error_code ignored;
        std::filesystem::remove(into, ignored);
        complaint = std::move(reason);
        return false;
    };

    if (abandoned) {
        return giveUp("the download was abandoned");
    }
    if (!wrote) {
        return giveUp("the download could not be written to disk");
    }
    if (!answer) {
        return giveUp("the download did not complete: " + httplib::to_string(answer.error()));
    }
    if (answer->status < 200 || answer->status >= 300) {
        return giveUp("the server answered " + std::to_string(answer->status) + " for the package");
    }

    return true;
}

}
