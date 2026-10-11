// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/metrics.hpp>

#include <utility>

namespace softphone::core {

std::string_view toString(MetricKind kind) noexcept
{
    switch (kind) {
    case MetricKind::Counter:
        return "counter";
    case MetricKind::Gauge:
        return "gauge";
    }
    return "unknown";
}

MetricRegistry &MetricRegistry::instance()
{
    static MetricRegistry registry;
    return registry;
}

Counter &MetricRegistry::counter(std::string name, std::string help)
{
    const std::lock_guard lock{mutex_};

    auto &entry = entries_[std::move(name)];
    if (!entry.counter) {
        entry.help = std::move(help);
        entry.kind = MetricKind::Counter;
        entry.counter = std::make_unique<Counter>();
    }
    return *entry.counter;
}

Gauge &MetricRegistry::gauge(std::string name, std::string help)
{
    const std::lock_guard lock{mutex_};

    auto &entry = entries_[std::move(name)];
    if (!entry.gauge) {
        entry.help = std::move(help);
        entry.kind = MetricKind::Gauge;
        entry.gauge = std::make_unique<Gauge>();
    }
    return *entry.gauge;
}

std::vector<MetricReading> MetricRegistry::readings() const
{
    const std::lock_guard lock{mutex_};

    std::vector<MetricReading> readings;
    readings.reserve(entries_.size());
    for (const auto &[name, entry] : entries_) {
        readings.push_back(MetricReading{name, entry.help, entry.kind,
                                         entry.counter ? static_cast<double>(entry.counter->value())
                                         : entry.gauge ? entry.gauge->value()
                                                       : 0.0});
    }
    return readings;
}

void MetricRegistry::reset()
{
    const std::lock_guard lock{mutex_};
    entries_.clear();
}

namespace metrics {
namespace {

Counter &counter(const char *name, const char *help)
{
    return MetricRegistry::instance().counter(name, help);
}

}

Counter &registrationsSucceeded()
{
    return counter("registrations_succeeded_total", "SIP registrations that were accepted");
}

Counter &registrationsFailed()
{
    return counter("registrations_failed_total",
                   "SIP registrations that were refused or timed out");
}

Counter &callsIncoming()
{
    return counter("calls_incoming_total", "Calls that arrived at this phone");
}

Counter &callsOutgoing()
{
    return counter("calls_outgoing_total", "Calls this phone placed");
}

Counter &callsAnswered()
{
    return counter("calls_answered_total", "Calls that became a conversation");
}

Counter &callsMissed()
{
    return counter("calls_missed_total", "Incoming calls that rang and were never answered");
}

Counter &callsDeclined()
{
    return counter("calls_declined_total", "Calls refused, by this phone or by the far end");
}

Counter &callsPickedUpElsewhere()
{
    return counter("calls_picked_up_elsewhere_total",
                   "Incoming calls answered on another telephone of the same group");
}

Counter &callsFailed()
{
    return counter("calls_failed_total", "Calls that could not be completed at all");
}

Counter &apiRequests()
{
    return counter("api_requests_total", "Requests the local control API answered");
}

Counter &apiRequestsRefused()
{
    return counter("api_requests_refused_total",
                   "Requests the local control API turned away — a wrong token, a switched-off "
                   "endpoint, a path nobody serves");
}

Counter &webhooksDelivered()
{
    return counter("webhooks_delivered_total", "Webhook deliveries the far end accepted");
}

Counter &webhooksFailed()
{
    return counter("webhooks_failed_total",
                   "Webhook deliveries that were refused or never arrived");
}

Counter &webhooksDropped()
{
    return counter("webhooks_dropped_total",
                   "Webhooks thrown away because the queue was full — an endpoint that stopped "
                   "answering");
}

void registerEverything()
{
    static_cast<void>(registrationsSucceeded());
    static_cast<void>(registrationsFailed());
    static_cast<void>(callsIncoming());
    static_cast<void>(callsOutgoing());
    static_cast<void>(callsAnswered());
    static_cast<void>(callsMissed());
    static_cast<void>(callsDeclined());
    static_cast<void>(callsPickedUpElsewhere());
    static_cast<void>(callsFailed());
    static_cast<void>(apiRequests());
    static_cast<void>(apiRequestsRefused());
    static_cast<void>(webhooksDelivered());
    static_cast<void>(webhooksFailed());
    static_cast<void>(webhooksDropped());
}

}

}
