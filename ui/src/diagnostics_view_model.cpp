// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "diagnostics_view_model.hpp"

#include <softphone/core/metrics.hpp>

#include <QVariantMap>

#include <algorithm>
#include <array>
#include <chrono>
#include <utility>

namespace softphone::ui {
namespace {

constexpr auto kTick = std::chrono::milliseconds{500};

[[nodiscard]] QString text(const std::string &value)
{
    return QString::fromStdString(value);
}

[[nodiscard]] qint64 asMilliseconds(std::chrono::system_clock::time_point when)
{
    return static_cast<qint64>(
        std::chrono::duration_cast<std::chrono::milliseconds>(when.time_since_epoch()).count());
}

[[nodiscard]] double asMilliseconds(unsigned microseconds)
{
    return static_cast<double>(microseconds) / 1000.0;
}

}

int SipMessageRows::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(rows_.size());
}

QVariant SipMessageRows::data(const QModelIndex &index, int role) const
{
    if (role != Qt::UserRole || index.row() < 0 || index.row() >= rows_.size()) {
        return {};
    }
    return rows_.at(index.row());
}

QHash<int, QByteArray> SipMessageRows::roleNames() const
{
    return {{Qt::UserRole, QByteArrayLiteral("message")}};
}

void SipMessageRows::follow(QVariantList now)
{
    const auto sequenceIn = [](const QVariantList &list, qsizetype at) {
        return list.at(at).toMap().value(QStringLiteral("sequence")).toULongLong();
    };

    qsizetype gone = 0;
    if (now.isEmpty()) {
        gone = rows_.size();
    } else {
        const auto oldest = sequenceIn(now, 0);
        while (gone < rows_.size() && sequenceIn(rows_, gone) < oldest) {
            ++gone;
        }
    }

    const auto kept = rows_.size() - gone;
    bool continues = kept <= now.size();
    for (qsizetype at = 0; continues && at < kept; ++at) {
        continues = sequenceIn(rows_, gone + at) == sequenceIn(now, at);
    }
    if (!continues) {
        beginResetModel();
        rows_ = std::move(now);
        endResetModel();
        return;
    }

    if (gone > 0) {
        beginRemoveRows({}, 0, static_cast<int>(gone - 1));
        rows_.erase(rows_.begin(), rows_.begin() + gone);
        endRemoveRows();
    }
    if (now.size() > kept) {
        beginInsertRows({}, static_cast<int>(kept), static_cast<int>(now.size() - 1));
        for (qsizetype at = kept; at < now.size(); ++at) {
            rows_.append(now.at(at));
        }
        endInsertRows();
    }
}

void SipMessageRows::refill(QVariantList now)
{
    beginResetModel();
    rows_ = std::move(now);
    endResetModel();
}

DiagnosticsViewModel::DiagnosticsViewModel(TraceAttacher attach, MediaPoller poll, QObject *parent)
    : QObject{parent}, attach_{std::move(attach)}, poll_{std::move(poll)}
{
    timer_.setInterval(kTick);
    QObject::connect(&timer_, &QTimer::timeout, this, [this]() { refresh(); });
}

DiagnosticsViewModel::~DiagnosticsViewModel()
{

    if (attach_) {
        attach_(nullptr);
    }
}

void DiagnosticsViewModel::watch(bool wanted)
{
    if (watching_ == wanted) {
        return;
    }

    watching_ = wanted;

    if (watching_) {
        if (attach_) {
            attach_(&trace_);
        }
        timer_.start();
    } else {
        if (attach_) {
            attach_(nullptr);
        }
        timer_.stop();

        trace_.clear();
        tallies_.clear();
        droppedCalls_ = 0;
        publish();
        Q_EMIT callsChanged();
    }

    Q_EMIT watchingChanged();

    if (watching_) {
        refresh();
    }
}

void DiagnosticsViewModel::refresh()
{
    publish();
    Q_EMIT countersChanged();

    if (poll_) {
        poll_([this](std::vector<core::CallMedia> media) { applyMedia(std::move(media)); });
    }
}

void DiagnosticsViewModel::applyMedia(std::vector<core::CallMedia> media)
{
    const auto now = std::chrono::steady_clock::now();

    for (const auto &call : media) {
        auto &tally = tallies_[call.callId];
        if (tally.seen == 0) {
            tally.seen = nextSeen_++;
            tally.startedAt = std::chrono::system_clock::now();
        }
        tally.goneAt.reset();
        tally.endedAt.reset();

        if (!call.hasMedia) {

            const auto stats = tally.latest.stats;
            tally.latest = call;
            tally.latest.stats = stats;
            continue;
        }

        if (tally.everHadMedia && core::streamRestarted(tally.latest.stats, call.stats)) {
            tally.carriedSentPackets += tally.latest.stats.sent.packets;
            tally.carriedSentBytes += tally.latest.stats.sent.bytes;
            tally.carriedReceivedPackets += tally.latest.stats.received.packets;
            tally.carriedReceivedBytes += tally.latest.stats.received.bytes;
        }

        tally.latest = call;
        tally.everHadMedia = true;
    }

    for (auto &[id, tally] : tallies_) {

        const auto present = std::ranges::any_of(
            media, [&wanted = id](const auto &call) { return call.callId == wanted; });
        if (!present && !tally.goneAt.has_value()) {
            tally.goneAt = now;
            tally.endedAt = std::chrono::system_clock::now();
            tally.latest.hasMedia = false;

            tally.latest.state = core::CallState::Ended;
        }
    }

    while (tallies_.size() > kMaxCalls) {
        const auto oldest =
            std::ranges::min_element(tallies_, [](const auto &left, const auto &right) {

                const auto rank = [](const auto &entry) {
                    return std::pair{!entry.second.goneAt.has_value(), entry.second.seen};
                };
                return rank(left) < rank(right);
            });

        if (oldest == tallies_.end() || !oldest->second.goneAt.has_value()) {
            break;
        }

        tallies_.erase(oldest);
        ++droppedCalls_;
    }

    Q_EMIT callsChanged();
}

void DiagnosticsViewModel::setFilter(const QString &wanted)
{
    if (filter_ == wanted) {
        return;
    }
    filter_ = wanted;
    Q_EMIT filterChanged();
    publish(true);
}

void DiagnosticsViewModel::clear()
{
    trace_.clear();
    publish();
}

void DiagnosticsViewModel::clearCalls()
{

    std::erase_if(tallies_, [](const auto &entry) { return entry.second.goneAt.has_value(); });
    droppedCalls_ = 0;

    Q_EMIT callsChanged();
}

QVariantList DiagnosticsViewModel::messages() const
{
    QVariantList rows;
    for (const auto &message : trace_.messages()) {
        if (!filter_.isEmpty()) {

            const auto haystack = text(message.summary) + ' ' + text(message.callId) + ' ' +
                                  text(message.remoteAddress) + ' ' + text(message.localAddress);
            if (!haystack.contains(filter_, Qt::CaseInsensitive)) {
                continue;
            }
        }

        QVariantMap row;
        row.insert(QStringLiteral("sequence"), static_cast<qulonglong>(message.sequence));
        row.insert(QStringLiteral("atMs"), asMilliseconds(message.at));
        row.insert(QStringLiteral("incoming"), message.direction == core::SipDirection::Incoming);
        row.insert(QStringLiteral("summary"), text(message.summary));
        row.insert(QStringLiteral("transport"), text(message.transport));
        row.insert(QStringLiteral("localAddress"), text(message.localAddress));
        row.insert(QStringLiteral("remoteAddress"), text(message.remoteAddress));
        row.insert(QStringLiteral("callId"), text(message.callId));
        rows.append(row);
    }
    return rows;
}

void DiagnosticsViewModel::publish(bool afresh)
{
    if (afresh) {
        rows_.refill(messages());
    } else {
        rows_.follow(messages());
    }
    Q_EMIT messagesChanged();
}

int DiagnosticsViewModel::dropped() const
{
    return static_cast<int>(trace_.dropped());
}

int DiagnosticsViewModel::total() const
{
    return static_cast<int>(trace_.size());
}

QString DiagnosticsViewModel::textOf(qulonglong sequence) const
{
    for (const auto &message : trace_.messages()) {
        if (message.sequence == sequence) {
            return text(message.text);
        }
    }

    return {};
}

QVariantList DiagnosticsViewModel::calls() const
{

    std::vector<const Tally *> ordered;
    ordered.reserve(tallies_.size());
    for (const auto &entry : tallies_) {
        ordered.push_back(&entry.second);
    }
    std::ranges::sort(ordered, {}, [](const Tally *tally) { return tally->seen; });

    QVariantList rows;
    for (const auto *entry : ordered) {
        const auto &tally = *entry;
        const auto &call = tally.latest;
        const auto &stats = tally.latest.stats;

        QVariantMap row;
        row.insert(QStringLiteral("callId"), text(call.callId));

        row.insert(QStringLiteral("number"), text(call.number));
        row.insert(QStringLiteral("name"), text(call.name));
        row.insert(QStringLiteral("state"), static_cast<int>(call.state));
        row.insert(QStringLiteral("incoming"), call.direction == core::CallDirection::Incoming);
        row.insert(QStringLiteral("durationSeconds"), static_cast<qint64>(call.duration.count()));

        row.insert(QStringLiteral("atMs"), asMilliseconds(tally.startedAt));
        row.insert(QStringLiteral("endedAtMs"),
                   tally.endedAt.has_value() ? asMilliseconds(*tally.endedAt) : qint64{0});

        row.insert(QStringLiteral("live"), call.hasMedia);
        row.insert(QStringLiteral("everHadMedia"), tally.everHadMedia);

        row.insert(QStringLiteral("codec"), text(stats.codec));
        row.insert(QStringLiteral("clockRate"), static_cast<int>(stats.clockRate));
        row.insert(QStringLiteral("encrypted"), stats.encrypted);
        row.insert(QStringLiteral("sentPayload"), static_cast<int>(stats.sentPayloadType));
        row.insert(QStringLiteral("receivedPayload"), static_cast<int>(stats.receivedPayloadType));

        row.insert(QStringLiteral("payloadsDiffer"),
                   tally.everHadMedia && stats.sentPayloadType != stats.receivedPayloadType);

        row.insert(QStringLiteral("localAddress"), text(stats.localAddress));
        row.insert(QStringLiteral("remoteAddress"), text(stats.remoteAddress));

        row.insert(QStringLiteral("sentPackets"), static_cast<qulonglong>(stats.sent.packets));
        row.insert(QStringLiteral("sentBytes"), static_cast<qulonglong>(stats.sent.bytes));
        row.insert(QStringLiteral("sentLost"), static_cast<qulonglong>(stats.sent.lost));
        row.insert(QStringLiteral("sentJitterMs"), asMilliseconds(stats.sent.jitterMeanUsec));

        row.insert(QStringLiteral("receivedPackets"),
                   static_cast<qulonglong>(stats.received.packets));
        row.insert(QStringLiteral("receivedBytes"), static_cast<qulonglong>(stats.received.bytes));
        row.insert(QStringLiteral("receivedLost"), static_cast<qulonglong>(stats.received.lost));
        row.insert(QStringLiteral("receivedDiscarded"),
                   static_cast<qulonglong>(stats.received.discarded));
        row.insert(QStringLiteral("receivedJitterMs"),
                   asMilliseconds(stats.received.jitterMeanUsec));

        row.insert(QStringLiteral("roundTripMs"), asMilliseconds(stats.roundTripMeanUsec));

        row.insert(QStringLiteral("sentPacketsTotal"),
                   static_cast<qulonglong>(tally.carriedSentPackets + stats.sent.packets));
        row.insert(QStringLiteral("sentBytesTotal"),
                   static_cast<qulonglong>(tally.carriedSentBytes + stats.sent.bytes));
        row.insert(QStringLiteral("receivedPacketsTotal"),
                   static_cast<qulonglong>(tally.carriedReceivedPackets + stats.received.packets));
        row.insert(QStringLiteral("receivedBytesTotal"),
                   static_cast<qulonglong>(tally.carriedReceivedBytes + stats.received.bytes));

        rows.append(row);
    }
    return rows;
}

QVariantList DiagnosticsViewModel::counters() const
{

    static constexpr std::array kOrder{
        std::string_view{"registrations_succeeded_total"},
        std::string_view{"registrations_failed_total"},
        std::string_view{"calls_incoming_total"},
        std::string_view{"calls_outgoing_total"},
        std::string_view{"calls_answered_total"},
        std::string_view{"calls_missed_total"},
        std::string_view{"calls_declined_total"},
        std::string_view{"calls_picked_up_elsewhere_total"},
        std::string_view{"calls_failed_total"},
        std::string_view{"api_requests_total"},
        std::string_view{"api_requests_refused_total"},
        std::string_view{"webhooks_delivered_total"},
        std::string_view{"webhooks_failed_total"},
        std::string_view{"webhooks_dropped_total"},
    };

    auto readings = core::MetricRegistry::instance().readings();
    std::ranges::stable_sort(readings, {}, [](const core::MetricReading &reading) {
        const auto at = std::ranges::find(kOrder, reading.name);
        return at == kOrder.end() ? kOrder.size() : static_cast<std::size_t>(at - kOrder.begin());
    });

    QVariantList rows;
    for (const auto &reading : readings) {
        QVariantMap row;
        row.insert(QStringLiteral("name"), text(reading.name));

        row.insert(QStringLiteral("help"), text(reading.help));
        row.insert(QStringLiteral("value"), reading.value);
        row.insert(QStringLiteral("counter"), reading.kind == core::MetricKind::Counter);
        rows.append(row);
    }
    return rows;
}

}
