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

#include <softphone/core/buttons.hpp>

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

#include <chrono>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace softphone::ui {

class ButtonsViewModel : public QObject {
    Q_OBJECT

    QML_ELEMENT
    QML_UNCREATABLE("Created by the application and exposed as `buttons`.")

    Q_PROPERTY(QVariantList buttons READ buttons NOTIFY buttonsChanged)

    Q_PROPERTY(QVariantList panel READ panel NOTIFY buttonsChanged)

    Q_PROPERTY(bool any READ any NOTIFY buttonsChanged)

    Q_PROPERTY(QVariantList lines READ lines NOTIFY linesChanged)

    Q_PROPERTY(QString selectedId READ selectedId NOTIFY selectionChanged)
    Q_PROPERTY(bool editing READ editing NOTIFY selectionChanged)
    Q_PROPERTY(bool addingNew READ addingNew NOTIFY selectionChanged)

    Q_PROPERTY(QString draftTitle READ draftTitle WRITE setDraftTitle NOTIFY editorChanged)
    Q_PROPERTY(QString draftNumber READ draftNumber WRITE setDraftNumber NOTIFY editorChanged)
    Q_PROPERTY(QString draftLine READ draftLine WRITE setDraftLine NOTIFY editorChanged)
    Q_PROPERTY(bool draftWatched READ draftWatched WRITE setDraftWatched NOTIFY editorChanged)

    Q_PROPERTY(bool canWatch READ canWatch NOTIFY editorChanged)

    Q_PROPERTY(bool canSave READ canSave NOTIFY editorChanged)

public:

    enum class Lamp {

        Unknown,
        Idle,
        Ringing,
        Busy,
    };
    Q_ENUM(Lamp)

    enum class Trouble {
        None,
        NoLine,
        Asking,
        NoSuchLine,
        Refused,
        Silent,
    };
    Q_ENUM(Trouble)

    using Clock = std::function<std::chrono::steady_clock::time_point()>;

    explicit ButtonsViewModel(Clock clock = {}, QObject *parent = nullptr);

    [[nodiscard]] QVariantList buttons() const;
    [[nodiscard]] QVariantList panel() const;
    [[nodiscard]] QVariantList lines() const;

    [[nodiscard]] bool any() const;

    void applyButtons(std::vector<core::PhoneButton> buttons);

    void applyAccounts(std::vector<core::AccountConfig> accounts, std::string dialAccountId);

    void applyState(const QString &buttonId, core::LampState lamp);

    void refuseDial(Dialling::Refusal reason);

public Q_SLOTS:

    void dial(const QString &buttonId);

    void select(const QString &buttonId);

    void addButton();

    void save();

    void cancel();

    void removeButton(const QString &buttonId);

    void moveButton(const QString &buttonId, int by);

    [[nodiscard]] QString selectedId() const
    {
        return selectedId_;
    }

    [[nodiscard]] bool editing() const noexcept
    {
        return editing_;
    }

    [[nodiscard]] bool addingNew() const noexcept
    {
        return editing_ && addingNew_;
    }

    [[nodiscard]] QString draftTitle() const;
    [[nodiscard]] QString draftNumber() const;
    [[nodiscard]] QString draftLine() const;
    [[nodiscard]] bool draftWatched() const noexcept;
    [[nodiscard]] bool canWatch() const;
    [[nodiscard]] bool canSave() const;

    void setDraftTitle(const QString &title);
    void setDraftNumber(const QString &number);
    void setDraftLine(const QString &accountId);
    void setDraftWatched(bool watched);

Q_SIGNALS:
    void buttonsChanged();
    void linesChanged();
    void selectionChanged();
    void editorChanged();

    void buttonsEdited(QVariantList buttons);

    void dialRequested(QString target, QString accountId);

    void dialRefused(Dialling::Refusal reason);

private:
    [[nodiscard]] core::PhoneButton *find(const QString &buttonId);
    void announce();

    std::vector<core::PhoneButton> buttons_;
    std::vector<core::AccountConfig> accounts_;
    std::string dialAccountId_;

    Clock clock_;
    std::string dialledId_;
    std::chrono::steady_clock::time_point dialledAt_{};
    std::map<std::string, core::LampState> states_;

    QString selectedId_;
    bool editing_{false};
    bool addingNew_{false};
    core::PhoneButton draft_;
};

}
