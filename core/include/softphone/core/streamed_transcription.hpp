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

#include <softphone/core/asr.hpp>
#include <softphone/core/streaming_asr.hpp>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace softphone::core {

using PcmReader = std::function<bool(const std::filesystem::path &file, unsigned rate,
                                     std::vector<std::int16_t> &mono)>;

inline constexpr unsigned kStreamedFasterThanLife = 8;

class StreamedTranscription final : public IAsrProvider {
public:

    using Opener = std::function<std::unique_ptr<IStreamingAsr>()>;

    StreamedTranscription(std::string type, Opener opener, PcmReader reader);

    [[nodiscard]] std::string_view type() const noexcept override
    {
        return type_;
    }

    [[nodiscard]] AsrCapabilities capabilities(const AsrEndpoint &where) const override;

    [[nodiscard]] AsrResult transcribe(const AsrEndpoint &where, const AsrInput &input,
                                       const AsrOptions &options,
                                       const AsrCancel &cancelled) override;

    [[nodiscard]] AsrProbe probe(const AsrEndpoint &where) override;

private:
    std::string type_;
    Opener opener_;
    PcmReader reader_;
};

}
