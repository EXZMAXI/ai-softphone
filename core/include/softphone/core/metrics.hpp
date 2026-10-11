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

#include <atomic>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace softphone::core {

enum class MetricKind {
    Counter,
    Gauge,
};

[[nodiscard]] std::string_view toString(MetricKind kind) noexcept;

class Counter {
public:
    void add(std::uint64_t by = 1) noexcept
    {
        value_.fetch_add(by, std::memory_order_relaxed);
    }

    [[nodiscard]] std::uint64_t value() const noexcept
    {
        return value_.load(std::memory_order_relaxed);
    }

private:
    std::atomic<std::uint64_t> value_{0};
};

class Gauge {
public:
    void set(double value) noexcept
    {
        value_.store(value, std::memory_order_relaxed);
    }

    [[nodiscard]] double value() const noexcept
    {
        return value_.load(std::memory_order_relaxed);
    }

private:
    std::atomic<double> value_{0.0};
};

struct MetricReading {
    std::string name;
    std::string help;
    MetricKind kind{MetricKind::Counter};
    double value{0.0};
};

class MetricRegistry {
public:
    static MetricRegistry &instance();

    MetricRegistry() = default;
    MetricRegistry(const MetricRegistry &) = delete;
    MetricRegistry &operator=(const MetricRegistry &) = delete;
    MetricRegistry(MetricRegistry &&) = delete;
    MetricRegistry &operator=(MetricRegistry &&) = delete;
    ~MetricRegistry() = default;

    [[nodiscard]] Counter &counter(std::string name, std::string help);
    [[nodiscard]] Gauge &gauge(std::string name, std::string help);

    [[nodiscard]] std::vector<MetricReading> readings() const;

    void reset();

private:
    struct Entry {
        std::string help;
        MetricKind kind{MetricKind::Counter};
        std::unique_ptr<Counter> counter;
        std::unique_ptr<Gauge> gauge;
    };

    mutable std::mutex mutex_;
    std::map<std::string, Entry> entries_;
};

namespace metrics {

[[nodiscard]] Counter &registrationsSucceeded();
[[nodiscard]] Counter &registrationsFailed();
[[nodiscard]] Counter &callsIncoming();
[[nodiscard]] Counter &callsOutgoing();
[[nodiscard]] Counter &callsAnswered();
[[nodiscard]] Counter &callsMissed();
[[nodiscard]] Counter &callsDeclined();

[[nodiscard]] Counter &callsPickedUpElsewhere();
[[nodiscard]] Counter &callsFailed();
[[nodiscard]] Counter &apiRequests();
[[nodiscard]] Counter &apiRequestsRefused();
[[nodiscard]] Counter &webhooksDelivered();
[[nodiscard]] Counter &webhooksFailed();
[[nodiscard]] Counter &webhooksDropped();

void registerEverything();

}

}
