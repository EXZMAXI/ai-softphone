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

#include <softphone/core/http.hpp>
#include <softphone/core/telemetry.hpp>

#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace softphone::core {

struct UsageReport {

    std::string uniqueId;
    std::string product{"ai-softphone"};

    std::string version;

    std::string language;

    std::string instanceId;

    std::string osFamily;
    std::string osVersion;
    std::string osArch;
    std::string osLocale;

    std::string osCountry;

    std::string osTimezone;

    bool extended{false};
    TelemetryCounters counters;
    std::vector<std::string> sipServers;

    int overviewDone{-1};

    std::string layout;
};

struct FeedbackReport {
    std::string uniqueId;
    std::string product{"ai-softphone"};
    std::string version;

    std::string language;

    std::string instanceId;

    std::string osFamily;
    std::string osVersion;
    std::string osArch;
    std::string osLocale;
    std::string osCountry;
    std::string osTimezone;

    std::string subject;
    std::string message;

    std::string name;
    std::string email;

};

struct Attachment {

    std::string name;

    std::string bytes;
};

[[nodiscard]] std::string asJson(const FeedbackReport &report);

[[nodiscard]] std::vector<Attachment> attachmentsFor(const std::filesystem::path &logFile,
                                                     const std::filesystem::path &stateDirectory);

[[nodiscard]] std::string asJson(const UsageReport &report);

[[nodiscard]] std::vector<std::string> telemetryHosts(std::string_view country = {});

[[nodiscard]] bool thisBuildReportsOnItsOwn() noexcept;

[[nodiscard]] std::vector<std::string> publishedTelemetryHosts(std::string_view country = {});

[[nodiscard]] std::string hostOf(const std::string &url);

class TelemetryService {
public:

    static constexpr auto kInterval = std::chrono::hours{24};

    static constexpr std::chrono::minutes kRetries[] = {
        std::chrono::minutes{15}, std::chrono::minutes{60}, std::chrono::minutes{240}};

    using Clock = std::function<std::chrono::system_clock::time_point()>;

    struct Facts {
        std::string osFamily;
        std::string osVersion;
        std::string osArch;
        std::string osLocale;
        std::string osCountry;
        std::string osTimezone;
    };

    TelemetryService(TelemetryStore &store, IHttpClient &client, Clock clock);

    void setFacts(Facts facts);

    void setLanguage(std::string code);

    void setCountry(std::string code);

    void setOverviewDone(int done);

    void setLayout(std::string name);

    void setMode(TelemetryMode mode);

    [[nodiscard]] TelemetryMode mode() const noexcept
    {
        return mode_;
    }

    void setNoticeAnswered(bool answered);

    void setReportsOnItsOwn(bool reporting);

    [[nodiscard]] bool reportsOnItsOwn() const noexcept
    {
        return reportsOnItsOwn_;
    }

    [[nodiscard]] bool dueNow() const;

    bool send(const HttpCancel &cancelled);

    [[nodiscard]] UsageReport reportNow() const;

    [[nodiscard]] std::string lastHost() const;

    bool sendFeedback(FeedbackReport report, std::vector<Attachment> attached,
                      const HttpCancel &cancelled);

    static constexpr auto kTick = std::chrono::minutes{1};

    void start();

    void stop();

    ~TelemetryService();

    TelemetryService(const TelemetryService &) = delete;
    TelemetryService &operator=(const TelemetryService &) = delete;
    TelemetryService(TelemetryService &&) = delete;
    TelemetryService &operator=(TelemetryService &&) = delete;

private:
    void work();

    void putTheFiles(const std::string &host, const std::string &answer,
                     const std::vector<Attachment> &attached, const HttpCancel &cancelled);

    mutable std::mutex mutex_;
    std::condition_variable wakeup_;
    std::thread worker_;
    bool stopping_{false};

    TelemetryStore &store_;
    IHttpClient &client_;
    Clock clock_;
    Facts facts_;

    std::string language_;
    std::string country_;
    int overviewDone_{-1};
    std::string layout_;

    TelemetryMode mode_{TelemetryMode::Extended};
    bool noticeAnswered_{false};

    bool reportsOnItsOwn_{thisBuildReportsOnItsOwn()};

    std::chrono::system_clock::time_point lastAttempt_{};
    std::size_t failures_{0};

    bool sentThisRun_{false};
    std::string lastHost_;
};

[[nodiscard]] TelemetryService::Facts machineFacts();

}
