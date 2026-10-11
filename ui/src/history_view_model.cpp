// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "history_view_model.hpp"

#include "list_edit.hpp"

#include <softphone/core/logging.hpp>
#include <softphone/core/time_text.hpp>

#include <algorithm>
#include <chrono>
#include <ctime>
#include <unordered_set>

namespace softphone::ui {
namespace {

[[nodiscard]] QString text(const std::string &value)
{
    return QString::fromStdString(value);
}

[[nodiscard]] HistoryViewModel::Outcome outcomeOf(const core::CallRecord &record)
{
    switch (core::outcome(record)) {
    case core::CallOutcome::Answered:
        return HistoryViewModel::Outcome::Answered;
    case core::CallOutcome::Missed:
        return HistoryViewModel::Outcome::Missed;
    case core::CallOutcome::Declined:
        return HistoryViewModel::Outcome::Declined;
    case core::CallOutcome::PickedUpElsewhere:
        return HistoryViewModel::Outcome::PickedUpElsewhere;
    case core::CallOutcome::Failed:
        return HistoryViewModel::Outcome::Failed;
    }
    return HistoryViewModel::Outcome::Failed;
}

}

HistoryRows::HistoryRows(HistoryViewModel &owner) : owner_{owner} {}

int HistoryRows::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : known_;
}

QVariant HistoryRows::data(const QModelIndex &index, int role) const
{
    if (role != Qt::UserRole || index.row() < 0 ||
        index.row() >= static_cast<int>(owner_.heldCount())) {
        return {};
    }
    return owner_.rowFor(static_cast<std::size_t>(index.row()));
}

QHash<int, QByteArray> HistoryRows::roleNames() const
{
    return {{Qt::UserRole, QByteArrayLiteral("call")}};
}

void HistoryRows::refilled()
{
    beginResetModel();
    known_ = static_cast<int>(owner_.heldCount());
    endResetModel();
}

void HistoryRows::arrivedAtTheTop(int howMany)
{
    if (howMany <= 0) {
        return;
    }

    beginInsertRows({}, 0, howMany - 1);
    known_ += howMany;
    endInsertRows();
}

void HistoryRows::fellOffTheEnd(int howMany)
{
    if (howMany <= 0) {
        return;
    }

    beginRemoveRows({}, known_ - howMany, known_ - 1);
    known_ -= howMany;
    endRemoveRows();
}

void HistoryRows::wentFromTheMiddle(int at, int howMany)
{
    if (howMany <= 0 || at < 0 || at + howMany > known_) {
        return;
    }

    beginRemoveRows({}, at, at + howMany - 1);
    known_ -= howMany;
    endRemoveRows();
}

void HistoryRows::everyRowSaysSomethingNew()
{
    if (known_ == 0) {
        return;
    }
    Q_EMIT dataChanged(index(0, 0), index(known_ - 1, 0), {Qt::UserRole});
}

void HistoryRows::addedAtTheEnd(int howMany)
{
    if (howMany <= 0) {
        return;
    }
    beginInsertRows({}, known_, known_ + howMany - 1);
    known_ += howMany;
    endInsertRows();
}

HistoryViewModel::HistoryViewModel(Clock clock, QObject *parent)
    : QObject{parent}, clock_{std::move(clock)}, rows_{*this}
{
    if (!clock_) {
        clock_ = []() { return std::chrono::system_clock::now(); };
    }
}

HistoryViewModel::Stamp stampFor(std::chrono::system_clock::time_point when,
                                 std::chrono::system_clock::time_point now)
{

    const auto then = core::localParts(when);
    const auto today = core::localParts(now);

    const auto asDays = [](const std::tm &date) {
        const std::chrono::year_month_day ymd{
            std::chrono::year{date.tm_year + 1900} /
            std::chrono::month{static_cast<unsigned>(date.tm_mon + 1)} /
            std::chrono::day{static_cast<unsigned>(date.tm_mday)}};
        return std::chrono::sys_days{ymd}.time_since_epoch().count();
    };

    const auto elapsed = asDays(today) - asDays(then);

    if (elapsed <= 0) {

        return HistoryViewModel::Stamp::Time;
    }
    if (elapsed == 1) {
        return HistoryViewModel::Stamp::Yesterday;
    }
    if (elapsed < 7) {
        return HistoryViewModel::Stamp::Weekday;
    }
    if (then.tm_year == today.tm_year) {
        return HistoryViewModel::Stamp::DayMonth;
    }
    return HistoryViewModel::Stamp::DayMonthYear;
}

QAbstractItemModel *HistoryViewModel::rows()
{
    return &rows_;
}

int HistoryViewModel::shown() const
{
    return static_cast<int>(calls_.size());
}

std::size_t HistoryViewModel::heldCount() const
{
    return calls_.size();
}

QVariantMap HistoryViewModel::rowFor(std::size_t at) const
{
    return rowFrom(calls_.at(at));
}

QVariantMap HistoryViewModel::rowFrom(const core::CallRecord &record) const
{
    QVariantMap row;
    {
        row.insert(QStringLiteral("id"), text(record.id));

        row.insert(QStringLiteral("seanceId"), text(record.seanceId));

        row.insert(QStringLiteral("startedAtMs"),
                   static_cast<qint64>(std::chrono::duration_cast<std::chrono::milliseconds>(
                                           record.startedAt.time_since_epoch())
                                           .count()));

        row.insert(QStringLiteral("stamp"), static_cast<int>(stampFor(record.startedAt, clock_())));
        row.insert(QStringLiteral("incoming"), record.direction == core::CallDirection::Incoming);
        row.insert(QStringLiteral("outcome"), static_cast<int>(outcomeOf(record)));

        row.insert(QStringLiteral("refused"),
                   record.direction == core::CallDirection::Incoming &&
                       core::outcome(record) == core::CallOutcome::Declined);

        row.insert(QStringLiteral("mark"),
                   static_cast<int>(markFor(core::outcome(record),
                                            record.direction == core::CallDirection::Incoming)));
        row.insert(QStringLiteral("answeredBy"), static_cast<int>(record.autoAnswer));
        row.insert(QStringLiteral("durationSeconds"), static_cast<qint64>(record.duration.count()));

        row.insert(QStringLiteral("attempts"), record.attempts);

        row.insert(QStringLiteral("recordings"), record.recordingCount);

        row.insert(QStringLiteral("callId"), text(record.callId));
        row.insert(QStringLiteral("number"), text(record.number));
        row.insert(QStringLiteral("name"), text(nameFor(record)));

        row.insert(QStringLiteral("known"), inTheAddressBook(record));
        row.insert(QStringLiteral("lineLabel"), text(lineLabel(record)));

        const auto choice =
            core::chooseAccount(accounts_, record.accountId, record.accountAddress, dialAccountId_);

        row.insert(QStringLiteral("lineMissing"),
                   choice.asked && !choice.honoured && !choice.accountId.empty());

    }

    return row;
}

HistoryViewModel::Mark HistoryViewModel::markFor(core::CallOutcome outcome, bool incoming) noexcept
{

    switch (outcome) {
    case core::CallOutcome::Answered:
        return incoming ? Mark::AnsweredHere : Mark::Placed;

    case core::CallOutcome::Missed:
    case core::CallOutcome::Declined:

        return incoming ? Mark::GotAway : Mark::NoConversation;

    case core::CallOutcome::Failed:
    case core::CallOutcome::PickedUpElsewhere:

        return Mark::NoConversation;
    }
    return Mark::NoConversation;
}

void HistoryViewModel::setFilter(Filter filter)
{
    if (filter_ == filter) {
        return;
    }

    filter_ = filter;

    wanted_ = kPageSize;
    paging_ = false;

    Q_EMIT filterChanged();
    Q_EMIT queryChanged();
}

void HistoryViewModel::setSearch(const QString &search)
{

    if (search_ == search) {
        return;
    }

    search_ = search;

    wanted_ = kPageSize;
    paging_ = false;

    Q_EMIT filterChanged();
    Q_EMIT queryChanged();
}

void HistoryViewModel::setAvailable(bool available)
{
    if (available_ == available) {
        return;
    }
    available_ = available;
    Q_EMIT availableChanged();
}

core::CallLogQuery HistoryViewModel::query() const
{
    core::CallLogQuery query;

    query.limit = wanted_ + 1;

    switch (filter_) {
    case Filter::All:
        break;
    case Filter::Missed:
        query.missedOnly = true;
        break;
    case Filter::Declined:
        query.declinedOnly = true;
        break;
    case Filter::Incoming:
        query.direction = core::CallDirection::Incoming;
        break;
    case Filter::Outgoing:
        query.direction = core::CallDirection::Outgoing;
        break;
    }

    query.search = search_.trimmed().toStdString();

    return query;
}

core::CallLogQuery HistoryViewModel::newestQuery() const
{
    core::CallLogQuery query;

    query.limit = kNewestShown * 8;
    return query;
}

core::CallLogQuery HistoryViewModel::nextPageQuery() const
{
    auto next = query();

    next.limit = kPageSize + 1;

    if (!calls_.empty()) {
        const auto &last = calls_.back();
        next.before = core::CallLogCursor{last.startedAt, last.sequence};
    }
    return next;
}

void HistoryViewModel::applyNewest(std::vector<core::CallRecord> calls)
{

    if (newest_ == calls) {
        return;
    }
    newest_ = std::move(calls);
    Q_EMIT newestChanged();
}

void HistoryViewModel::applyCalls(std::vector<core::CallRecord> calls)
{

    paging_ = false;

    more_ = calls.size() > static_cast<std::size_t>(wanted_);
    if (more_) {
        calls.resize(static_cast<std::size_t>(wanted_));
    }

    const auto sameFromThe = [&calls, this](std::size_t offset) {
        const auto overlap = std::min(calls_.size(), calls.size() - offset);
        for (std::size_t at = 0; at < overlap; ++at) {
            if (calls[at + offset].id != calls_[at].id) {
                return false;
            }
        }
        return true;
    };

    auto arrived = std::string::npos;
    if (!calls_.empty()) {
        for (std::size_t at = 0; at < calls.size(); ++at) {
            if (calls[at].id == calls_.front().id) {
                arrived = at;
                break;
            }
        }
    }

    const bool sameList = arrived != std::string::npos && sameFromThe(arrived) &&
                          calls.size() >= arrived && calls.size() - arrived <= calls_.size();

    const auto idsOf = [](const std::vector<core::CallRecord> &records) {
        std::vector<std::string> ids;
        ids.reserve(records.size());
        for (const auto &record : records) {
            ids.push_back(record.id);
        }
        return ids;
    };
    const auto were = sameList ? std::vector<std::string>{} : idsOf(calls_);
    const auto are = sameList ? std::vector<std::string>{} : idsOf(calls);

    const auto wasHeld = calls_.size();
    calls_ = std::move(calls);

    if (!sameList) {

        if (const auto gone = removalBetween(were, are); gone.has_value()) {
            rows_.wentFromTheMiddle(gone->at, gone->howMany);
            rows_.addedAtTheEnd(gone->appended);
        } else {
            rows_.refilled();
        }
    } else {
        rows_.arrivedAtTheTop(static_cast<int>(arrived));

        rows_.fellOffTheEnd(static_cast<int>(wasHeld + arrived - calls_.size()));
        if (arrived == 0 && wasHeld == calls_.size()) {

            rows_.everyRowSaysSomethingNew();
        }
    }

    Q_EMIT callsChanged();
}

void HistoryViewModel::appendCalls(std::vector<core::CallRecord> calls)
{
    paging_ = false;

    more_ = calls.size() > static_cast<std::size_t>(kPageSize);
    if (more_) {
        calls.resize(static_cast<std::size_t>(kPageSize));
    }

    std::unordered_set<std::string> already;
    already.reserve(calls_.size());
    for (const auto &call : calls_) {
        already.insert(call.id);
    }

    const auto before = calls_.size();
    for (auto &call : calls) {
        if (already.contains(call.id)) {
            continue;
        }
        calls_.push_back(std::move(call));
    }

    wanted_ = static_cast<int>(calls_.size());

    rows_.addedAtTheEnd(static_cast<int>(calls_.size() - before));

    Q_EMIT callsChanged();
}

void HistoryViewModel::applyAccounts(std::vector<core::AccountConfig> accounts,
                                     std::string dialAccountId)
{
    accounts_ = std::move(accounts);
    dialAccountId_ = std::move(dialAccountId);
    rows_.everyRowSaysSomethingNew();
    Q_EMIT callsChanged();
}

void HistoryViewModel::setContacts(std::vector<core::Contact> contacts)
{
    contacts_.reset(std::move(contacts));
    rows_.everyRowSaysSomethingNew();
    Q_EMIT callsChanged();
}

void HistoryViewModel::showMore()
{
    if (!more_ || paging_) {
        return;
    }

    paging_ = true;
    Q_EMIT pageRequested();
}

void HistoryViewModel::redial(const QString &id)
{
    const auto *record = find(id);
    if (record == nullptr) {

        core::Logger::instance().log(core::LogLevel::Warning, "ui",
                                     "redial: no row with id " + id.toStdString());
        return;
    }

    const auto target = core::redialTarget(*record);
    if (target.empty()) {
        core::Logger::instance().log(core::LogLevel::Warning, "ui",
                                     "redial: the row carries no number to dial");
        Q_EMIT dialRefused(Dialling::Refusal::NothingToDial);
        return;
    }

    const auto choice =
        core::chooseAccount(accounts_, record->accountId, record->accountAddress, dialAccountId_);
    if (choice.accountId.empty()) {
        core::Logger::instance().log(core::LogLevel::Warning, "ui",
                                     "redial: no account is switched on to call from");
        Q_EMIT dialRefused(Dialling::Refusal::NoLine);
        return;
    }

    Q_EMIT dialRequested(text(target), text(choice.accountId));
}

void HistoryViewModel::refuseDial(Dialling::Refusal reason)
{
    Q_EMIT dialRefused(reason);
}

void HistoryViewModel::missedOne()
{

    if (unseenMissed_ || anybodyWatching()) {
        return;
    }
    unseenMissed_ = true;
    Q_EMIT unseenChanged();
}

bool HistoryViewModel::anybodyWatching() const
{
    for (const auto looking : watching_) {
        if (looking) {
            return true;
        }
    }
    return false;
}

void HistoryViewModel::watching(QObject *who, bool onScreen)
{
    if (who == nullptr) {
        return;
    }

    if (const auto known = watching_.find(who); known != watching_.end()) {
        *known = onScreen;
    } else {
        watching_.insert(who, onScreen);

        connect(who, &QObject::destroyed, this, [this, who]() { watching_.remove(who); });
    }

    if (!onScreen || !unseenMissed_) {
        return;
    }
    unseenMissed_ = false;
    Q_EMIT unseenChanged();
}

void HistoryViewModel::forget(const QString &id)
{
    const auto *record = find(id);
    if (record == nullptr) {
        return;
    }
    Q_EMIT callDropped(text(record->id));
}

void HistoryViewModel::clear()
{
    Q_EMIT clearRequested();
}

QVariantList HistoryViewModel::newest() const
{
    QVariantList out;

    for (std::size_t at = 0; at < newest_.size() && out.size() < kNewestShown;) {
        auto row = rowFrom(newest_.at(at));
        const auto number = row.value(QStringLiteral("number")).toString();
        const auto incoming = row.value(QStringLiteral("incoming")).toBool();
        const auto missed = row.value(QStringLiteral("outcome")).toInt();

        std::size_t next = at + 1;
        while (next < newest_.size()) {
            const auto also = rowFrom(newest_.at(next));
            if (also.value(QStringLiteral("number")).toString() != number ||
                also.value(QStringLiteral("incoming")).toBool() != incoming ||
                also.value(QStringLiteral("outcome")).toInt() != missed) {
                break;
            }
            ++next;
        }

        row.insert(QStringLiteral("times"), static_cast<int>(next - at));
        out.push_back(row);
        at = next;
    }
    return out;
}

QVariantMap HistoryViewModel::contactSeed(const QString &id) const
{
    QVariantMap seed;
    const auto *record = find(id);
    if (record == nullptr) {
        return seed;
    }

    seed.insert(QStringLiteral("number"), text(record->number));

    seed.insert(QStringLiteral("name"), text(record->name));
    return seed;
}

const core::CallRecord *HistoryViewModel::find(const QString &id) const
{
    const auto wanted = id.toStdString();
    const auto named = [&wanted](const core::CallRecord &record) { return record.id == wanted; };

    if (const auto at = std::ranges::find_if(calls_, named); at != calls_.end()) {
        return &*at;
    }
    const auto at = std::ranges::find_if(newest_, named);
    return at == newest_.end() ? nullptr : &*at;
}

int HistoryViewModel::linesWorthNaming() const
{
    return static_cast<int>(
        std::ranges::count_if(accounts_, [](const auto &account) { return account.enabled; }));
}

bool HistoryViewModel::inTheAddressBook(const core::CallRecord &record) const
{

    if (record.direction == core::CallDirection::Outgoing && !record.dialedNumber.empty() &&
        contacts_.find(record.dialedNumber) != nullptr) {
        return true;
    }
    return contacts_.find(record.number) != nullptr;
}

std::string HistoryViewModel::nameFor(const core::CallRecord &record) const
{

    if (record.direction == core::CallDirection::Outgoing && !record.dialedNumber.empty()) {
        if (auto dialled = contacts_.nameFor(record.dialedNumber); !dialled.empty()) {
            return dialled;
        }
    }

    if (auto known = contacts_.nameFor(record.number); !known.empty()) {
        return known;
    }
    return record.name;
}

std::string HistoryViewModel::lineLabel(const core::CallRecord &record) const
{
    if (record.accountId.empty() && record.accountAddress.empty()) {
        return {};
    }

    for (const auto &account : accounts_) {

        if (!account.enabled) {
            continue;
        }

        const bool byId = !record.accountId.empty() && account.uuid == record.accountId;
        const bool byAddress = !record.accountAddress.empty() &&
                               core::accountAddress(account) == record.accountAddress;
        if (byId || byAddress) {

            return linesWorthNaming() < 2 ? std::string{} : core::accountLabel(account);
        }
    }

    return record.accountAddress;
}

}
