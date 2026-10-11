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
#include <softphone/core/contacts.hpp>

#include <QAbstractListModel>
#include <QCollator>
#include <QHash>
#include <QLocale>
#include <QObject>
#include <QString>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace softphone::ui {

class ContactsViewModel;

struct ContactEntry {
    core::Contact contact;

    QString name;
    QString number;
};

class ContactRows : public QAbstractListModel {
    Q_OBJECT

public:
    explicit ContactRows(ContactsViewModel &owner);

    [[nodiscard]] int rowCount(const QModelIndex &parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    void refill();

private:
    ContactsViewModel &owner_;

    std::vector<std::size_t> shown_;
};

class ContactsViewModel : public QObject {
    Q_OBJECT

    QML_ELEMENT
    QML_UNCREATABLE("Created by the application and exposed as `contacts`.")

    Q_PROPERTY(QAbstractItemModel *rows READ rows CONSTANT)

    Q_PROPERTY(int shown READ shown NOTIFY contactsChanged)

    Q_PROPERTY(int total READ total NOTIFY contactsChanged)

    Q_PROPERTY(QString search READ search WRITE setSearch NOTIFY searchChanged)

    Q_PROPERTY(bool available READ available NOTIFY availableChanged)

    Q_PROPERTY(bool editing READ editing NOTIFY editorChanged)

    Q_PROPERTY(QString editingId READ editingId NOTIFY editorChanged)

    Q_PROPERTY(QString name READ name WRITE setName NOTIFY editorChanged)
    Q_PROPERTY(QString number READ number WRITE setNumber NOTIFY editorChanged)

    Q_PROPERTY(QString accountId READ accountId WRITE setAccountId NOTIFY editorChanged)

    Q_PROPERTY(bool canSave READ canSave NOTIFY editorChanged)
    Q_PROPERTY(Fault fault READ fault NOTIFY editorChanged)

    Q_PROPERTY(QVariantList lines READ lines NOTIFY linesChanged)

public:

    enum class Fault { None, MissingName, MissingNumber };
    Q_ENUM(Fault)

    explicit ContactsViewModel(QObject *parent = nullptr);

    [[nodiscard]] QAbstractItemModel *rows();
    [[nodiscard]] int shown() const;
    [[nodiscard]] int total() const;

    [[nodiscard]] QVariantMap rowFor(std::size_t at) const;

    [[nodiscard]] const std::vector<ContactEntry> &book() const;

    [[nodiscard]] bool matches(const ContactEntry &entry) const;
    [[nodiscard]] QString search() const;
    void setSearch(const QString &text);

    [[nodiscard]] bool available() const noexcept
    {
        return available_;
    }

    [[nodiscard]] bool editing() const noexcept
    {
        return editing_;
    }

    [[nodiscard]] QString editingId() const;
    [[nodiscard]] QString name() const;
    void setName(const QString &value);
    [[nodiscard]] QString number() const;
    void setNumber(const QString &value);
    [[nodiscard]] QString accountId() const;
    void setAccountId(const QString &value);
    [[nodiscard]] bool canSave() const;
    [[nodiscard]] Fault fault() const;
    [[nodiscard]] QVariantList lines() const;

    void applyContacts(std::vector<core::Contact> contacts);

    void applyAccounts(std::vector<core::AccountConfig> accounts, std::string dialAccountId);

    void setAvailable(bool available);

    void setLocale(const QLocale &locale);

    Q_INVOKABLE void addContact(const QString &number = {}, const QString &name = {});

    Q_INVOKABLE void edit(const QString &id);

    Q_INVOKABLE void cancelEdit();

    Q_INVOKABLE void save();

    Q_INVOKABLE void removeContact(const QString &id);

    Q_INVOKABLE void dial(const QString &id);

    void refuseDial(Dialling::Refusal reason);

    Q_INVOKABLE [[nodiscard]] QVariantMap lineFor(const QString &id) const;

Q_SIGNALS:
    void contactsChanged();
    void searchChanged();
    void availableChanged();
    void editorChanged();
    void linesChanged();

    void contactSubmitted(const softphone::core::Contact &contact);
    void contactDropped(const QString &id);

    void dialRequested(const QString &number, const QString &accountId);

    void dialRefused(Dialling::Refusal reason);

private:
    [[nodiscard]] const core::Contact *find(const QString &id) const;
    [[nodiscard]] core::Contact edited() const;

    [[nodiscard]] std::string lineLabel(const core::Contact &contact) const;
    [[nodiscard]] const core::AccountConfig *accountFor(const core::Contact &contact) const;

    [[nodiscard]] int linesWorthNaming() const;

    void reindex(std::vector<core::Contact> contacts);

    std::vector<ContactEntry> book_;
    ContactRows rows_;

    std::vector<core::AccountConfig> accounts_;
    std::string dialAccountId_;

    QString search_;
    QLocale collation_;
    bool available_{true};

    bool editing_{false};
    QString editingId_;
    QString name_;
    QString number_;
    QString accountId_;
};

}
