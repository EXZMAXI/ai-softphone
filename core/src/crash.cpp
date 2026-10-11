// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/crash.hpp>

#include "crash_detail.hpp"

#include <softphone/core/logging.hpp>
#include <softphone/core/version.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <string_view>
#include <system_error>

#if defined(_WIN32)
#include <io.h>
#else
#include <unistd.h>
#endif

namespace softphone::core {
namespace crashDetail {
namespace {

std::array<char, 512> bannerText{};
std::size_t bannerLength{0};

std::atomic<unsigned> reportsWritten{0};

void writeAll(int descriptor, std::string_view text) noexcept
{
    std::size_t written = 0;
    while (written < text.size()) {
#if defined(_WIN32)
        const auto wrote = ::_write(descriptor, text.data() + written,
                                    static_cast<unsigned>(text.size() - written));
#else
        const auto wrote = ::write(descriptor, text.data() + written, text.size() - written);
#endif
        if (wrote <= 0) {
            return;
        }
        written += static_cast<std::size_t>(wrote);
    }
}

std::size_t appendNumber(std::array<char, kPathLength> &into, std::size_t at,
                         std::uint64_t value) noexcept
{
    std::array<char, 24> digits{};
    std::size_t count = 0;
    do {
        digits.at(count++) = static_cast<char>('0' + (value % 10));
        value /= 10;
    } while (value != 0 && count < digits.size());

    while (count > 0 && at + 1 < into.size()) {
        into.at(at++) = digits.at(--count);
    }
    into.at(at) = '\0';
    return at;
}

}

Breadcrumbs &breadcrumbs() noexcept
{

    static Breadcrumbs *ring = new Breadcrumbs{};
    return *ring;
}

Destination &destination() noexcept
{
    static Destination *where = new Destination{};
    return *where;
}

std::size_t append(std::array<char, kPathLength> &into, std::size_t at,
                   std::string_view text) noexcept
{
    const auto room = into.size() - 1 - at;
    const auto take = std::min(room, text.size());
    std::memcpy(into.data() + at, text.data(), take);
    into.at(at + take) = '\0';
    return at + take;
}

void nameTheFiles() noexcept
{
    auto &where = destination();

    const auto seconds =
        static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(
                                       std::chrono::system_clock::now().time_since_epoch())
                                       .count());
    const auto ordinal = reportsWritten.fetch_add(1, std::memory_order_relaxed);

    for (auto *target : {&where.report, &where.dump}) {
        std::size_t at = append(*target, 0, {where.directory.data(), where.directoryLength});
        at = append(*target, at, "crash-");
        at = appendNumber(*target, at, seconds);
        if (ordinal != 0) {
            at = append(*target, at, "-");
            at = appendNumber(*target, at, ordinal);
        }
        at = append(*target, at, target == &where.report ? ".txt" : ".dmp");
        static_cast<void>(at);
    }
}

void prepareBanner() noexcept
{
    std::string text;
    text.reserve(bannerText.size());
    text += "ai-softphone crash report\nversion: ";
    text += versionString();
    text += "\nplatform: ";
    text += platformName();
    text += "\ndelivered as: ";
    text += packageFormat();
    text += '\n';

    bannerLength = std::min(text.size(), bannerText.size());
    std::memcpy(bannerText.data(), text.data(), bannerLength);
}

std::string_view banner() noexcept
{
    return {bannerText.data(), bannerLength};
}

void writeReport(int descriptor, std::string_view what) noexcept
{
    writeAll(descriptor, banner());
    writeAll(descriptor, "reason: ");
    writeAll(descriptor, what);
    writeAll(descriptor, "\n\nthe last lines the phone said, oldest first:\n");

    auto &ring = breadcrumbs();
    std::uint64_t after = 0;
    for (std::size_t taken = 0; taken < ring.lines.size(); ++taken) {
        const Breadcrumbs::Line *next = nullptr;
        std::uint64_t bestSequence = 0;

        for (const auto &line : ring.lines) {
            const auto sequence = line.sequence.load(std::memory_order_acquire);
            if (sequence <= after) {
                continue;
            }
            if (next == nullptr || sequence < bestSequence) {
                next = &line;
                bestSequence = sequence;
            }
        }

        if (next == nullptr) {
            break;
        }

        writeAll(descriptor, "  ");
        writeAll(descriptor, next->text.data());
        writeAll(descriptor, "\n");
        after = bestSequence;
    }
}

std::string_view markerName() noexcept
{
    return "running.mark";
}

}

void leaveBreadcrumb(std::string_view text) noexcept
{
    auto &ring = crashDetail::breadcrumbs();

    const auto sequence = ring.next.fetch_add(1, std::memory_order_relaxed);
    auto &line = ring.lines.at((sequence - 1) % ring.lines.size());

    line.sequence.store(0, std::memory_order_release);

    const auto take = std::min(text.size(), crashDetail::kBreadcrumbLength - 1);
    std::memcpy(line.text.data(), text.data(), take);
    line.text.at(take) = '\0';

    line.sequence.store(sequence, std::memory_order_release);
}

void writeCrashReport(int descriptor, std::string_view reason) noexcept
{
    crashDetail::writeReport(descriptor, reason);
}

namespace {

std::optional<CrashReport> whatIsThere()
{
    const auto &where = crashDetail::destination();
    if (where.directoryLength == 0) {
        return std::nullopt;
    }

    const std::filesystem::path directory{
        std::string{where.directory.data(), where.directoryLength}};

    std::error_code failure;
    if (!std::filesystem::exists(directory / crashDetail::markerName(), failure)) {
        return std::nullopt;
    }

    CrashReport found;
    std::filesystem::file_time_type newest{};

    for (const auto &entry : std::filesystem::directory_iterator{directory, failure}) {
        if (!entry.is_regular_file(failure)) {
            continue;
        }
        const auto name = entry.path().filename().string();
        if (!name.starts_with("crash-") || !name.ends_with(".txt")) {
            continue;
        }
        const auto written = entry.last_write_time(failure);
        if (found.report.empty() || written > newest) {
            newest = written;
            found.report = entry.path();
            found.when = name.substr(6, name.size() - 10);
        }
    }

    if (found.report.empty()) {

        return CrashReport{};
    }

    auto dump = found.report;
    dump.replace_extension(".dmp");
    if (std::filesystem::exists(dump, failure)) {
        found.dump = dump;
    }
    return found;
}

struct Memo {
    bool taken{false};
    std::optional<CrashReport> answer;
};

Memo &memo()
{
    static Memo one;
    return one;
}

}

namespace crashDetail {

void rememberPreviousRun()
{
    auto &one = memo();
    one.answer = whatIsThere();
    one.taken = true;
}

}

std::optional<CrashReport> lastRunCrashed()
{

    const auto &one = memo();
    return one.taken ? one.answer : whatIsThere();
}

void discardCrashReport(const CrashReport &report) noexcept
{
    std::error_code failure;
    if (!report.report.empty()) {
        std::filesystem::remove(report.report, failure);
    }
    if (!report.dump.empty()) {
        std::filesystem::remove(report.dump, failure);
    }
}

void markCleanExit() noexcept
{
    const auto &where = crashDetail::destination();
    if (where.directoryLength == 0) {
        return;
    }

    std::error_code failure;
    std::filesystem::remove(
        std::filesystem::path{std::string{where.directory.data(), where.directoryLength}} /
            crashDetail::markerName(),
        failure);
}

std::optional<int> answeredCrashDumpRequest(int argc, char **argv)
{
    if (argv == nullptr) {
        return std::nullopt;
    }

    for (int at = 1; at < argc; ++at) {
        if (argv[at] == nullptr || std::string_view{argv[at]} != kWriteCrashDumpOption) {
            continue;
        }

        if (at + 4 >= argc) {
            return EXIT_FAILURE;
        }

        const auto number = [](const char *text) -> unsigned long long {
            if (text == nullptr) {
                return 0ULL;
            }
            char *end = nullptr;
            const auto value = std::strtoull(text, &end, 10);
            return end == text ? 0ULL : value;
        };

        const bool wrote = writeDumpOfAnotherProcess(
            static_cast<unsigned long>(number(argv[at + 1])),
            static_cast<unsigned long>(number(argv[at + 2])), number(argv[at + 3]),
            argv[at + 4] == nullptr ? std::string{} : std::string{argv[at + 4]});
        return wrote ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    return std::nullopt;
}

}
