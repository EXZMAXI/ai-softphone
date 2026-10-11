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
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace softphone::core {

enum class LogLevel {
    Error,
    Warning,
    Info,
    Debug,

    Trace,
};

[[nodiscard]] std::string_view toString(LogLevel level) noexcept;

[[nodiscard]] std::optional<LogLevel> parseLogLevel(std::string_view text) noexcept;

[[nodiscard]] std::optional<LogLevel> logLevelFromEnvironment();

[[nodiscard]] int pjsipLogLevelFor(LogLevel level) noexcept;

[[nodiscard]] std::string redact(std::string_view text);

[[nodiscard]] std::string shortenBuildPaths(std::string_view text);

struct LogAttribute {
    std::string_view name;
    std::string_view value;

    constexpr LogAttribute(std::string_view named, std::string_view text) noexcept
        : name{named}, value{text}
    {}

    constexpr LogAttribute(std::string_view named, const char *text) noexcept
        : name{named}, value{text}
    {}

    LogAttribute(std::string_view named, std::string &&text) = delete;
};

struct LogRecord {
    LogLevel level{LogLevel::Info};

    std::chrono::system_clock::time_point at{};

    std::string_view category;
    std::string_view message;

    std::span<const LogAttribute> attributes;
};

class ILogSink {
public:
    ILogSink() = default;
    ILogSink(const ILogSink &) = delete;
    ILogSink &operator=(const ILogSink &) = delete;
    ILogSink(ILogSink &&) = delete;
    ILogSink &operator=(ILogSink &&) = delete;
    virtual ~ILogSink() = default;

    virtual void write(const LogRecord &record) = 0;

    [[nodiscard]] virtual bool clear()
    {
        return false;
    }
};

class FileLogSink final : public ILogSink {
public:

    static constexpr std::uintmax_t kMaxBytes = 4 * 1024 * 1024;

    explicit FileLogSink(std::filesystem::path file);

    [[nodiscard]] const std::filesystem::path &file() const noexcept
    {
        return file_;
    }

    void write(const LogRecord &record) override;

    [[nodiscard]] bool clear() override;

    [[nodiscard]] std::uintmax_t size() const;

private:
    void rollOverIfLarge();

    std::filesystem::path file_;
    std::mutex mutex_;
};

class FanOutSink final : public ILogSink {
public:
    explicit FanOutSink(std::vector<std::shared_ptr<ILogSink>> sinks);

    void write(const LogRecord &record) override;

    [[nodiscard]] bool clear() override;

private:
    std::vector<std::shared_ptr<ILogSink>> sinks_;
};

class SystemLogSink final : public ILogSink {
public:

    static constexpr std::string_view kIdentity{"ai-softphone"};

    explicit SystemLogSink(std::string identity = std::string{kIdentity});
    ~SystemLogSink() override;

    [[nodiscard]] static bool available() noexcept;

    void write(const LogRecord &record) override;

private:
    std::string identity_;
};

[[nodiscard]] std::shared_ptr<ILogSink> makeSink(std::filesystem::path file, bool alsoSystemLog);

[[nodiscard]] std::filesystem::path defaultLogFile();

[[nodiscard]] std::filesystem::path logFileIn(const std::filesystem::path &root);

class Logger {
public:
    static Logger &instance();

    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;
    Logger(Logger &&) = delete;
    Logger &operator=(Logger &&) = delete;

    void setSink(std::shared_ptr<ILogSink> sink);
    void setLevel(LogLevel level);
    [[nodiscard]] LogLevel level() const;

    [[nodiscard]] bool clear();

    void log(LogLevel level, std::string_view category, std::string_view message);

    void log(LogLevel level, std::string_view category, std::string_view message,
             std::span<const LogAttribute> attributes);

private:
    Logger() = default;
    ~Logger() = default;

    mutable std::mutex mutex_;
    std::shared_ptr<ILogSink> sink_;
    LogLevel level_{LogLevel::Info};
};

}
