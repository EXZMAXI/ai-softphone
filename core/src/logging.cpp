// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/logging.hpp>

#include <softphone/core/crash.hpp>

#include <softphone/core/paths.hpp>
#include <softphone/core/time_text.hpp>

#if defined(__unix__) || defined(__APPLE__)
#define SOFTPHONE_HAS_SYSLOG 1
#include <syslog.h>
#else
#define SOFTPHONE_HAS_SYSLOG 0
#endif

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <system_error>

namespace softphone::core {
namespace {

constexpr auto kRedacted = "<redacted>";

constexpr std::array kSensitiveKeys{
    std::string_view{"response"}, std::string_view{"password"}, std::string_view{"passwd"},
    std::string_view{"secret"},   std::string_view{"pwd"},
};

[[nodiscard]] bool isKeyCharacter(char character) noexcept
{
    return (std::isalnum(static_cast<unsigned char>(character)) != 0) || character == '_' ||
           character == '-';
}

[[nodiscard]] bool matchesKeyAt(std::string_view text, std::size_t position, std::string_view key)
{
    if (position + key.size() > text.size()) {
        return false;
    }

    for (std::size_t i = 0; i < key.size(); ++i) {
        const auto left = std::tolower(static_cast<unsigned char>(text[position + i]));
        const auto right = std::tolower(static_cast<unsigned char>(key[i]));
        if (left != right) {
            return false;
        }
    }

    if (position > 0 && isKeyCharacter(text[position - 1])) {
        return false;
    }

    return true;
}

}

std::string_view toString(LogLevel level) noexcept
{
    switch (level) {
    case LogLevel::Error:
        return "error";
    case LogLevel::Warning:
        return "warning";
    case LogLevel::Info:
        return "info";
    case LogLevel::Debug:
        return "debug";
    case LogLevel::Trace:
        return "trace";
    }
    return "unknown";
}

std::optional<LogLevel> parseLogLevel(std::string_view text) noexcept
{
    if (text == "error") {
        return LogLevel::Error;
    }
    if (text == "warning") {
        return LogLevel::Warning;
    }
    if (text == "info") {
        return LogLevel::Info;
    }
    if (text == "debug") {
        return LogLevel::Debug;
    }
    if (text == "trace") {
        return LogLevel::Trace;
    }
    return std::nullopt;
}

std::optional<LogLevel> logLevelFromEnvironment()
{
    if (const auto *text = std::getenv("SOFTPHONE_LOG_LEVEL"); text != nullptr) {
        return parseLogLevel(text);
    }
    return std::nullopt;
}

int pjsipLogLevelFor(LogLevel level) noexcept
{
    switch (level) {
    case LogLevel::Error:
        return 1;
    case LogLevel::Warning:
        return 2;
    case LogLevel::Info:
        return 3;
    case LogLevel::Debug:
        return 4;
    case LogLevel::Trace:

        return 5;
    }
    return 3;
}

namespace {

[[nodiscard]] bool partOfAPath(char letter) noexcept
{
    switch (letter) {
    case ' ':
    case '\t':
    case '\r':
    case '\n':
    case ']':
    case ')':
    case '>':
    case '"':
    case '\'':
        return false;
    default:
        return true;
    }
}

[[nodiscard]] bool isSeparator(char letter) noexcept
{
    return letter == '/' || letter == '\\';
}

[[nodiscard]] bool namesSource(std::string_view run)
{
    while (!run.empty() && std::isdigit(static_cast<unsigned char>(run.back())) != 0) {
        run.remove_suffix(1);
    }
    if (!run.empty() && run.back() == ':') {
        run.remove_suffix(1);
    }

    for (const auto *extension : {".c", ".cc", ".cpp", ".cxx", ".h", ".hpp"}) {
        if (run.size() > std::string_view{extension}.size() && run.ends_with(extension)) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] std::string worthKeeping(std::string_view run)
{
    std::size_t at = 0;
    while (at < run.size()) {
        const auto end = run.find_first_of("/\\", at);
        const auto segment = run.substr(at, end == std::string_view::npos ? end : end - at);
        if (segment.starts_with("pj")) {
            return std::string{run.substr(at)};
        }
        if (end == std::string_view::npos) {
            break;
        }
        at = end + 1;
    }

    const auto last = run.find_last_of("/\\");
    const auto tail = last == std::string_view::npos ? run : run.substr(last + 1);
    return tail.empty() ? std::string{"…"} : std::string{tail};
}

}

std::string shortenBuildPaths(std::string_view text)
{
    std::string out;
    out.reserve(text.size());

    std::size_t at = 0;
    while (at < text.size()) {
        const auto letter = text[at];

        const bool drive = at + 2 < text.size() &&
                           std::isalpha(static_cast<unsigned char>(letter)) != 0 &&
                           text[at + 1] == ':' && isSeparator(text[at + 2]) &&
                           (at == 0 || std::isalnum(static_cast<unsigned char>(text[at - 1])) == 0);

        const bool rooted =
            letter == '/' && (at == 0 || !partOfAPath(text[at - 1]) || text[at - 1] == '[');

        if (!drive && !rooted) {
            out.push_back(letter);
            ++at;
            continue;
        }

        auto end = at;
        while (end < text.size() && partOfAPath(text[end])) {
            ++end;
        }

        const auto run = text.substr(at, end - at);
        if (drive || namesSource(run)) {
            out += worthKeeping(run);
        } else {
            out += run;
        }
        at = end;
    }

    return out;
}

std::string redact(std::string_view text)
{
    std::string result;
    result.reserve(text.size());

    std::size_t index = 0;
    while (index < text.size()) {
        bool redactedHere = false;

        for (const auto &key : kSensitiveKeys) {
            if (!matchesKeyAt(text, index, key)) {
                continue;
            }

            auto after = index + key.size();
            while (after < text.size() && (text[after] == ' ' || text[after] == '\t')) {
                ++after;
            }
            if (after >= text.size() || (text[after] != '=' && text[after] != ':')) {
                continue;
            }
            ++after;
            while (after < text.size() && (text[after] == ' ' || text[after] == '\t')) {
                ++after;
            }

            result.append(text.substr(index, key.size()));
            result.push_back('=');

            if (after < text.size() && text[after] == '"') {

                auto end = text.find('"', after + 1);
                if (end == std::string_view::npos) {
                    end = text.size();
                } else {
                    ++end;
                }
                result.push_back('"');
                result.append(kRedacted);
                result.push_back('"');
                index = end;
            } else {

                auto end = after;
                while (end < text.size() && text[end] != ',' && text[end] != ';' &&
                       text[end] != '\r' && text[end] != '\n' && text[end] != ' ') {
                    ++end;
                }
                result.append(kRedacted);
                index = end;
            }

            redactedHere = true;
            break;
        }

        if (!redactedHere) {
            result.push_back(text[index]);
            ++index;
        }
    }

    return result;
}

std::shared_ptr<ILogSink> makeSink(std::filesystem::path file, bool alsoSystemLog)
{
    auto toFile = std::make_shared<FileLogSink>(std::move(file));
    if (!alsoSystemLog || !SystemLogSink::available()) {
        return toFile;
    }

    std::vector<std::shared_ptr<ILogSink>> sinks;
    sinks.push_back(std::move(toFile));
    sinks.push_back(std::make_shared<SystemLogSink>());
    return std::make_shared<FanOutSink>(std::move(sinks));
}

std::filesystem::path defaultLogFile()
{
    return logFileIn(stateDirectory());
}

std::filesystem::path logFileIn(const std::filesystem::path &root)
{
    return root / "ai-softphone.log";
}

FileLogSink::FileLogSink(std::filesystem::path file) : file_{std::move(file)}
{
    std::error_code error;
    std::filesystem::create_directories(file_.parent_path(), error);
}

void FileLogSink::rollOverIfLarge()
{
    std::error_code error;
    const auto size = std::filesystem::file_size(file_, error);
    if (error || size < kMaxBytes) {
        return;
    }

    auto previous = file_;
    previous += ".1";
    std::filesystem::rename(file_, previous, error);
}

bool FileLogSink::clear()
{
    const std::lock_guard lock{mutex_};

    std::error_code error;
    auto previous = file_;
    previous += ".1";
    std::filesystem::remove(previous, error);

    std::ofstream file{file_, std::ios::trunc | std::ios::binary};
    return static_cast<bool>(file);
}

std::uintmax_t FileLogSink::size() const
{
    std::error_code error;
    const auto size = std::filesystem::file_size(file_, error);
    return error ? 0 : size;
}

void FileLogSink::write(const LogRecord &record)
{
    const std::lock_guard lock{mutex_};

    rollOverIfLarge();

    std::ofstream file{file_, std::ios::app | std::ios::binary};
    if (!file) {
        return;
    }

    const auto milliseconds =
        std::chrono::duration_cast<std::chrono::milliseconds>(record.at.time_since_epoch()) % 1000;

    const std::tm parts = localParts(record.at);

    std::array<char, 32> stamp{};
    std::strftime(stamp.data(), stamp.size(), "%Y-%m-%dT%H:%M:%S", &parts);

    std::array<char, 8> fraction{};
    std::snprintf(fraction.data(), fraction.size(), ".%03d",
                  static_cast<int>(milliseconds.count()));

    file << stamp.data() << fraction.data() << ' ' << toString(record.level) << ' '
         << record.category << ": " << record.message;

    for (const auto &attribute : record.attributes) {
        file << ' ' << attribute.name << '=' << attribute.value;
    }

    file << '\n';
}

FanOutSink::FanOutSink(std::vector<std::shared_ptr<ILogSink>> sinks) : sinks_{std::move(sinks)}
{
    std::erase(sinks_, nullptr);
}

void FanOutSink::write(const LogRecord &record)
{
    for (const auto &sink : sinks_) {
        sink->write(record);
    }
}

bool FanOutSink::clear()
{

    bool cleared = false;
    for (const auto &sink : sinks_) {
        cleared = sink->clear() || cleared;
    }
    return cleared;
}

SystemLogSink::SystemLogSink(std::string identity) : identity_{std::move(identity)}
{
#if SOFTPHONE_HAS_SYSLOG

    ::openlog(identity_.c_str(), LOG_PID, LOG_USER);
#endif
}

SystemLogSink::~SystemLogSink()
{
#if SOFTPHONE_HAS_SYSLOG
    ::closelog();
#endif
}

bool SystemLogSink::available() noexcept
{
#if SOFTPHONE_HAS_SYSLOG
    return true;
#else
    return false;
#endif
}

void SystemLogSink::write([[maybe_unused]] const LogRecord &record)
{
#if SOFTPHONE_HAS_SYSLOG

    const int priority = record.level == LogLevel::Error     ? LOG_ERR
                         : record.level == LogLevel::Warning ? LOG_WARNING
                         : record.level == LogLevel::Info    ? LOG_INFO
                                                             : LOG_DEBUG;

    std::string line{record.category};
    line += ": ";
    line.append(record.message);
    for (const auto &attribute : record.attributes) {
        line += ' ';
        line.append(attribute.name);
        line += '=';
        line.append(attribute.value);
    }

    ::syslog(priority, "%s", line.c_str());
#endif
}

Logger &Logger::instance()
{
    static Logger logger;
    return logger;
}

void Logger::setSink(std::shared_ptr<ILogSink> sink)
{
    const std::lock_guard lock{mutex_};
    sink_ = std::move(sink);
}

void Logger::setLevel(LogLevel level)
{
    const std::lock_guard lock{mutex_};
    level_ = level;
}

LogLevel Logger::level() const
{
    const std::lock_guard lock{mutex_};
    return level_;
}

bool Logger::clear()
{
    std::shared_ptr<ILogSink> sink;
    {

        const std::lock_guard lock{mutex_};
        sink = sink_;
    }
    return sink && sink->clear();
}

void Logger::log(LogLevel level, std::string_view category, std::string_view message)
{
    log(level, category, message, {});
}

void Logger::log(LogLevel level, std::string_view category, std::string_view message,
                 std::span<const LogAttribute> attributes)
{
    std::shared_ptr<ILogSink> sink;
    {
        const std::lock_guard lock{mutex_};
        if (level > level_ || !sink_) {
            return;
        }
        sink = sink_;
    }

    const auto safeMessage = redact(message);

    std::vector<std::string> safeValues;
    safeValues.reserve(attributes.size());
    for (const auto &attribute : attributes) {
        safeValues.push_back(redact(attribute.value));
    }

    std::vector<LogAttribute> safeAttributes;
    safeAttributes.reserve(attributes.size());
    for (std::size_t at = 0; at < attributes.size(); ++at) {
        safeAttributes.push_back(LogAttribute{attributes[at].name, safeValues[at]});
    }

    sink->write(
        LogRecord{level, std::chrono::system_clock::now(), category, safeMessage, safeAttributes});

    leaveBreadcrumb(std::string{toString(level)} + " " + std::string{category} + ": " +
                    safeMessage);
}

}
