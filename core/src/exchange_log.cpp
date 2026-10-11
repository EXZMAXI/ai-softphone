// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/exchange_log.hpp>

#include <softphone/core/time_text.hpp>

#include <nlohmann/json.hpp>

#include <map>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace softphone::core::exchange {
namespace {

struct Kept {
    std::chrono::steady_clock::time_point at{};
    nlohmann::ordered_json document;

    std::size_t bytes{0};
};

std::mutex &lock()
{
    static std::mutex one;
    return one;
}

std::map<std::string, Kept> &held()
{
    static std::map<std::string, Kept> all;
    return all;
}

Clock &clock()
{
    static Clock now;
    return now;
}

[[nodiscard]] std::chrono::steady_clock::time_point rightNow()
{
    return clock() ? clock()() : std::chrono::steady_clock::now();
}

[[nodiscard]] nlohmann::json bodyOf(std::string_view text)
{
    if (text.empty()) {
        return nullptr;
    }
    auto parsed = nlohmann::json::parse(text, nullptr, false);
    if (parsed.is_discarded()) {
        return std::string{text};
    }
    return parsed;
}

void sweepLocked()
{
    auto &all = held();
    const auto now = rightNow();

    std::size_t weight = 0;
    for (auto one = all.begin(); one != all.end();) {
        if (now - one->second.at > kKeptFor) {
            one = all.erase(one);
            continue;
        }
        weight += one->second.bytes;
        ++one;
    }

    while (weight > kRoomFor && !all.empty()) {
        auto oldest = all.begin();
        for (auto one = all.begin(); one != all.end(); ++one) {
            if (one->second.at < oldest->second.at) {
                oldest = one;
            }
        }
        weight -= oldest->second.bytes;
        all.erase(oldest);
    }
}

}

void keep(std::string_view artifactId, int part, int of, std::string_view request,
          std::string_view response)
{
    if (artifactId.empty()) {
        return;
    }

    nlohmann::ordered_json step;
    step["part"] = part;
    step["of"] = of;
    step["request"] = bodyOf(request);
    step["response"] = bodyOf(response);

    const std::lock_guard<std::mutex> one{lock()};
    sweepLocked();

    auto &entry = held()[std::string{artifactId}];
    if (entry.document.is_null()) {
        entry.document = nlohmann::ordered_json::object();
        entry.document["artifact"] = std::string{artifactId};
        entry.document["steps"] = nlohmann::ordered_json::array();
    }
    entry.at = rightNow();
    entry.document["at"] = isoUtc(std::chrono::system_clock::now());
    entry.bytes += request.size() + response.size();
    entry.document["steps"].push_back(std::move(step));
}

std::string readBack(std::string_view artifactId)
{
    if (artifactId.empty()) {
        return {};
    }
    const std::lock_guard<std::mutex> one{lock()};
    sweepLocked();

    const auto found = held().find(std::string{artifactId});
    return found == held().end() ? std::string{} : found->second.document.dump(2);
}

bool kept(std::string_view artifactId)
{
    if (artifactId.empty()) {
        return false;
    }
    const std::lock_guard<std::mutex> one{lock()};
    sweepLocked();
    return held().find(std::string{artifactId}) != held().end();
}

void sweep()
{
    const std::lock_guard<std::mutex> one{lock()};
    sweepLocked();
}

void forgetAll()
{
    const std::lock_guard<std::mutex> one{lock()};
    held().clear();
}

void setClock(Clock now)
{
    const std::lock_guard<std::mutex> one{lock()};
    clock() = std::move(now);
}

}
