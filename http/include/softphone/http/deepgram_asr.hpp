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
#include <softphone/core/http.hpp>

#include <string>
#include <string_view>

namespace softphone::http {

class DeepgramAsrProvider final : public core::IAsrProvider {
public:
    explicit DeepgramAsrProvider(core::IHttpClient &client);

    [[nodiscard]] std::string_view type() const noexcept override
    {
        return "deepgram";
    }

    [[nodiscard]] core::AsrCapabilities capabilities(const core::AsrEndpoint &where) const override;

    [[nodiscard]] core::AsrResult transcribe(const core::AsrEndpoint &where,
                                             const core::AsrInput &input,
                                             const core::AsrOptions &options,
                                             const core::AsrCancel &cancelled) override;

    [[nodiscard]] core::AsrProbe probe(const core::AsrEndpoint &where) override;

private:
    core::IHttpClient &client_;
};

[[nodiscard]] core::AsrResult readDeepgramTranscription(std::string_view body);

}
