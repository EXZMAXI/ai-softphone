// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/telemetry.hpp>

#include <softphone/core/logging.hpp>
#include <softphone/core/paths.hpp>
#include <softphone/core/uuid.hpp>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <fstream>
#include <sstream>
#include <system_error>
#include <utility>

namespace softphone::core {
namespace {

constexpr std::string_view kArea{"telemetry"};

constexpr std::uint64_t kMostOfAnything = 1'000'000'000ULL;

[[nodiscard]] std::uint64_t countAt(const nlohmann::json &document, const char *name)
{
    if (!document.contains(name) || !document[name].is_number_unsigned()) {
        return 0;
    }
    return std::min(document[name].get<std::uint64_t>(), kMostOfAnything);
}

[[nodiscard]] std::string textAt(const nlohmann::json &document, const char *name)
{
    if (!document.contains(name) || !document[name].is_string()) {
        return {};
    }
    return document[name].get<std::string>();
}

}

std::string_view toString(TelemetryMode mode) noexcept
{
    switch (mode) {
    case TelemetryMode::Basic:
        return "basic";
    case TelemetryMode::Extended:
        return "extended";
    case TelemetryMode::Off:
        return "off";
    }
    return "extended";
}

TelemetryMode parseTelemetryMode(std::string_view text) noexcept
{
    if (text == "basic") {
        return TelemetryMode::Basic;
    }
    if (text == "off") {
        return TelemetryMode::Off;
    }

    return TelemetryMode::Extended;
}

std::string installationId()
{

    const TelemetryStore store{stateDirectory() / "telemetry.json"};
    return store.instanceId();
}

TelemetryStore::TelemetryStore(std::filesystem::path file) : file_{std::move(file)}
{
    bool read = false;

    {
        std::ifstream input{file_};
        if (input) {
            std::stringstream all;
            all << input.rdbuf();
            const auto document = nlohmann::json::parse(all.str(), nullptr, false);
            if (document.is_object()) {
                state_.instanceId = textAt(document, "instanceId");
                state_.noticeSeen = textAt(document, "noticeSeen");

                if (document.contains("counters") && document["counters"].is_object()) {
                    state_.counters.calls = countAt(document["counters"], "calls");
                    state_.counters.captures = countAt(document["counters"], "captures");
                }
                if (document.contains("sipServers") && document["sipServers"].is_array()) {
                    for (const auto &one : document["sipServers"]) {
                        if (one.is_string() &&
                            state_.sipServers.size() < TelemetryStore::kMostServers) {
                            state_.sipServers.push_back(one.get<std::string>());
                        }
                    }
                }
                if (document.contains("lastAccepted") &&
                    document["lastAccepted"].is_number_integer()) {
                    state_.lastAccepted = std::chrono::system_clock::time_point{
                        std::chrono::seconds{document["lastAccepted"].get<std::int64_t>()}};
                }
                read = isUuid(state_.instanceId);
            }
        }
    }

    if (!read) {

        const bool existed = std::filesystem::exists(file_);
        state_ = {};
        state_.instanceId = makeUuid();
        if (existed) {
            Logger::instance().log(LogLevel::Debug, kArea,
                                   "the telemetry file could not be read; starting again");
        }
    }

    write();
}

std::string TelemetryStore::instanceId() const
{
    const std::lock_guard guard{mutex_};
    return state_.instanceId;
}

TelemetryState TelemetryStore::state() const
{
    const std::lock_guard guard{mutex_};
    return state_;
}

void TelemetryStore::countCall()
{
    const std::lock_guard guard{mutex_};
    if (state_.counters.calls < kMostOfAnything) {
        ++state_.counters.calls;
    }
    write();
}

void TelemetryStore::countCapture()
{
    const std::lock_guard guard{mutex_};
    if (state_.counters.captures < kMostOfAnything) {
        ++state_.counters.captures;
    }
    write();
}

void TelemetryStore::sawServer(const std::string &kind)
{
    if (kind.empty()) {
        return;
    }
    const std::lock_guard guard{mutex_};
    if (std::find(state_.sipServers.begin(), state_.sipServers.end(), kind) !=
        state_.sipServers.end()) {
        return;
    }
    if (state_.sipServers.size() >= kMostServers) {
        return;
    }
    state_.sipServers.push_back(kind);
    write();
}

void TelemetryStore::accepted(const TelemetryCounters &sent,
                              const std::vector<std::string> &serversSent,
                              std::chrono::system_clock::time_point at)
{
    const std::lock_guard guard{mutex_};

    state_.counters.calls -= std::min(sent.calls, state_.counters.calls);
    state_.counters.captures -= std::min(sent.captures, state_.counters.captures);

    static_cast<void>(serversSent);

    state_.lastAccepted = at;
    write();
}

void TelemetryStore::noteNoticeSeen(const std::string &version)
{
    const std::lock_guard guard{mutex_};
    state_.noticeSeen = version;
    write();
}

void TelemetryStore::forgetCounters()
{
    const std::lock_guard guard{mutex_};
    state_.counters = {};
    state_.sipServers.clear();
    write();
}

void TelemetryStore::write()
{
    nlohmann::json document;
    document["instanceId"] = state_.instanceId;
    document["counters"]["calls"] = state_.counters.calls;
    document["counters"]["captures"] = state_.counters.captures;
    document["sipServers"] = state_.sipServers;
    document["lastAccepted"] =
        std::chrono::duration_cast<std::chrono::seconds>(state_.lastAccepted.time_since_epoch())
            .count();
    document["noticeSeen"] = state_.noticeSeen;

    std::error_code error;
    std::filesystem::create_directories(file_.parent_path(), error);

    const auto temporary = std::filesystem::path{file_}.concat(".new");
    {
        std::ofstream out{temporary, std::ios::trunc};
        if (!out) {

            Logger::instance().log(LogLevel::Debug, kArea, "the telemetry file cannot be written");
            return;
        }
        out << document.dump(2) << '\n';
        if (!out) {
            return;
        }
    }

    static_cast<void>(restrictToOwner(temporary));
    std::filesystem::rename(temporary, file_, error);
    if (error) {
        std::filesystem::remove(temporary, error);
    }
}

}
