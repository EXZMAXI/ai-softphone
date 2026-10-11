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

#include <softphone/core/http.hpp>
#include <softphone/core/llm.hpp>

namespace softphone::http {

class AnthropicChatProvider final : public core::ILlmProvider {
public:
    explicit AnthropicChatProvider(core::IHttpClient &client);

    [[nodiscard]] core::LlmCapabilities capabilities(const core::LlmEndpoint &where) const override;

    [[nodiscard]] core::LlmAnswer ask(const core::LlmEndpoint &where,
                                      const std::vector<core::LlmMessage> &messages,
                                      const core::LlmOptions &options,
                                      const core::LlmCancel &cancelled) override;

    [[nodiscard]] core::LlmProbe probe(const core::LlmEndpoint &where) override;

private:

    [[nodiscard]] core::LlmAnswer askOnce(const core::LlmEndpoint &where,
                                          const std::vector<core::LlmMessage> &messages,
                                          const core::LlmOptions &options,
                                          const core::LlmCancel &cancelled);

    core::IHttpClient &client_;
};

}
