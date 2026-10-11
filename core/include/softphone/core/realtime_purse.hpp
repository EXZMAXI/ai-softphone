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
#include <optional>
#include <string>
#include <string_view>

namespace softphone::core {

struct RealtimeSpendSettings {

    std::int64_t monthlyRecogniserMicros{0};

    std::int64_t monthlyModelMicros{0};

    std::int64_t recogniserMicrosPerMinute{0};

    friend bool operator==(const RealtimeSpendSettings &, const RealtimeSpendSettings &) = default;
};

enum class RealtimeSpend {

    Fine,

    ModelOnly,

    Stop,
};

[[nodiscard]] std::string_view toString(RealtimeSpend verdict) noexcept;

struct RealtimeSpent {
    std::int64_t recogniserMicros{0};
    std::int64_t modelMicros{0};

    std::int64_t recogniserMicrosThisMonth{0};
    std::int64_t modelMicrosThisMonth{0};

    friend bool operator==(const RealtimeSpent &, const RealtimeSpent &) = default;
};

class RealtimePurse {
public:
    explicit RealtimePurse(RealtimeSpendSettings settings = {});

    void settle(RealtimeSpendSettings settings);

    [[nodiscard]] const RealtimeSpendSettings &settings() const noexcept
    {
        return settings_;
    }

    void thisMonth(std::int64_t recogniserMicros, std::int64_t modelMicros);

    void listenedFor(std::chrono::milliseconds howLong);

    void modelSpent(std::int64_t micros);

    [[nodiscard]] const RealtimeSpent &spent() const noexcept
    {
        return spent_;
    }

    [[nodiscard]] RealtimeSpend verdict() const noexcept;

    [[nodiscard]] std::string_view trouble() const noexcept;

    [[nodiscard]] bool pricedRecogniser() const noexcept
    {
        return settings_.recogniserMicrosPerMinute > 0;
    }

private:
    RealtimeSpendSettings settings_;
    RealtimeSpent spent_;

    std::chrono::milliseconds charged_{0};
};

}
