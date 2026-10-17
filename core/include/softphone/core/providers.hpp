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

#include <softphone/core/store.hpp>

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace softphone::core {

namespace secretKeys {

[[nodiscard]] inline std::string forProvider(std::string_view id)
{
    return "provider:" + std::string{id};
}

}

enum class ProviderKind {

    Asr,

    Llm,
};

[[nodiscard]] std::string_view toString(ProviderKind kind) noexcept;
[[nodiscard]] std::optional<ProviderKind> parseProviderKind(std::string_view text) noexcept;

struct ProviderConfig {
    std::string id;
    ProviderKind kind{ProviderKind::Asr};

    std::string type;

    std::string name;

    std::string endpoint;
    std::string model;

    std::string region;

    std::vector<std::pair<std::string, std::string>> params;

    std::int64_t inPricePerMillion{0};
    std::int64_t outPricePerMillion{0};

    std::string currency;

    std::chrono::seconds readTimeout{0};

    bool hasSecret{false};

    bool enabled{true};

    bool isDefault{false};

    std::string realtimeEndpoint;

    std::string language;

    std::string realtimeModel;

    std::int64_t realtimePriceMicros{0};

    bool isRealtimeDefault{false};

    std::vector<std::string> modelsSeen;

    std::chrono::system_clock::time_point createdAt{};
    std::chrono::system_clock::time_point updatedAt{};
};

[[nodiscard]] std::string
encodeParams(const std::vector<std::pair<std::string, std::string>> &params);

[[nodiscard]] std::string encodeNames(const std::vector<std::string> &names);

[[nodiscard]] std::vector<std::string> decodeNames(std::string_view text);

[[nodiscard]] std::vector<std::pair<std::string, std::string>> decodeParams(std::string_view text);

class IProviderStore {
public:
    IProviderStore() = default;
    IProviderStore(const IProviderStore &) = delete;
    IProviderStore &operator=(const IProviderStore &) = delete;
    IProviderStore(IProviderStore &&) = delete;
    IProviderStore &operator=(IProviderStore &&) = delete;
    virtual ~IProviderStore() = default;

    [[nodiscard]] virtual StoreStatus save(ProviderConfig &provider) = 0;

    [[nodiscard]] virtual StoreStatus providers(ProviderKind kind,
                                                std::vector<ProviderConfig> &out) = 0;

    [[nodiscard]] virtual StoreStatus provider(const std::string &id, ProviderConfig &out) = 0;

    [[nodiscard]] virtual StoreStatus defaultProvider(ProviderKind kind, ProviderConfig &out) = 0;

    [[nodiscard]] virtual StoreStatus removeProvider(const std::string &id) = 0;
};

}
