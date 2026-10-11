// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/webhook_notifier.hpp>

#include <softphone/core/uuid.hpp>

#include <algorithm>
#include <utility>

namespace softphone::core {

WebhookNotifier::WebhookNotifier(IScheduler &scheduler, Sender send, WallClock clock)
    : scheduler_{scheduler}, send_{std::move(send)},
      clock_{clock ? std::move(clock) : defaultWallClock()}
{}

void WebhookNotifier::setSettings(WebhookSettings settings)
{
    settings_ = std::move(settings);
}

void WebhookNotifier::setAddressLookup(AddressLookup lookup)
{
    address_ = std::move(lookup);
}

void WebhookNotifier::setNameLookup(NameLookup lookup)
{
    name_ = std::move(lookup);
}

void WebhookNotifier::setSecretLookup(SecretLookup lookup)
{
    secret_ = std::move(lookup);
}

void WebhookNotifier::observe(const CallInfo &call)
{
    if (call.id.empty()) {
        return;
    }

    auto [entry, inserted] = seen_.try_emplace(call.id, Sighting{clock_(), call, false});
    if (inserted && seen_.size() > kRememberedCalls) {
        forgetTheOldest();
    }

    if (inserted && call.state == CallState::Ended) {
        entry->second.standing = call;
        entry->second.ended = true;
        raise(call, webhookEvent::kEnded, entry->second.startedAt, std::chrono::seconds{0});
        return;
    }

    if (inserted) {

        raise(call, webhookEvent::kStarted, entry->second.startedAt, std::chrono::seconds{0});
        entry->second.standing = call;
        return;
    }

    if (call.state == CallState::Ended) {

        if (entry->second.ended) {
            return;
        }
        entry->second.ended = true;

        auto lasted = std::chrono::seconds{0};
        if (call.connectedAt.has_value()) {
            lasted =
                std::max(std::chrono::seconds{0}, std::chrono::duration_cast<std::chrono::seconds>(
                                                      scheduler_.now() - *call.connectedAt));
        }
        raise(call, webhookEvent::kEnded, clock_(), lasted);
        return;
    }

    if (sameStanding(entry->second.standing, call)) {

        return;
    }

    entry->second.standing = call;
    raise(call, webhookEvent::kStateChanged, clock_(), std::chrono::seconds{0});
}

void WebhookNotifier::raise(const CallInfo &call, WebhookEvent event,
                            std::chrono::system_clock::time_point at, std::chrono::seconds duration)
{
    if (!send_ || !settings_.enabled || !settings_.wants(event)) {
        return;
    }

    WebhookContext context;
    context.at = at;
    context.duration = duration;
    if (address_) {
        const auto line = address_(call.accountId);
        context.accountId = line.uuid;
        context.accountAddress = line.address;
    }
    if (name_) {
        context.name = name_(call.remoteUri.empty() ? call.remoteDisplayName : call.remoteUri);
    }

    const auto fields = webhookFields(call, event, context);
    if (auto request = webhookFor(settings_, fields, event, secret_ ? secret_() : std::string{})) {
        send_(std::move(*request));
    }
}

void WebhookNotifier::sendTestEvent()
{
    if (!send_ || !settings_.enabled) {
        return;
    }

    CallInfo call;
    call.id = "test-event";

    call.uuid = makeUuid();
    call.direction = CallDirection::Incoming;
    call.remoteUri = "sip:0000@example.invalid";
    call.remoteDisplayName = "Test";
    call.state = CallState::RingingIn;

    WebhookContext context;
    context.at = clock_();
    context.name = "Test";

    const auto fields = webhookFields(call, webhookEvent::kStarted, context);

    auto settings = settings_;
    std::erase(settings.silenced, webhookEvent::kStarted);

    if (auto request = webhookFor(settings, fields, webhookEvent::kStarted,
                                  secret_ ? secret_() : std::string{})) {
        send_(std::move(*request));
    }
}

void WebhookNotifier::forgetTheOldest()
{
    const auto oldest = std::ranges::min_element(
        seen_, {}, [](const auto &entry) { return entry.second.startedAt; });
    if (oldest != seen_.end()) {
        seen_.erase(oldest);
    }
}

}
