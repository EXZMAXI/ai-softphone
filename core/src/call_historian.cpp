// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/call_historian.hpp>

#include <softphone/core/logging.hpp>
#include <softphone/core/metrics.hpp>

#include <algorithm>
#include <utility>

namespace softphone::core {

CallHistorian::CallHistorian(ICallLog &log, IScheduler &scheduler, WallClock clock)
    : log_{log}, scheduler_{scheduler}, clock_{std::move(clock)}
{
    if (!clock_) {
        clock_ = defaultWallClock();
    }
}

void CallHistorian::setAddressLookup(AddressLookup lookup)
{
    address_ = std::move(lookup);
}

void CallHistorian::setNameLookup(NameLookup lookup)
{
    name_ = std::move(lookup);
}

void CallHistorian::setOnRecorded(Recorded told)
{
    recorded_ = std::move(told);
}

void CallHistorian::observe(const CallInfo &call)
{
    if (call.id.empty()) {
        return;
    }

    auto [entry, inserted] = seen_.try_emplace(
        call.id, Sighting{.startedAt = clock_(), .written = false, .recordId = {}});
    if (inserted && seen_.size() > kRememberedCalls) {
        forgetTheOldest();
    }

    if (call.state != CallState::Ended || entry->second.written) {
        return;
    }

    CallRecordContext context;
    context.startedAt = entry->second.startedAt;
    context.endedAt = scheduler_.now();

    if (address_) {
        const auto line = address_(call.accountId);
        context.accountId = line.uuid;
        context.accountAddress = line.address;
    }

    const auto ourNumber = sipUserPart(context.accountAddress);

    const LogAttribute fields[] = {{"call", call.id}, {"line", call.accountId}};
    Logger::instance().log(LogLevel::Debug, "history",
                           "ended: reason=" + std::string{toString(call.end.reason)} +
                               " askedByThem=" + (call.transfer.askedByThem ? "yes" : "no") +
                               " successor=" + call.transfer.otherCallId +
                               " farEnd=" + sipUserPart(call.remoteUri) + " number=" + ourNumber,
                           fields);

    if (continuedElsewhere(call, scheduler_.now(), ourNumber)) {
        entry->second.written = true;
        return;
    }
    if (name_) {

        context.name = name_(call.remoteUri.empty() ? call.remoteDisplayName : call.remoteUri);
    }

    auto record = callRecord(call, context);

    (record.direction == CallDirection::Incoming ? metrics::callsIncoming()
                                                 : metrics::callsOutgoing())
        .add();
    if (recorded_) {
        recorded_(record);
    }

    switch (outcome(record)) {
    case CallOutcome::Answered:
        metrics::callsAnswered().add();
        break;
    case CallOutcome::Missed:
        metrics::callsMissed().add();
        break;
    case CallOutcome::Declined:
        metrics::callsDeclined().add();
        break;
    case CallOutcome::Failed:
        metrics::callsFailed().add();
        break;
    case CallOutcome::PickedUpElsewhere:
        metrics::callsPickedUpElsewhere().add();
        break;
    }

    if (call.redialAttempt == 1 && !call.redialContinues.empty()) {
        const auto grewOutOf = seen_.find(call.redialContinues);
        if (grewOutOf != seen_.end() && !grewOutOf->second.recordId.empty()) {
            campaign_.recordId = grewOutOf->second.recordId;
            campaign_.startedAt = grewOutOf->second.startedAt;

            campaign_.attempt = 0;
            campaign_.grewOutOfACall = true;
        }
    }

    const bool continuesCampaign = call.redialAttempt > 0 && !campaign_.recordId.empty() &&
                                   call.redialAttempt > campaign_.attempt;

    if (call.redialAttempt > 0) {
        record.attempts = call.redialAttempt + (campaign_.grewOutOfACall ? 1 : 0);
        if (continuesCampaign) {
            record.startedAt = campaign_.startedAt;
        }
    }

    const auto replaced = continuesCampaign ? campaign_.recordId : std::string{};

    lastStatus_ = log_.record(record);

    entry->second.recordId = record.id;

    if (call.redialAttempt > 0 && lastStatus_ == StoreStatus::Ok) {

        if (!replaced.empty()) {
            static_cast<void>(log_.removeCall(replaced));
        }

        campaign_.recordId = record.id;
        campaign_.attempt = call.redialAttempt;
        if (!continuesCampaign) {
            campaign_.startedAt = record.startedAt;
        }
    } else if (call.redialAttempt == 0) {

        campaign_ = Campaign{};
    }

    entry->second.written = true;
}

void CallHistorian::forgetTheOldest()
{
    const auto oldest =
        std::min_element(seen_.begin(), seen_.end(), [](const auto &left, const auto &right) {

            if (left.second.written != right.second.written) {
                return left.second.written;
            }
            return left.second.startedAt < right.second.startedAt;
        });
    if (oldest != seen_.end()) {
        seen_.erase(oldest);
    }
}

}
