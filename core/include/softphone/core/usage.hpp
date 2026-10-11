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

#include <softphone/core/jobs.hpp>
#include <softphone/core/store.hpp>

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace softphone::core {

struct UsageEntry {
    std::string id;
    std::chrono::system_clock::time_point at{};

    std::string providerId;

    std::string providerType;
    std::string model;

    JobKind kind{JobKind::Generate};
    std::string jobId;
    std::string recordingId;

    int promptTokens{0};
    int answerTokens{0};

    std::int64_t costMicros{0};

    std::string currency;
};

struct UsageTotals {
    int calls{0};
    std::int64_t promptTokens{0};
    std::int64_t answerTokens{0};
    std::int64_t costMicros{0};

    std::string currency;
};

class IUsageStore {
public:
    IUsageStore() = default;
    IUsageStore(const IUsageStore &) = delete;
    IUsageStore &operator=(const IUsageStore &) = delete;
    IUsageStore(IUsageStore &&) = delete;
    IUsageStore &operator=(IUsageStore &&) = delete;
    virtual ~IUsageStore() = default;

    [[nodiscard]] virtual StoreStatus recordUsage(UsageEntry &entry) = 0;

    [[nodiscard]] virtual StoreStatus usageSince(std::chrono::system_clock::time_point from,
                                                 UsageTotals &out) = 0;

    [[nodiscard]] virtual StoreStatus usageFor(const std::string &recordingId,
                                               UsageTotals &out) = 0;

    [[nodiscard]] virtual StoreStatus
    forgetUsageBefore(std::chrono::system_clock::time_point when) = 0;
};

}
