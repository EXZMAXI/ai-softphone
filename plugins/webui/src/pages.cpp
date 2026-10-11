// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/plugins/webui/pages.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <string>
#include <string_view>

namespace softphone::plugins::webui {
namespace {

constexpr std::string_view kStyle = R"(
:root { color-scheme: light dark; --line: #8884; }
body { font: 15px/1.5 system-ui, sans-serif; margin: 0 auto; max-width: 46rem; padding: 2rem 1rem; }
h1 { font-size: 1.4rem; margin: 0 0 1.5rem; }
h2 { font-size: 1.1rem; margin: 2rem 0 .5rem; }
a { color: inherit; }
table { border-collapse: collapse; width: 100%; }
td, th { border-bottom: 1px solid var(--line); padding: .5rem .4rem; text-align: left; }
th { font-weight: 600; }
td.n { text-align: right; white-space: nowrap; }
dl { display: grid; grid-template-columns: max-content 1fr; gap: .3rem 1.5rem; margin: 0; }
dt { opacity: .65; }
dd { margin: 0; }
.turn { display: grid; grid-template-columns: 4.5rem 4rem 1fr; gap: .6rem; padding: .35rem 0; }
.turn span { opacity: .65; font-variant-numeric: tabular-nums; }
.empty { opacity: .65; padding: 2rem 0; }
audio { width: 100%; margin: 1.5rem 0 0; }
)";

void appendShell(std::string &out, const Words &words, std::string_view title,
                 std::string_view body)
{

    out += "<!doctype html>\n<html lang=\"";
    out += words.tag;
    out += "\"><head><meta charset=\"utf-8\">";
    out += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
    out += "<title>";
    out += title;
    out += "</title><style>";
    out += kStyle;
    out += "</style></head><body>";
    out += body;
    out += "</body></html>\n";
}

[[nodiscard]] std::string shell(const Words &words, std::string_view title, std::string_view body)
{
    std::string out;
    out.reserve(body.size() + kStyle.size() + 256);
    appendShell(out, words, title, body);
    return out;
}

[[nodiscard]] std::string who(const core::Recording &recording, const Words &words)
{
    if (!recording.remoteName.empty()) {
        return recording.remoteName;
    }
    if (!recording.remoteNumber.empty()) {
        return recording.remoteNumber;
    }
    return std::string{words.unknownParty};
}

}

std::string escape(std::string_view text)
{
    std::string out;
    out.reserve(text.size());
    for (const char c : text) {
        switch (c) {
        case '&':
            out += "&amp;";
            break;
        case '<':
            out += "&lt;";
            break;
        case '>':
            out += "&gt;";
            break;
        case '"':
            out += "&quot;";
            break;
        case '\'':
            out += "&#39;";
            break;
        default:
            out.push_back(c);
            break;
        }
    }
    return out;
}

std::string asClock(std::chrono::milliseconds length)
{
    if (length < std::chrono::milliseconds::zero()) {
        length = std::chrono::milliseconds::zero();
    }
    const auto total = static_cast<long long>(length.count() / 1000);
    const auto hours = total / 3600;
    const auto minutes = (total % 3600) / 60;
    const auto seconds = total % 60;

    std::array<char, 32> text{};
    if (hours > 0) {
        std::snprintf(text.data(), text.size(), "%lld:%02lld:%02lld", hours, minutes, seconds);
    } else {
        std::snprintf(text.data(), text.size(), "%lld:%02lld", minutes, seconds);
    }
    return std::string{text.data()};
}

std::string asStamp(std::chrono::system_clock::time_point when, const Words &words)
{
    const auto seconds = std::chrono::system_clock::to_time_t(when);
    std::tm parts{};
#if defined(_WIN32)
    if (::localtime_s(&parts, &seconds) != 0) {
        return {};
    }
#else
    if (::localtime_r(&seconds, &parts) == nullptr) {
        return {};
    }
#endif

    std::array<char, 32> text{};
    const auto year = parts.tm_year + 1900;
    const auto month = parts.tm_mon + 1;
    if (words.date == Words::Date::DayFirst) {
        std::snprintf(text.data(), text.size(), "%02d.%02d.%04d %02d:%02d", parts.tm_mday, month,
                      year, parts.tm_hour, parts.tm_min);
    } else {
        std::snprintf(text.data(), text.size(), "%04d-%02d-%02d %02d:%02d", year, month,
                      parts.tm_mday, parts.tm_hour, parts.tm_min);
    }
    return std::string{text.data()};
}

std::string listPage(const std::vector<core::Recording> &recordings, const Words &words)
{
    std::string body = "<h1>";
    body += escape(words.recordings);
    body += "</h1>";

    if (recordings.empty()) {

        body += "<p class=\"empty\">";
        body += escape(words.nothingRecorded);
        body += "</p>";
        return shell(words, words.recordings, body);
    }

    body += "<table><tr><th>";
    body += escape(words.when);
    body += "</th><th>";
    body += escape(words.who);
    body += "</th><th class=\"n\">";
    body += escape(words.length);
    body += "</th></tr>";
    for (const auto &recording : recordings) {
        body += "<tr><td><a href=\"/ui/recordings/";
        body += escape(recording.id);
        body += "\">";
        body += escape(asStamp(recording.startedAt, words));
        body += "</a></td><td>";
        body += escape(who(recording, words));
        body += "</td><td class=\"n\">";
        body += escape(asClock(recording.duration));
        body += "</td></tr>";
    }
    body += "</table>";
    return shell(words, words.recordings, body);
}

std::string recordingPage(const Words &words, const core::Recording &recording,
                          const std::optional<core::Transcript> &transcript)
{
    const auto title = who(recording, words) + ", " + asStamp(recording.startedAt, words);

    std::string body = "<p><a href=\"/ui\">&larr; ";
    body += escape(words.back);
    body += "</a></p><h1>";
    body += escape(title);
    body += "</h1><dl>";

    const auto row = [&body](std::string_view name, std::string_view value) {
        if (value.empty()) {
            return;
        }
        body += "<dt>";
        body += name;
        body += "</dt><dd>";
        body += escape(value);
        body += "</dd>";
    };

    row(words.when, asStamp(recording.startedAt, words));
    row(words.length, asClock(recording.duration));
    row(words.number, recording.remoteNumber);
    row(words.parties, std::to_string(recording.partyCount));
    row(words.format, recording.format);
    body += "</dl>";

    body += "<audio controls preload=\"none\" src=\"/ui/recordings/";
    body += escape(recording.id);
    body += "/audio\"></audio>";

    body += "<h2>";
    body += escape(words.transcript);
    body += "</h2>";
    if (!transcript.has_value() || transcript->segments.empty()) {
        body += "<p class=\"empty\">";
        body += escape(words.notTranscribed);
        body += "</p>";
        return shell(words, title, body);
    }

    for (const auto &segment : transcript->segments) {
        body += "<div class=\"turn\"><span>";
        body += escape(asClock(segment.start));
        body += "</span><span>";
        body += escape(segment.speaker.empty() ? segment.voice : segment.speaker);
        body += "</span><div>";
        body += escape(segment.editedText.empty() ? segment.text : segment.editedText);
        body += "</div></div>";
    }
    return shell(words, title, body);
}

std::string missingPage(const Words &words)
{
    std::string body = "<h1>";
    body += escape(words.notHere);
    body += "</h1><p class=\"empty\">";
    body += escape(words.noSuchRecording);
    body += "</p><p><a href=\"/ui\">&larr; ";
    body += escape(words.back);
    body += "</a></p>";
    return shell(words, words.notHere, body);
}

std::string unavailablePage(const Words &words, std::string_view what)
{
    std::string body = "<h1>";
    body += escape(words.notAvailable);
    body += "</h1><p class=\"empty\">";
    body += escape(what);
    body += "</p>";
    return shell(words, words.notAvailable, body);
}

}
