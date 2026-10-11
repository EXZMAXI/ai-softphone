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

#include <softphone/core/audio_encoder.hpp>

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace softphone::core {

class TimelineWriter {
public:

    TimelineWriter(unsigned channels, IAudioEncoder &encoder, std::uint64_t start = 0);

    void offer(unsigned channel, std::uint64_t position, std::span<const std::int16_t> samples);

    void mute(std::uint64_t from, bool muted);

    void settled(std::uint64_t upTo);

    void mute(unsigned channel, std::uint64_t from, bool muted);

    EncoderStatus advance(std::uint64_t upTo);

    [[nodiscard]] std::optional<std::uint64_t> quietFrom() const;

    EncoderStatus finish();

    [[nodiscard]] std::uint64_t head() const noexcept
    {
        return head_;
    }

    [[nodiscard]] std::uint64_t written() const noexcept;

    [[nodiscard]] std::uint64_t late() const noexcept
    {
        return late_;
    }

    [[nodiscard]] std::uint64_t gaps() const noexcept
    {
        return gaps_;
    }

    [[nodiscard]] EncoderStatus status() const noexcept
    {
        return status_;
    }

private:

    struct Mute {
        std::uint64_t from{0};
        bool muted{false};
    };

    struct Channel {

        std::vector<std::int16_t> pending;

        std::vector<std::uint8_t> covered;

        std::vector<Mute> mutes;

        bool muted{false};
    };

    static void reserve(Channel &channel, std::size_t count);

    [[nodiscard]] static bool mutedAt(const Channel &channel, std::uint64_t position);

    EncoderStatus writeThrough(std::size_t count);

    IAudioEncoder &encoder_;
    std::vector<Channel> channels_;

    const std::uint64_t start_;
    std::uint64_t head_;

    std::uint64_t settled_{0};

    std::uint64_t late_{0};
    std::uint64_t gaps_{0};

    EncoderStatus status_{EncoderStatus::Ok};
};

}
