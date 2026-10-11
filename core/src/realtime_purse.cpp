// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/realtime_purse.hpp>

#include <algorithm>

namespace softphone::core {

std::string_view toString(RealtimeSpend verdict) noexcept
{
    switch (verdict) {
    case RealtimeSpend::ModelOnly:
        return "model-only";
    case RealtimeSpend::Stop:
        return "stop";
    case RealtimeSpend::Fine:
        break;
    }
    return "fine";
}

RealtimePurse::RealtimePurse(RealtimeSpendSettings settings) : settings_{settings} {}

void RealtimePurse::settle(RealtimeSpendSettings settings)
{
    settings_ = settings;
}

void RealtimePurse::thisMonth(std::int64_t recogniserMicros, std::int64_t modelMicros)
{
    spent_.recogniserMicrosThisMonth = std::max<std::int64_t>(0, recogniserMicros);
    spent_.modelMicrosThisMonth = std::max<std::int64_t>(0, modelMicros);
}

void RealtimePurse::listenedFor(std::chrono::milliseconds howLong)
{
    if (howLong <= charged_ || settings_.recogniserMicrosPerMinute <= 0) {

        charged_ = std::max(charged_, howLong);
        return;
    }

    const auto newly = howLong - charged_;
    charged_ = howLong;

    spent_.recogniserMicros += settings_.recogniserMicrosPerMinute * newly.count() / 60'000;
}

void RealtimePurse::modelSpent(std::int64_t micros)
{
    spent_.modelMicros += std::max<std::int64_t>(0, micros);
}

RealtimeSpend RealtimePurse::verdict() const noexcept
{

    if (settings_.monthlyRecogniserMicros > 0 &&
        spent_.recogniserMicrosThisMonth + spent_.recogniserMicros >=
            settings_.monthlyRecogniserMicros) {
        return RealtimeSpend::Stop;
    }

    if (settings_.monthlyModelMicros > 0 &&
        spent_.modelMicrosThisMonth + spent_.modelMicros >= settings_.monthlyModelMicros) {
        return RealtimeSpend::ModelOnly;
    }

    return RealtimeSpend::Fine;
}

std::string_view RealtimePurse::trouble() const noexcept
{

    switch (verdict()) {
    case RealtimeSpend::ModelOnly:
        return "model-ceiling";
    case RealtimeSpend::Stop:
        return "recogniser-ceiling";
    case RealtimeSpend::Fine:
        break;
    }
    return {};
}

}
