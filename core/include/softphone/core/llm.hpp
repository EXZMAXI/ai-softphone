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
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace softphone::core {

enum class LlmRole {

    System,

    User,

    Assistant,
};

[[nodiscard]] std::string_view toString(LlmRole role) noexcept;
[[nodiscard]] std::optional<LlmRole> parseLlmRole(std::string_view text) noexcept;

struct LlmMessage {
    LlmRole role{LlmRole::User};
    std::string text;
};

struct LlmCapabilities {

    int contextTokens{8192};

    int maxOutputTokens{4096};

    bool jsonMode{false};

    bool prefill{false};

    int maxConcurrent{2};
};

struct LlmEndpoint {
    std::string url;
    std::string model;
    std::string region;

    std::vector<std::pair<std::string, std::string>> params;

    std::string token;

    std::chrono::seconds timeout{120};
};

struct LlmOptions {

    int maxOutputTokens{0};

    bool wantsJson{false};

    std::string prefill;
};

struct LlmUsage {
    int promptTokens{0};
    int answerTokens{0};

    [[nodiscard]] int total() const noexcept
    {
        return promptTokens + answerTokens;
    }
};

[[nodiscard]] std::int64_t costMicros(const LlmUsage &usage, std::int64_t inPricePerMillion,
                                      std::int64_t outPricePerMillion) noexcept;

enum class LlmStatus {
    Ok,

    Unauthorised,

    RateLimited,

    TooLarge,

    Unreachable,

    Refused,

    Unsupported,

    Malformed,
    Cancelled,
    Failed,
};

[[nodiscard]] std::string_view toString(LlmStatus status) noexcept;

[[nodiscard]] bool worthRetrying(LlmStatus status) noexcept;

struct LlmAnswer {
    LlmStatus status{LlmStatus::Failed};

    std::string text;

    std::string detail;

    LlmUsage usage;

    std::string model;

    std::string raw;

    std::string request;

    std::vector<std::pair<std::string, std::string>> learnt;
};

struct LlmProbe {
    bool reachable{false};
    LlmStatus status{LlmStatus::Failed};
    std::string detail;

    std::vector<std::string> models;

    std::vector<std::pair<std::string, std::string>> learnt;
};

[[nodiscard]] int estimateTokens(std::string_view text) noexcept;
[[nodiscard]] int estimateTokens(const std::vector<LlmMessage> &messages) noexcept;

[[nodiscard]] std::string firstJsonValue(std::string_view answer);

using LlmCancel = std::function<bool()>;

class ILlmProvider {
public:
    ILlmProvider() = default;
    ILlmProvider(const ILlmProvider &) = delete;
    ILlmProvider &operator=(const ILlmProvider &) = delete;
    ILlmProvider(ILlmProvider &&) = delete;
    ILlmProvider &operator=(ILlmProvider &&) = delete;
    virtual ~ILlmProvider() = default;

    [[nodiscard]] virtual LlmCapabilities capabilities(const LlmEndpoint &where) const = 0;

    [[nodiscard]] virtual LlmAnswer ask(const LlmEndpoint &where,
                                        const std::vector<LlmMessage> &messages,
                                        const LlmOptions &options, const LlmCancel &cancelled) = 0;

    [[nodiscard]] virtual LlmProbe probe(const LlmEndpoint &where) = 0;
};

}
