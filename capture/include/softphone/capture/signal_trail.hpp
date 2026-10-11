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
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace softphone::capture {

struct Reading {

    std::chrono::milliseconds at{0};

    bool held{false};

    bool playing{false};

    bool duplex{false};

    bool talking{false};

    [[nodiscard]] bool operator==(const Reading &) const = default;

    [[nodiscard]] bool sameAs(const Reading &other) const noexcept
    {
        return held == other.held && playing == other.playing && duplex == other.duplex &&
               talking == other.talking;
    }
};

class SignalTrail {
public:

    static constexpr std::size_t kRoom = 200;

    explicit SignalTrail(std::size_t room = kRoom) : room_{room == 0 ? 1 : room} {}

    void saw(const Reading &one);

    [[nodiscard]] std::vector<Reading> readings() const
    {
        return readings_;
    }

    [[nodiscard]] bool empty() const noexcept
    {
        return readings_.empty();
    }

    void forget()
    {
        readings_.clear();
    }

    [[nodiscard]] std::string render(std::chrono::milliseconds settle,
                                     std::chrono::milliseconds holdOff) const;

    [[nodiscard]] static std::vector<Reading> parse(std::string_view text);

private:
    std::size_t room_;
    std::vector<Reading> readings_;
};

[[nodiscard]] std::vector<Reading> replay(const std::vector<Reading> &readings,
                                          std::chrono::milliseconds settle,
                                          std::chrono::milliseconds holdOff,
                                          std::chrono::milliseconds tick);

}
