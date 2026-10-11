// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/capture/applications.hpp>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <utility>

namespace softphone::capture {
namespace {

constexpr std::string_view kShipped =
    R"(# What to call a conversation found in somebody else's application.
# app | process | name | venue        (venue empty unless it is a browser)

app | zoom             | Zoom
app | zoom.us          | Zoom
app | teams            | Microsoft Teams
app | ms-teams         | Microsoft Teams
app | microsoft teams  | Microsoft Teams
app | teams-for-linux  | Microsoft Teams
app | slack            | Slack
app | discord          | Discord
app | webex            | Webex
app | telegram-desktop | Telegram
app | telegram         | Telegram
app | whatsapp         | WhatsApp

app | chrome           | Meeting in Chrome   | Chrome
app | google chrome    | Meeting in Chrome   | Chrome
app | chromium         | Meeting in Chromium | Chromium
app | firefox          | Meeting in Firefox  | Firefox
app | safari           | Meeting in Safari   | Safari
app | msedge           | Meeting in Edge     | Edge
app | msedgewebview2   | Meeting in Edge     | Edge
app | brave            | Meeting in Brave    | Brave
app | vivaldi          | Meeting in Vivaldi  | Vivaldi

# Never a conversation, whatever they do with a microphone: the desktop shell
# drawing a level meter, a mixer, the sound server's own tools. Matched whole
# and not as a prefix, so an application called `plasma-meeting` is recorded
# like anything else. A miss here costs a row somebody deletes; a false
# exclusion costs a conversation nobody has.
never | cinnamon
never | gnome-shell
never | plasmashell
never | xfce4-volumed
never | xfce4-mixer
never | pavucontrol
never | pulseaudio
never | pipewire
never | wireplumber
never | kmix
never | speech-dispatcher
)";

Applications &table()
{
    static Applications kept = readApplications(kShipped);
    return kept;
}

[[nodiscard]] std::vector<std::string> fieldsOf(std::string_view line)
{
    std::vector<std::string> fields;
    std::size_t at = 0;
    while (at <= line.size()) {
        const auto bar = line.find('|', at);
        const auto until = bar == std::string_view::npos ? line.size() : bar;
        auto one = line.substr(at, until - at);
        while (!one.empty() && (one.front() == ' ' || one.front() == '\t')) {
            one.remove_prefix(1);
        }
        while (!one.empty() && (one.back() == ' ' || one.back() == '\t' || one.back() == '\r')) {
            one.remove_suffix(1);
        }
        fields.emplace_back(one);
        if (bar == std::string_view::npos) {
            break;
        }
        at = bar + 1;
    }
    return fields;
}

}

std::string plainly(std::string_view process)
{
    std::string plain{process};
    std::transform(plain.begin(), plain.end(), plain.begin(),
                   [](unsigned char letter) { return static_cast<char>(std::tolower(letter)); });
    if (plain.size() > 4 && plain.compare(plain.size() - 4, 4, ".exe") == 0) {
        plain.erase(plain.size() - 4);
    }
    return plain;
}

std::string_view shippedApplications()
{
    return kShipped;
}

Applications readApplications(std::string_view text)
{
    Applications read;
    std::size_t at = 0;
    while (at < text.size()) {
        const auto ends = text.find('\n', at);
        auto line = text.substr(at, ends == std::string_view::npos ? text.size() - at : ends - at);
        at = ends == std::string_view::npos ? text.size() : ends + 1;

        if (const auto hash = line.find('#'); hash != std::string_view::npos) {
            line = line.substr(0, hash);
        }

        const auto fields = fieldsOf(line);
        if (fields.empty()) {
            continue;
        }
        if (fields.front() == "app" && fields.size() >= 3 && !fields[1].empty()) {
            read.known.push_back(KnownApplication{plainly(fields[1]), fields[2],
                                                  fields.size() > 3 ? fields[3] : std::string{}});
        } else if (fields.front() == "never" && fields.size() >= 2 && !fields[1].empty()) {
            read.never.push_back(plainly(fields[1]));
        }

    }
    return read;
}

void useApplications(Applications replacement)
{
    table() = std::move(replacement);
}

std::string_view knownApplication(std::string_view process)
{
    const auto plain = plainly(process);
    const auto &known = table().known;
    const auto found = std::ranges::find_if(
        known, [&plain](const KnownApplication &one) { return one.process == plain; });
    return found == known.end() ? std::string_view{} : std::string_view{found->name};
}

bool neverAConversation(std::string_view process)
{
    if (process.empty()) {
        return false;
    }
    const auto plain = plainly(process);
    const auto &never = table().never;
    return std::ranges::find(never, plain) != never.end();
}

std::string_view venueOf(std::string_view process)
{
    if (process.empty()) {
        return {};
    }
    const auto plain = plainly(process);
    const auto &known = table().known;
    const auto found = std::ranges::find_if(
        known, [&plain](const KnownApplication &one) { return one.process == plain; });
    return found == known.end() ? std::string_view{} : std::string_view{found->venue};
}

std::string nameFor(std::string_view process, std::string_view title)
{
    if (const auto known = knownApplication(process); !known.empty()) {
        return std::string{known};
    }
    if (!process.empty()) {
        return std::string{process};
    }
    if (!title.empty()) {
        return std::string{title};
    }
    return {};
}

}
