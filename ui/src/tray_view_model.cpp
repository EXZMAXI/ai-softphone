// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "tray_view_model.hpp"

#include "accounts_view_model.hpp"
#include "call_view_model.hpp"
#include "history_view_model.hpp"

namespace softphone::ui {
namespace {

[[nodiscard]] QString iconFor(TrayStatus status, bool missed)
{
    const auto named = [missed](QLatin1StringView ground) {
        return QStringLiteral("qrc:/icons/tray-%1%2.svg")
            .arg(QString{ground}, missed ? QStringLiteral("-missed") : QString{});
    };

    switch (status) {
    case TrayStatus::Offline:
        return named(QLatin1StringView{"offline"});
    case TrayStatus::Degraded:
        return named(QLatin1StringView{"degraded"});
    case TrayStatus::Failing:
        return named(QLatin1StringView{"failing"});
    case TrayStatus::Connected:
        return named(QLatin1StringView{"connected"});
    case TrayStatus::Ringing:
        return named(QLatin1StringView{"ringing"});
    }
    return named(QLatin1StringView{"offline"});
}

[[nodiscard]] TrayViewModel::Status toViewStatus(TrayStatus status) noexcept
{
    switch (status) {
    case TrayStatus::Offline:
        return TrayViewModel::Status::Offline;
    case TrayStatus::Degraded:
        return TrayViewModel::Status::Degraded;
    case TrayStatus::Failing:
        return TrayViewModel::Status::Failing;
    case TrayStatus::Connected:
        return TrayViewModel::Status::Connected;
    case TrayStatus::Ringing:
        return TrayViewModel::Status::Ringing;
    }
    return TrayViewModel::Status::Offline;
}

}

TrayViewModel::TrayViewModel(AccountsViewModel &accounts, CallViewModel &calls,
                             HistoryViewModel &history, QObject *parent)
    : QObject{parent}, accounts_{accounts}, calls_{calls}, history_{history}
{
    connect(&accounts_, &AccountsViewModel::accountsChanged, this, &TrayViewModel::refresh);
    connect(&calls_, &CallViewModel::callChanged, this, &TrayViewModel::refresh);

    connect(&history_, &HistoryViewModel::unseenChanged, this, &TrayViewModel::refresh);

    refresh();
}

void TrayViewModel::refresh()
{
    const auto lines = accounts_.lines();

    auto shown = trayPresentation(lines, calls_.knownCalls(), calls_.recordingsShown(),
                                  history_.unseenMissed());

    if (shown == shown_) {
        return;
    }

    shown_ = std::move(shown);
    Q_EMIT changed();
}

TrayViewModel::Status TrayViewModel::status() const noexcept
{
    return toViewStatus(shown_.status);
}

QString TrayViewModel::icon() const
{
    return iconFor(shown_.status, shown_.missed);
}

QString TrayViewModel::callLabel() const
{
    return QString::fromStdString(shown_.callLabel);
}

QString TrayViewModel::lineLabel() const
{
    return QString::fromStdString(shown_.lineLabel);
}

}
