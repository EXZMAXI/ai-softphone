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

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace softphone::core {

enum class EncoderStatus {
    Ok,

    Unwritable,

    Failed,

    NoRoom,
};

[[nodiscard]] std::string_view toString(EncoderStatus status) noexcept;

[[nodiscard]] constexpr bool succeeded(EncoderStatus status) noexcept
{
    return status == EncoderStatus::Ok;
}

struct EncoderSettings {

    unsigned sampleRate{16000};

    unsigned channels{2};

    unsigned bitratePerChannel{32};
};

class IAudioEncoder {
public:
    IAudioEncoder() = default;
    IAudioEncoder(const IAudioEncoder &) = delete;
    IAudioEncoder &operator=(const IAudioEncoder &) = delete;
    IAudioEncoder(IAudioEncoder &&) = delete;
    IAudioEncoder &operator=(IAudioEncoder &&) = delete;
    virtual ~IAudioEncoder() = default;

    virtual EncoderStatus write(std::span<const std::int16_t> left,
                                std::span<const std::int16_t> right) = 0;

    virtual EncoderStatus finish() = 0;

    [[nodiscard]] virtual std::uint64_t bytes() const noexcept = 0;
};

}
