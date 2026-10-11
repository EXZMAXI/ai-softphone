// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/telemetry_service.hpp>

#include <softphone/core/local_hosts.hpp>

#include <softphone/core/logging.hpp>
#include <softphone/core/paths.hpp>
#include <softphone/core/settings.hpp>
#include <softphone/core/uuid.hpp>
#include <softphone/core/version.hpp>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <span>
#include <string_view>
#include <utility>

namespace softphone::core {
namespace {

constexpr std::string_view kArea{"telemetry"};

constexpr std::size_t kLongestLanguage = 8;

constexpr std::string_view kHosts[] = {
    "https://telemetry.ai-softphone.com/api/token/v1/09529ce310212e53d62465f7fe79b44a",
};

constexpr const char *kHostsFromTheEnvironment = "SOFTPHONE_TELEMETRY_HOSTS";

#ifdef SOFTPHONE_TELEMETRY_REPORTS
constexpr bool kThisBuildReports = SOFTPHONE_TELEMETRY_REPORTS != 0;
#else
constexpr bool kThisBuildReports = false;
#endif

constexpr std::size_t kLongestKind = 40;

constexpr std::chrono::seconds kSayIt{10};

constexpr std::chrono::seconds kSendTheFiles{60};

constexpr std::uintmax_t kLogTail = 512 * 1024;

}

std::vector<std::string> telemetryHosts(std::string_view country)
{

    if (const char *told = std::getenv(kHostsFromTheEnvironment); told != nullptr) {
        std::vector<std::string> hosts;
        std::string_view rest{told};
        while (!rest.empty()) {
            const auto space = rest.find(' ');
            const auto one = rest.substr(0, space);
            if (!one.empty()) {
                hosts.emplace_back(one);
            }
            rest = space == std::string_view::npos ? std::string_view{} : rest.substr(space + 1);
        }
        return hosts;
    }

    return publishedTelemetryHosts(country);
}

bool thisBuildReportsOnItsOwn() noexcept
{
    return kThisBuildReports;
}

std::vector<std::string> publishedTelemetryHosts(std::string_view country)
{
    std::vector<std::string> hosts;
    hosts.reserve(std::size(kHosts));
    for (const auto one : kHosts) {
        hosts.emplace_back(one);
    }

    for (auto &one : hosts::telemetryHostsFor(country)) {
        hosts.push_back(std::move(one));
    }
    return hosts;
}

std::string hostOf(const std::string &url)
{
    const auto scheme = url.find("://");
    if (scheme == std::string::npos) {
        return {};
    }
    const auto host = url.find('/', scheme + 3);
    return host == std::string::npos ? url : url.substr(0, host);
}

std::string asJson(const UsageReport &report)
{
    nlohmann::json document;

    document["method"] = "usage";
    document["unique_id"] = report.uniqueId;
    document["product"] = report.product;
    document["version"] = report.version;

    document["language"] = report.language;
    document["instance_id"] = report.instanceId;

    document["os"]["family"] = report.osFamily;
    document["os"]["version"] = report.osVersion;
    document["os"]["arch"] = report.osArch;
    document["os"]["locale"] = report.osLocale;
    document["os"]["country"] = report.osCountry;
    document["os"]["timezone"] = report.osTimezone;

    if (report.extended) {
        document["counters"]["calls"] = report.counters.calls;
        document["counters"]["captures"] = report.counters.captures;

        document["sipServers"] = report.sipServers;

        nlohmann::json settings = nlohmann::json::object();
        if (report.overviewDone >= 0) {
            settings["summary"] = report.overviewDone;
        }
        if (!report.layout.empty()) {
            settings["layout"] = report.layout;
        }
        if (!settings.empty()) {
            document["settings"] = std::move(settings);
        }
    }

    document["ext"] = nlohmann::json::object();

    return document.dump();
}

std::string sipServerKind(std::string_view said)
{

    std::size_t at = 0;
    while (at < said.size() && said[at] == ' ') {
        ++at;
    }
    const auto begins = at;
    while (at < said.size() && said[at] != ' ' && said[at] != '/' && said[at] != '(') {
        ++at;
    }

    std::string product;
    bool anyLetter = false;
    for (const char letter : said.substr(begins, at - begins)) {
        const auto one = static_cast<unsigned char>(letter);
        if (letter == '.') {

            return {};
        }
        if (std::isalpha(one) != 0) {
            anyLetter = true;
        }
        if (std::isalnum(one) != 0 || letter == '-' || letter == '_') {
            product.push_back(static_cast<char>(std::tolower(one)));
        }
    }
    if (!anyLetter || product.empty()) {
        return {};
    }

    std::string version;
    for (std::size_t one = at; one < said.size(); ++one) {
        if (std::isdigit(static_cast<unsigned char>(said[one])) == 0) {
            continue;
        }
        int components = 0;
        while (one < said.size() &&
               (std::isdigit(static_cast<unsigned char>(said[one])) != 0 || said[one] == '.')) {
            if (said[one] == '.') {
                if (++components == 2) {
                    break;
                }
            }
            version.push_back(said[one]);
            ++one;
        }
        break;
    }
    while (!version.empty() && version.back() == '.') {
        version.pop_back();
    }

    auto kind = version.empty() ? product : product + "/" + version;
    if (kind.size() > kLongestKind) {
        kind.resize(kLongestKind);
    }
    return kind;
}

TelemetryService::TelemetryService(TelemetryStore &store, IHttpClient &client, Clock clock)
    : store_{store}, client_{client}, clock_{std::move(clock)}
{}

std::string asJson(const FeedbackReport &report)
{
    nlohmann::json document;
    document["method"] = "feedback";
    document["unique_id"] = report.uniqueId;
    document["product"] = report.product;
    document["version"] = report.version;
    document["language"] = report.language;
    document["instance_id"] = report.instanceId;

    document["os"]["family"] = report.osFamily;
    document["os"]["version"] = report.osVersion;
    document["os"]["arch"] = report.osArch;
    document["os"]["locale"] = report.osLocale;
    document["os"]["country"] = report.osCountry;
    document["os"]["timezone"] = report.osTimezone;

    document["subject"] = report.subject;
    document["message"] = report.message;
    document["name"] = report.name;

    document["email"] = report.email;

    document["ext"] = nlohmann::json::object();

    return document.dump();
}

std::vector<Attachment> attachmentsFor(const std::filesystem::path &logFile,
                                       const std::filesystem::path &stateDirectory)
{
    std::vector<Attachment> attached;

    std::error_code failure;
    if (std::filesystem::exists(logFile, failure)) {
        const auto size = std::filesystem::file_size(logFile, failure);
        std::ifstream input{logFile, std::ios::binary};
        if (input && !failure) {
            if (size > kLogTail) {
                input.seekg(static_cast<std::streamoff>(size - kLogTail));
            }
            std::string text{std::istreambuf_iterator<char>{input},
                             std::istreambuf_iterator<char>{}};

            text = shortenBuildPaths(redact(text));
            attached.push_back({asUtf8(logFile.filename()), std::move(text)});
        }
    }

    const auto newestOfThese = [&stateDirectory, &failure](std::string_view extension) {
        std::filesystem::path newest;
        std::filesystem::file_time_type latest{};
        for (const auto &entry : std::filesystem::directory_iterator{stateDirectory, failure}) {
            if (!entry.is_regular_file(failure) || entry.path().extension() != extension) {
                continue;
            }
            if (!entry.path().filename().string().starts_with("crash-")) {
                continue;
            }
            const auto written = entry.last_write_time(failure);
            if (newest.empty() || written > latest) {
                latest = written;
                newest = entry.path();
            }
        }
        return newest;
    };

    if (const auto report = newestOfThese(".txt"); !report.empty()) {
        std::ifstream input{report, std::ios::binary};
        if (input) {
            std::string text{std::istreambuf_iterator<char>{input},
                             std::istreambuf_iterator<char>{}};
            attached.push_back(
                {asUtf8(report.filename()), shortenBuildPaths(redact(std::move(text)))});
        }
    }

    if (const auto dumpFile = newestOfThese(".dmp"); !dumpFile.empty()) {
        std::ifstream dump{dumpFile, std::ios::binary};
        if (dump) {
            std::string bytes{std::istreambuf_iterator<char>{dump},
                              std::istreambuf_iterator<char>{}};
            attached.push_back({asUtf8(dumpFile.filename()), std::move(bytes)});
        }
    }

    return attached;
}

struct Somewhere {
    std::string url;
    std::string authorization;
};

[[nodiscard]] Somewhere somewhereFrom(const std::string &body)
{
    const auto document = nlohmann::json::parse(body, nullptr, false);
    if (!document.is_object()) {
        return {};
    }
    Somewhere told;
    if (document.contains("url") && document["url"].is_string()) {
        told.url = document["url"].get<std::string>();
    }
    if (document.contains("auth") && document["auth"].is_string()) {
        told.authorization = document["auth"].get<std::string>();
    }

    if (told.url.empty() || told.url.front() != '/' || told.authorization.empty()) {
        return {};
    }
    return told;
}

bool TelemetryService::sendFeedback(FeedbackReport report, std::vector<Attachment> attached,
                                    const HttpCancel &cancelled)
{

    std::string country;
    {
        const std::lock_guard guard{mutex_};
        country = country_;
        report.uniqueId = makeUuid();
        report.version = std::string{versionString()} + "-" + std::string{platformName()};
        report.language = language_;
        report.instanceId = store_.instanceId();
        report.osFamily = facts_.osFamily;
        report.osVersion = facts_.osVersion;
        report.osArch = facts_.osArch;
        report.osLocale = facts_.osLocale;
        report.osCountry = facts_.osCountry;
        report.osTimezone = facts_.osTimezone;
    }

    const auto body = asJson(report);

    for (const auto &host : telemetryHosts(country)) {
        if (cancelled && cancelled()) {
            return false;
        }

        HttpRequest request;
        request.timeout = kSayIt;
        request.verb = HttpVerb::Post;
        request.url = host;
        request.body = body;
        request.contentType = "application/json";

        const auto answer = client_.send(request, cancelled);
        if (!answer.ok()) {
            continue;
        }

        const auto answered = hostOf(request.url);
        {
            const LogAttribute fields[] = {{"host", answered}};
            Logger::instance().log(LogLevel::Info, kArea, "a message was delivered", fields);
        }

        if (!attached.empty()) {
            putTheFiles(answered, answer.body, attached, cancelled);
        }
        return true;
    }

    Logger::instance().log(LogLevel::Warning, kArea, "no host took the message");
    return false;
}

void TelemetryService::putTheFiles(const std::string &host, const std::string &answer,
                                   const std::vector<Attachment> &attached,
                                   const HttpCancel &cancelled)
{
    const auto told = somewhereFrom(answer);
    if (told.url.empty()) {
        Logger::instance().log(LogLevel::Debug, kArea,
                               "the message was taken and no route for its files came back");
        return;
    }

    for (const auto &file : attached) {
        if (cancelled && cancelled()) {
            return;
        }

        HttpRequest put;
        put.timeout = kSendTheFiles;
        put.verb = HttpVerb::Put;

        put.url = host + told.url + "/" + file.name;
        put.body = file.bytes;
        put.contentType = "application/octet-stream";
        put.headers.emplace_back("Authorization", headerSafeValue(told.authorization));

        const auto placed = client_.send(put, cancelled);
        const auto name = file.name;
        const auto size = std::to_string(file.bytes.size());
        const LogAttribute fields[] = {{"file", name}, {"bytes", size}};
        Logger::instance().log(placed.ok() ? LogLevel::Debug : LogLevel::Warning, kArea,
                               placed.ok() ? "a file went with the message"
                                           : "a file could not be sent with the message",
                               fields);
    }
}

TelemetryService::~TelemetryService()
{
    stop();
}

void TelemetryService::start()
{
    const std::lock_guard guard{mutex_};
    if (worker_.joinable() || stopping_) {
        return;
    }

    if (!reportsOnItsOwn_) {
        Logger::instance().log(
            LogLevel::Debug, kArea,
            "this build reports nothing of its own; a message or a report asked for by hand "
            "still goes");
    } else if (telemetryHosts(country_).empty()) {
        Logger::instance().log(LogLevel::Debug, kArea, "no telemetry host: nothing can be sent");
    }

    worker_ = std::thread{[this]() { work(); }};
}

void TelemetryService::stop()
{
    {
        const std::lock_guard guard{mutex_};
        if (stopping_) {
            return;
        }
        stopping_ = true;
    }

    wakeup_.notify_all();
    if (worker_.joinable()) {
        worker_.join();
    }
}

void TelemetryService::work()
{
    for (;;) {
        {
            std::unique_lock guard{mutex_};

            wakeup_.wait_for(guard, kTick, [this]() { return stopping_; });
            if (stopping_) {
                return;
            }
        }

        if (!dueNow()) {
            continue;
        }
        static_cast<void>(send([this]() {
            const std::lock_guard guard{mutex_};
            return stopping_;
        }));
    }
}

void TelemetryService::setFacts(Facts facts)
{
    const std::lock_guard guard{mutex_};
    facts_ = std::move(facts);
}

void TelemetryService::setLanguage(std::string code)
{

    std::string tidy;
    for (const char letter : code) {
        if (tidy.size() >= kLongestLanguage) {
            break;
        }
        const auto one = static_cast<unsigned char>(letter);
        if (std::isalnum(one) != 0 || letter == '-' || letter == '_') {
            tidy.push_back(letter);
        }
    }

    const std::lock_guard guard{mutex_};
    language_ = std::move(tidy);
}

void TelemetryService::setOverviewDone(int done)
{
    const std::lock_guard guard{mutex_};
    overviewDone_ = done < 0 ? -1 : done;
}

void TelemetryService::setLayout(std::string name)
{

    if (!parseLayout(name).has_value()) {
        name.clear();
    }

    const std::lock_guard guard{mutex_};
    layout_ = std::move(name);
}

void TelemetryService::setCountry(std::string code)
{

    const bool plausible = code.size() == 2 &&
                           std::isupper(static_cast<unsigned char>(code[0])) != 0 &&
                           std::isupper(static_cast<unsigned char>(code[1])) != 0;

    const std::lock_guard guard{mutex_};
    country_ = plausible ? std::move(code) : std::string{};
}

void TelemetryService::setMode(TelemetryMode mode)
{
    const std::lock_guard guard{mutex_};
    if (mode_ == mode) {
        return;
    }

    if (mode_ == TelemetryMode::Extended && mode != TelemetryMode::Extended) {
        store_.forgetCounters();
    }
    mode_ = mode;
}

void TelemetryService::setReportsOnItsOwn(bool reporting)
{
    const std::lock_guard guard{mutex_};
    reportsOnItsOwn_ = reporting;
}

void TelemetryService::setNoticeAnswered(bool answered)
{
    const std::lock_guard guard{mutex_};
    noticeAnswered_ = answered;
}

bool TelemetryService::dueNow() const
{
    const std::lock_guard guard{mutex_};
    if (!noticeAnswered_ || mode_ == TelemetryMode::Off) {
        return false;
    }

    if (!reportsOnItsOwn_) {
        return false;
    }

    if (telemetryHosts(country_).empty()) {
        return false;
    }

    const auto now = clock_();

    if (failures_ > 0) {
        const auto wait =
            failures_ <= std::size(kRetries)
                ? std::chrono::duration_cast<std::chrono::system_clock::duration>(
                      kRetries[failures_ - 1])
                : std::chrono::duration_cast<std::chrono::system_clock::duration>(kInterval);
        return now - lastAttempt_ >= wait;
    }

    if (!sentThisRun_) {
        return true;
    }

    return now - store_.state().lastAccepted >= kInterval;
}

UsageReport TelemetryService::reportNow() const
{
    const std::lock_guard guard{mutex_};
    const auto state = store_.state();

    UsageReport report;
    report.uniqueId = makeUuid();
    report.version = std::string{versionString()} + "-" + std::string{platformName()};
    report.language = language_;
    report.instanceId = state.instanceId;

    report.osFamily = facts_.osFamily;
    report.osVersion = facts_.osVersion;
    report.osArch = facts_.osArch;
    report.osLocale = facts_.osLocale;
    report.osCountry = facts_.osCountry;
    report.osTimezone = facts_.osTimezone;

    report.extended = mode_ == TelemetryMode::Extended;
    if (report.extended) {
        report.counters = state.counters;
        report.sipServers = state.sipServers;
        report.overviewDone = overviewDone_;
        report.layout = layout_;
    }
    return report;
}

std::string TelemetryService::lastHost() const
{
    const std::lock_guard guard{mutex_};
    return lastHost_;
}

bool TelemetryService::send(const HttpCancel &cancelled)
{
    const auto report = reportNow();
    const auto body = asJson(report);

    const auto country = [this]() {
        const std::lock_guard guard{mutex_};
        return country_;
    }();

    {
        const std::lock_guard guard{mutex_};
        lastAttempt_ = clock_();

        sentThisRun_ = true;
    }

    for (const auto &host : telemetryHosts(country)) {
        if (cancelled && cancelled()) {

            return false;
        }

        HttpRequest request;
        request.timeout = kSayIt;
        request.verb = HttpVerb::Post;
        request.url = host;
        request.body = body;
        request.contentType = "application/json";

        const auto answer = client_.send(request, cancelled);
        if (!answer.ok()) {
            continue;
        }

        store_.accepted(report.counters, report.sipServers, clock_());
        {
            const std::lock_guard guard{mutex_};
            failures_ = 0;
        }

        const auto answered = hostOf(request.url);
        {
            const std::lock_guard guard{mutex_};
            lastHost_ = answered;
        }
        const LogAttribute fields[] = {{"host", answered}};
        Logger::instance().log(LogLevel::Debug, kArea, "a usage report was accepted", fields);
        return true;
    }

    {
        const std::lock_guard guard{mutex_};
        ++failures_;
    }
    Logger::instance().log(LogLevel::Debug, kArea, "no telemetry host took the report");
    return false;
}

}
