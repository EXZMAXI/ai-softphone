// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "autostart.hpp"

#include <sstream>
#include <string>
#include <utility>

namespace softphone::ui {
namespace {

[[nodiscard]] std::string_view trim(std::string_view text)
{
    while (!text.empty() && (text.front() == ' ' || text.front() == '\t')) {
        text.remove_prefix(1);
    }
    while (!text.empty() && (text.back() == ' ' || text.back() == '\t' || text.back() == '\r')) {
        text.remove_suffix(1);
    }
    return text;
}

[[nodiscard]] bool isFalse(std::string_view value)
{
    return value == "false" || value == "0" || value == "no";
}

}

std::string autostartEntry(const std::string &binary, bool startMinimised)
{

    std::ostringstream entry;
    entry << "[Desktop Entry]\n"
          << "Type=Application\n"
          << "Version=1.5\n"
          << "Name=AI Softphone\n"
          << "Exec=" << binary;

    if (startMinimised) {
        entry << ' ' << kMinimisedArgument;
    }

    entry << "\n"
          << "Icon=ai-softphone\n"
          << "Terminal=false\n"

          << "X-GNOME-Autostart-enabled=true\n"
          << "Hidden=false\n";

    return entry.str();
}

AutostartState readAutostartEntry(std::string_view text)
{
    AutostartState state;

    std::string exec;
    bool hidden = false;
    bool switchedOff = false;
    bool sawEntry = false;

    std::istringstream lines{std::string{text}};
    for (std::string line; std::getline(lines, line);) {
        const auto trimmed = trim(line);
        if (trimmed.empty() || trimmed.front() == '#') {
            continue;
        }

        if (trimmed == "[Desktop Entry]") {
            sawEntry = true;
            continue;
        }

        const auto split = trimmed.find('=');
        if (split == std::string_view::npos) {
            continue;
        }

        const auto key = trim(trimmed.substr(0, split));
        const auto value = trim(trimmed.substr(split + 1));

        if (key == "Exec") {
            exec = std::string{value};
        } else if (key == "Hidden") {
            hidden = !isFalse(value);
        } else if (key == "X-GNOME-Autostart-enabled") {
            switchedOff = isFalse(value);
        }
    }

    if (!sawEntry || exec.empty()) {

        return state;
    }

    state.present = true;
    state.enabled = !hidden && !switchedOff;

    const auto flag = exec.find(std::string{kMinimisedArgument});
    state.startMinimised = flag != std::string::npos;
    state.command = flag == std::string::npos ? exec : exec.substr(0, flag);

    while (!state.command.empty() && state.command.back() == ' ') {
        state.command.pop_back();
    }

    return state;
}

std::string runCommand(const std::string &binary, bool startMinimised)
{
    std::string command = '"' + binary + '"';
    if (startMinimised) {
        command += ' ';
        command.append(kMinimisedArgument);
    }
    return command;
}

AutostartState readRunCommand(std::string_view text)
{
    AutostartState state;

    const auto trimmed = trim(text);
    if (trimmed.empty()) {
        return state;
    }

    state.present = true;

    state.enabled = true;

    if (trimmed.front() == '"') {
        if (const auto closing = trimmed.find('"', 1); closing != std::string_view::npos) {
            state.command = std::string{trimmed.substr(1, closing - 1)};
            state.startMinimised =
                trimmed.find(kMinimisedArgument, closing) != std::string_view::npos;
            return state;
        }
    }

    const auto space = trimmed.find(' ');
    state.command = std::string{trimmed.substr(0, space)};
    state.startMinimised = trimmed.find(kMinimisedArgument) != std::string_view::npos;
    return state;
}

namespace {

[[nodiscard]] std::string escaped(std::string_view text)
{
    std::string out;
    out.reserve(text.size());
    for (const char letter : text) {
        switch (letter) {
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
            out += "&apos;";
            break;
        default:
            out.push_back(letter);
            break;
        }
    }
    return out;
}

[[nodiscard]] std::string firstString(std::string_view text, std::size_t from)
{
    const auto opened = text.find("<string>", from);
    if (opened == std::string_view::npos) {
        return {};
    }
    const auto begins = opened + std::string_view{"<string>"}.size();
    const auto closed = text.find("</string>", begins);
    if (closed == std::string_view::npos) {
        return {};
    }

    std::string out{text.substr(begins, closed - begins)};
    for (const auto &[from_, to] :
         {std::pair{"&lt;", "<"}, std::pair{"&gt;", ">"}, std::pair{"&quot;", "\""},
          std::pair{"&apos;", "\'"}, std::pair{"&amp;", "&"}}) {
        for (auto at = out.find(from_); at != std::string::npos; at = out.find(from_, at + 1)) {
            out.replace(at, std::string_view{from_}.size(), to);
        }
    }
    return out;
}

}

std::string launchAgentPlist(const std::string &binary, bool startMinimised)
{
    std::string arguments = "        <string>" + escaped(binary) + "</string>\n";
    if (startMinimised) {
        arguments += "        <string>" + std::string{kMinimisedArgument} + "</string>\n";
    }

    return std::string{R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>Label</key>
    <string>)"} +
           std::string{kLaunchAgentLabel} + R"(</string>
    <key>ProgramArguments</key>
    <array>
)" + arguments +
           R"(    </array>
    <key>RunAtLoad</key>
    <true/>
</dict>
</plist>
)";
}

AutostartState readLaunchAgentPlist(std::string_view text)
{
    AutostartState state;
    const auto arguments = text.find("<key>ProgramArguments</key>");
    if (arguments == std::string_view::npos) {
        return state;
    }

    state.command = firstString(text, arguments);
    if (state.command.empty()) {

        return state;
    }

    state.present = true;

    const auto runAtLoad = text.find("<key>RunAtLoad</key>");
    state.enabled = runAtLoad == std::string_view::npos ||
                    text.find("<false/>", runAtLoad) > text.find("<true/>", runAtLoad);

    state.startMinimised = text.find(kMinimisedArgument, arguments) != std::string_view::npos;
    return state;
}

}
