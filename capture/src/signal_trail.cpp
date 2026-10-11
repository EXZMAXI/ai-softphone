// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/capture/signal_trail.hpp>

#include <softphone/capture/detector.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>
#include <sstream>

namespace softphone::capture {

namespace {

constexpr std::array kFlags{'H', 'P', 'D', 'T'};

[[nodiscard]] bool isFlags(std::string_view word)
{
    if (word.size() != kFlags.size()) {
        return false;
    }
    for (std::size_t at = 0; at < kFlags.size(); ++at) {
        if (word[at] != kFlags.at(at) && word[at] != '.') {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool isNumber(std::string_view word)
{
    return !word.empty() && std::ranges::all_of(word, [](unsigned char letter) {
        return std::isdigit(letter) != 0;
    });
}

}

void SignalTrail::saw(const Reading &one)
{
    if (!readings_.empty() && readings_.back().sameAs(one)) {
        return;
    }
    readings_.push_back(one);
    if (readings_.size() > room_) {
        readings_.erase(readings_.begin());
    }
}

std::string SignalTrail::render(std::chrono::milliseconds settle,
                                std::chrono::milliseconds holdOff) const
{
    std::ostringstream out;
    out << "# trail: settle " << settle.count() << " ms, hold-off " << holdOff.count()
        << " ms; H held, P playing, D the holder's own output, T talking\n";
    for (const Reading &one : readings_) {
        out << one.at.count() << ' ' << (one.held ? kFlags[0] : '.')
            << (one.playing ? kFlags[1] : '.') << (one.duplex ? kFlags[2] : '.')
            << (one.talking ? kFlags[3] : '.') << '\n';
    }
    return out.str();
}

std::vector<Reading> SignalTrail::parse(std::string_view text)
{
    std::vector<Reading> found;
    std::istringstream lines{std::string{text}};
    std::string line;
    while (std::getline(lines, line)) {

        std::istringstream words{line};
        std::string word;
        std::string previous;
        while (words >> word) {
            if (isFlags(word) && isNumber(previous)) {
                Reading one;
                one.at = std::chrono::milliseconds{std::strtoll(previous.c_str(), nullptr, 10)};
                one.held = word[0] == kFlags[0];
                one.playing = word[1] == kFlags[1];
                one.duplex = word[2] == kFlags[2];
                one.talking = word[3] == kFlags[3];
                found.push_back(one);
                break;
            }
            previous = word;
        }
    }
    return found;
}

std::vector<Reading> replay(const std::vector<Reading> &readings, std::chrono::milliseconds settle,
                            std::chrono::milliseconds holdOff, std::chrono::milliseconds tick)
{
    std::vector<Reading> out;
    if (readings.empty() || tick <= std::chrono::milliseconds::zero()) {
        return out;
    }

    DetectorSettings settings;
    settings.settle = settle;
    settings.holdOff = holdOff;
    ConversationGate gate{settings};

    const std::chrono::steady_clock::time_point epoch{};
    const auto append = [&out](const Reading &one) {
        if (out.empty() || !out.back().sameAs(one)) {
            out.push_back(one);
        }
    };

    for (std::size_t index = 0; index < readings.size(); ++index) {
        const Reading &now = readings.at(index);

        const auto until = index + 1 < readings.size() ? readings.at(index + 1).at : now.at + tick;
        for (auto at = now.at; at < until; at += tick) {

            static_cast<void>(gate.saw(now.held, now.duplex, epoch + at));
            append(Reading{at, now.held, now.playing, now.duplex, gate.talking()});
        }
    }
    return out;
}

}
