// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#pragma once

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace softphone::core {

enum class TelemetryMode {

    Basic,

    Extended,

    Off,
};

[[nodiscard]] std::string_view toString(TelemetryMode mode) noexcept;
[[nodiscard]] TelemetryMode parseTelemetryMode(std::string_view text) noexcept;

struct TelemetryCounters {

    std::uint64_t calls{0};

    std::uint64_t captures{0};

    [[nodiscard]] bool empty() const noexcept
    {
        return calls == 0 && captures == 0;
    }

    bool operator==(const TelemetryCounters &) const = default;
};

struct TelemetryState {

    std::string instanceId;

    TelemetryCounters counters;

    std::vector<std::string> sipServers;

    std::chrono::system_clock::time_point lastAccepted{};

    std::string noticeSeen;

    bool operator==(const TelemetryState &) const = default;
};

[[nodiscard]] std::string sipServerKind(std::string_view said);

[[nodiscard]] std::string installationId();

class TelemetryStore {
public:

    explicit TelemetryStore(std::filesystem::path file);

    [[nodiscard]] std::string instanceId() const;

    [[nodiscard]] TelemetryState state() const;

    void countCall();
    void countCapture();

    static constexpr std::size_t kMostServers = 8;
    void sawServer(const std::string &kind);

    void accepted(const TelemetryCounters &sent, const std::vector<std::string> &serversSent,
                  std::chrono::system_clock::time_point at);

    void noteNoticeSeen(const std::string &version);

    void forgetCounters();

private:
    void write();

    mutable std::mutex mutex_;
    std::filesystem::path file_;
    TelemetryState state_;
};

}
