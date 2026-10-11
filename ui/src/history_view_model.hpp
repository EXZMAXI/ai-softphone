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

#include "dialling.hpp"

#include <softphone/core/account_config.hpp>
#include <softphone/core/call_log.hpp>
#include <softphone/core/contacts.hpp>

#include <QAbstractListModel>
#include <QHash>
#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace softphone::ui {

class HistoryViewModel;

class HistoryRows : public QAbstractListModel {
    Q_OBJECT

public:
    explicit HistoryRows(HistoryViewModel &owner);

    [[nodiscard]] int rowCount(const QModelIndex &parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    void refilled();

    void arrivedAtTheTop(int howMany);

    void addedAtTheEnd(int howMany);

    void fellOffTheEnd(int howMany);

    void wentFromTheMiddle(int at, int howMany);

    void everyRowSaysSomethingNew();

private:
    HistoryViewModel &owner_;

    int known_{0};
};

class HistoryViewModel : public QObject {
    Q_OBJECT

    QML_ELEMENT
    QML_UNCREATABLE("Created by the application and exposed as `history`.")

    Q_PROPERTY(QAbstractItemModel *rows READ rows CONSTANT)

    Q_PROPERTY(int shown READ shown NOTIFY callsChanged)

    Q_PROPERTY(Filter filter READ filter WRITE setFilter NOTIFY filterChanged)

    Q_PROPERTY(QString search READ search WRITE setSearch NOTIFY filterChanged)

    Q_PROPERTY(bool more READ more NOTIFY callsChanged)

    Q_PROPERTY(bool available READ available NOTIFY availableChanged)

    Q_PROPERTY(bool unseenMissed READ unseenMissed NOTIFY unseenChanged)

    Q_PROPERTY(bool filtered READ filtered NOTIFY filterChanged)

    Q_PROPERTY(QVariantList newest READ newest NOTIFY newestChanged)

    Q_PROPERTY(bool anyNewest READ anyNewest NOTIFY newestChanged)

public:

    enum class Filter { All, Missed, Declined, Incoming, Outgoing };
    Q_ENUM(Filter)

    enum class Outcome { Answered, Missed, Declined, Failed, PickedUpElsewhere };

    enum class Mark {

        AnsweredHere,

        Placed,

        GotAway,

        NoConversation,
    };
    Q_ENUM(Mark)

    [[nodiscard]] static Mark markFor(core::CallOutcome outcome, bool incoming) noexcept;

    enum class Answered { Person, Invitation, AccountSetting, SwitchRequest };
    Q_ENUM(Answered)
    Q_ENUM(Outcome)

    enum class Stamp {

        Time,
        Yesterday,

        Weekday,

        DayMonth,

        DayMonthYear,
    };
    Q_ENUM(Stamp)

    using Clock = std::function<std::chrono::system_clock::time_point()>;

    explicit HistoryViewModel(Clock clock = {}, QObject *parent = nullptr);

    [[nodiscard]] QAbstractItemModel *rows();
    [[nodiscard]] int shown() const;

    [[nodiscard]] QVariantMap rowFor(std::size_t at) const;

    [[nodiscard]] QVariantMap rowFrom(const core::CallRecord &record) const;

    static constexpr int kNewestShown = 15;

    [[nodiscard]] std::size_t heldCount() const;

    [[nodiscard]] Filter filter() const noexcept
    {
        return filter_;
    }

    void setFilter(Filter filter);

    [[nodiscard]] QString search() const
    {
        return search_;
    }

    void setSearch(const QString &search);

    [[nodiscard]] bool more() const noexcept
    {
        return more_;
    }

    [[nodiscard]] bool available() const noexcept
    {
        return available_;
    }

    [[nodiscard]] bool filtered() const noexcept
    {
        return filter_ != Filter::All || !search_.isEmpty();
    }

    void setAvailable(bool available);

    void missedOne();

    Q_INVOKABLE void watching(QObject *who, bool onScreen);

    [[nodiscard]] bool unseenMissed() const noexcept
    {
        return unseenMissed_;
    }

    [[nodiscard]] core::CallLogQuery query() const;

    [[nodiscard]] core::CallLogQuery nextPageQuery() const;

    void applyCalls(std::vector<core::CallRecord> calls);

    void applyNewest(std::vector<core::CallRecord> calls);

    [[nodiscard]] core::CallLogQuery newestQuery() const;

    void appendCalls(std::vector<core::CallRecord> calls);

    void applyAccounts(std::vector<core::AccountConfig> accounts, std::string dialAccountId);

    void setContacts(std::vector<core::Contact> contacts);

    Q_INVOKABLE void showMore();

    Q_INVOKABLE void redial(const QString &id);

    void refuseDial(Dialling::Refusal reason);

    Q_INVOKABLE void forget(const QString &id);

    Q_INVOKABLE void clear();

    Q_INVOKABLE [[nodiscard]] QVariantMap contactSeed(const QString &id) const;

    [[nodiscard]] QVariantList newest() const;

    [[nodiscard]] bool anyNewest() const noexcept
    {
        return !newest_.empty();
    }

Q_SIGNALS:
    void callsChanged();
    void newestChanged();
    void filterChanged();
    void availableChanged();
    void unseenChanged();

    void queryChanged();

    void pageRequested();

    void dialRequested(const QString &number, const QString &accountId);

    void dialRefused(Dialling::Refusal reason);
    void callDropped(const QString &id);
    void clearRequested();

private:
    [[nodiscard]] const core::CallRecord *find(const QString &id) const;
    [[nodiscard]] std::string nameFor(const core::CallRecord &record) const;

    [[nodiscard]] bool inTheAddressBook(const core::CallRecord &record) const;
    [[nodiscard]] std::string lineLabel(const core::CallRecord &record) const;

    [[nodiscard]] int linesWorthNaming() const;

    Clock clock_;
    std::vector<core::CallRecord> calls_;

    std::vector<core::CallRecord> newest_;
    HistoryRows rows_;
    std::vector<core::AccountConfig> accounts_;
    core::ContactBook contacts_;
    std::string dialAccountId_;

    Filter filter_{Filter::All};
    QString search_;

    int wanted_{kPageSize};
    bool more_{false};

    bool paging_{false};
    bool available_{true};
    bool unseenMissed_{false};

    QHash<QObject *, bool> watching_;

    [[nodiscard]] bool anybodyWatching() const;

public:

    static constexpr int kPageSize = 50;
};

[[nodiscard]] HistoryViewModel::Stamp stampFor(std::chrono::system_clock::time_point when,
                                               std::chrono::system_clock::time_point now);

}
