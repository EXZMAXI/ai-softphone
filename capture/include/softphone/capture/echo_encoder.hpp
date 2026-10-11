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

#include <softphone/capture/echo.hpp>

#include <softphone/core/audio_encoder.hpp>

#include <memory>
#include <vector>

namespace softphone::capture {

class EchoCancellingEncoder final : public core::IAudioEncoder {
public:
    EchoCancellingEncoder(std::unique_ptr<core::IAudioEncoder> below,
                          std::unique_ptr<IEchoCanceller> canceller);

    core::EncoderStatus write(std::span<const std::int16_t> nearSide,
                              std::span<const std::int16_t> farSide) override;

    core::EncoderStatus finish() override;

    [[nodiscard]] std::uint64_t bytes() const noexcept override;

    [[nodiscard]] const char *canceller() const;

private:
    core::EncoderStatus drain(bool everything);

    std::unique_ptr<core::IAudioEncoder> below_;
    std::unique_ptr<IEchoCanceller> canceller_;

    std::vector<std::int16_t> near_;
    std::vector<std::int16_t> far_;

    std::vector<std::int16_t> workNear_;
    std::vector<std::int16_t> workFar_;
};

}
