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

#include "call_view_model.hpp"

#include <softphone/core/call_backend.hpp>
#include <softphone/core/sip_trace.hpp>

#include <QAbstractListModel>
#include <QByteArray>
#include <QHash>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

#include <chrono>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace softphone::ui {

class SipMessageRows : public QAbstractListModel {
    Q_OBJECT

public:
    using QAbstractListModel::QAbstractListModel;

    [[nodiscard]] int rowCount(const QModelIndex &parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    void follow(QVariantList now);

    void refill(QVariantList now);

private:
    QVariantList rows_;
};

class DiagnosticsViewModel : public QObject {
    Q_OBJECT

    QML_ELEMENT
    QML_UNCREATABLE("Created by the application and exposed as `diagnostics`.")

    Q_PROPERTY(bool watching READ watching NOTIFY watchingChanged)

    Q_PROPERTY(QAbstractItemModel *rows READ rows CONSTANT)

    Q_PROPERTY(int shown READ shown NOTIFY messagesChanged)

    Q_PROPERTY(int dropped READ dropped NOTIFY messagesChanged)

    Q_PROPERTY(int total READ total NOTIFY messagesChanged)

    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)

    Q_PROPERTY(QVariantList calls READ calls NOTIFY callsChanged)

    Q_PROPERTY(int droppedCalls READ droppedCalls NOTIFY callsChanged)

    Q_PROPERTY(QVariantList counters READ counters NOTIFY countersChanged)

public:

    using MediaPoller = std::function<void(std::function<void(std::vector<core::CallMedia>)>)>;

    using TraceAttacher = std::function<void(core::ISipTraceSink *)>;

    explicit DiagnosticsViewModel(TraceAttacher attach = {}, MediaPoller poll = {},
                                  QObject *parent = nullptr);
    ~DiagnosticsViewModel() override;

    [[nodiscard]] bool watching() const noexcept
    {
        return watching_;
    }

    Q_INVOKABLE [[nodiscard]] QVariantList messages() const;

    [[nodiscard]] QAbstractItemModel *rows() noexcept
    {
        return &rows_;
    }

    [[nodiscard]] int shown() const
    {
        return rows_.rowCount();
    }

    [[nodiscard]] int dropped() const;
    [[nodiscard]] int total() const;

    [[nodiscard]] QString filter() const
    {
        return filter_;
    }

    [[nodiscard]] QVariantList calls() const;

    [[nodiscard]] int droppedCalls() const noexcept
    {
        return droppedCalls_;
    }

    [[nodiscard]] QVariantList counters() const;

    Q_INVOKABLE [[nodiscard]] QString textOf(qulonglong sequence) const;

    void applyMedia(std::vector<core::CallMedia> media);

public Q_SLOTS:

    void watch(bool wanted);

    void setFilter(const QString &wanted);

    void clear();

    void clearCalls();

Q_SIGNALS:
    void watchingChanged();
    void messagesChanged();
    void filterChanged();
    void callsChanged();
    void countersChanged();

private:
    void refresh();

    void publish(bool afresh = false);

    TraceAttacher attach_;
    MediaPoller poll_;

    core::SipTrace trace_;
    SipMessageRows rows_;
    QTimer timer_;
    bool watching_{false};
    QString filter_;

    struct Tally {
        core::CallMedia latest;

        std::uint64_t carriedSentPackets{0};
        std::uint64_t carriedSentBytes{0};
        std::uint64_t carriedReceivedPackets{0};
        std::uint64_t carriedReceivedBytes{0};

        bool everHadMedia{false};

        std::optional<std::chrono::steady_clock::time_point> goneAt;

        std::chrono::system_clock::time_point startedAt{};
        std::optional<std::chrono::system_clock::time_point> endedAt;

        std::uint64_t seen{0};
    };

    static constexpr std::size_t kMaxCalls = 200;

    std::map<std::string, Tally> tallies_;
    std::uint64_t nextSeen_{1};

    int droppedCalls_{0};
};

}
