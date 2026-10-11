// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "contacts_view_model.hpp"

#include <softphone/core/logging.hpp>

#include <QCollator>
#include <QVariantMap>

#include <algorithm>

namespace softphone::ui {
namespace {

[[nodiscard]] QString text(const std::string &value)
{
    return QString::fromStdString(value);
}

}

ContactsViewModel::ContactsViewModel(QObject *parent) : QObject{parent}, rows_{*this}
{
    setLocale(QLocale{});
}

ContactRows::ContactRows(ContactsViewModel &owner) : owner_{owner} {}

int ContactRows::rowCount(const QModelIndex &parent) const
{

    return parent.isValid() ? 0 : static_cast<int>(shown_.size());
}

QVariant ContactRows::data(const QModelIndex &index, int role) const
{
    if (role != Qt::UserRole || index.row() < 0 || index.row() >= static_cast<int>(shown_.size())) {
        return {};
    }

    return owner_.rowFor(shown_[static_cast<std::size_t>(index.row())]);
}

QHash<int, QByteArray> ContactRows::roleNames() const
{

    return {{Qt::UserRole, QByteArrayLiteral("contact")}};
}

void ContactRows::refill()
{

    beginResetModel();
    shown_.clear();
    shown_.reserve(owner_.book().size());
    for (std::size_t at = 0; at < owner_.book().size(); ++at) {
        if (owner_.matches(owner_.book()[at])) {
            shown_.push_back(at);
        }
    }
    endResetModel();
}

void ContactsViewModel::setLocale(const QLocale &locale)
{

    collation_ = locale.language() == QLocale::C ? QLocale{QLocale::English} : locale;

    std::vector<core::Contact> again;
    again.reserve(book_.size());
    for (auto &entry : book_) {
        again.push_back(std::move(entry.contact));
    }
    reindex(std::move(again));

    Q_EMIT contactsChanged();
}

void ContactsViewModel::reindex(std::vector<core::Contact> contacts)
{

    QCollator collation{collation_};
    collation.setCaseSensitivity(Qt::CaseInsensitive);

    struct Ordering {
        QCollatorSortKey key;
        std::size_t at;
    };

    std::vector<Ordering> order;
    order.reserve(contacts.size());
    for (std::size_t at = 0; at < contacts.size(); ++at) {
        order.push_back(Ordering{collation.sortKey(QString::fromStdString(contacts[at].name)), at});
    }

    std::ranges::sort(order, [&contacts](const Ordering &left, const Ordering &right) {
        const auto ranking = left.key.compare(right.key);

        return ranking != 0 ? ranking < 0 : contacts[left.at].id < contacts[right.at].id;
    });

    book_.clear();
    book_.reserve(contacts.size());
    for (auto &ranked : order) {
        auto &contact = contacts[ranked.at];
        auto name = QString::fromStdString(contact.name);
        auto number = QString::fromStdString(contact.number);
        book_.push_back(ContactEntry{std::move(contact), std::move(name), std::move(number)});
    }

    rows_.refill();
}

QAbstractItemModel *ContactsViewModel::rows()
{
    return &rows_;
}

int ContactsViewModel::shown() const
{
    return rows_.rowCount();
}

const std::vector<ContactEntry> &ContactsViewModel::book() const
{
    return book_;
}

bool ContactsViewModel::matches(const ContactEntry &entry) const
{

    return search_.isEmpty() || entry.name.contains(search_, Qt::CaseInsensitive) ||
           entry.number.contains(search_, Qt::CaseInsensitive);
}

QVariantMap ContactsViewModel::rowFor(std::size_t at) const
{
    const auto &entry = book_.at(at);
    const auto &contact = entry.contact;

    QVariantMap row;
    row.insert(QStringLiteral("id"), text(contact.id));
    row.insert(QStringLiteral("name"), entry.name);
    row.insert(QStringLiteral("number"), entry.number);
    row.insert(QStringLiteral("accountId"), text(contact.accountId));

    row.insert(QStringLiteral("lineLabel"), text(lineLabel(contact)));

    const auto choice =
        core::chooseAccount(accounts_, contact.accountId, contact.accountAddress, dialAccountId_);

    row.insert(QStringLiteral("lineMissing"),
               choice.asked && !choice.honoured && !choice.accountId.empty());

    return row;
}

int ContactsViewModel::total() const
{
    return static_cast<int>(book_.size());
}

QString ContactsViewModel::search() const
{
    return search_;
}

void ContactsViewModel::setSearch(const QString &value)
{
    if (search_ == value) {
        return;
    }
    search_ = value;
    rows_.refill();
    Q_EMIT searchChanged();
    Q_EMIT contactsChanged();
}

void ContactsViewModel::setAvailable(bool available)
{
    if (available_ == available) {
        return;
    }
    available_ = available;
    Q_EMIT availableChanged();
}

QString ContactsViewModel::editingId() const
{
    return editingId_;
}

QString ContactsViewModel::name() const
{
    return name_;
}

void ContactsViewModel::setName(const QString &value)
{
    if (name_ == value) {
        return;
    }
    name_ = value;
    Q_EMIT editorChanged();
}

QString ContactsViewModel::number() const
{
    return number_;
}

void ContactsViewModel::setNumber(const QString &value)
{
    if (number_ == value) {
        return;
    }
    number_ = value;
    Q_EMIT editorChanged();
}

QString ContactsViewModel::accountId() const
{
    return accountId_;
}

void ContactsViewModel::setAccountId(const QString &value)
{
    if (accountId_ == value) {
        return;
    }
    accountId_ = value;
    Q_EMIT editorChanged();
}

core::Contact ContactsViewModel::edited() const
{
    core::Contact contact;
    contact.id = editingId_.toStdString();
    contact.name = name_.trimmed().toStdString();
    contact.number = number_.trimmed().toStdString();
    contact.accountId = accountId_.toStdString();

    for (const auto &account : accounts_) {
        if (account.uuid == contact.accountId) {
            contact.accountAddress = core::accountAddress(account);
            break;
        }
    }
    return contact;
}

ContactsViewModel::Fault ContactsViewModel::fault() const
{

    switch (core::validate(edited())) {
    case core::ContactProblem::None:
        return Fault::None;
    case core::ContactProblem::MissingName:
        return Fault::MissingName;
    case core::ContactProblem::MissingNumber:
        return Fault::MissingNumber;
    }
    return Fault::None;
}

bool ContactsViewModel::canSave() const
{
    return editing_ && available_ && fault() == Fault::None;
}

QVariantList ContactsViewModel::lines() const
{
    QVariantList rows;
    for (const auto &account : accounts_) {
        const auto address = core::accountAddress(account);
        if (address.empty() || !account.enabled || account.uuid.empty()) {

            continue;
        }

        QVariantMap row;
        row.insert(QStringLiteral("id"), text(account.uuid));
        row.insert(QStringLiteral("address"), text(address));
        row.insert(QStringLiteral("label"), text(core::accountLabel(account)));
        rows.append(row);
    }
    return rows;
}

void ContactsViewModel::applyContacts(std::vector<core::Contact> contacts)
{
    reindex(std::move(contacts));

    Q_EMIT contactsChanged();
}

void ContactsViewModel::applyAccounts(std::vector<core::AccountConfig> accounts,
                                      std::string dialAccountId)
{
    accounts_ = std::move(accounts);
    dialAccountId_ = std::move(dialAccountId);

    rows_.refill();
    Q_EMIT linesChanged();

    Q_EMIT contactsChanged();
}

void ContactsViewModel::addContact(const QString &number, const QString &name)
{
    editing_ = true;
    editingId_.clear();
    name_ = name;
    number_ = number;
    accountId_.clear();
    Q_EMIT editorChanged();
}

void ContactsViewModel::edit(const QString &id)
{
    const auto *contact = find(id);
    if (contact == nullptr) {
        return;
    }

    editing_ = true;
    editingId_ = text(contact->id);
    name_ = text(contact->name);
    number_ = text(contact->number);
    accountId_ = text(contact->accountId);
    Q_EMIT editorChanged();
}

void ContactsViewModel::cancelEdit()
{
    if (!editing_) {
        return;
    }

    editing_ = false;
    editingId_.clear();
    name_.clear();
    number_.clear();
    accountId_.clear();
    Q_EMIT editorChanged();
}

void ContactsViewModel::save()
{
    if (!canSave()) {
        return;
    }

    Q_EMIT contactSubmitted(edited());
    cancelEdit();
}

void ContactsViewModel::removeContact(const QString &id)
{
    const auto *contact = find(id);
    if (contact == nullptr) {
        return;
    }

    const auto gone = text(contact->id);
    if (editing_ && editingId_ == gone) {
        cancelEdit();
    }
    Q_EMIT contactDropped(gone);
}

void ContactsViewModel::dial(const QString &id)
{
    const auto *contact = find(id);
    if (contact == nullptr) {
        core::Logger::instance().log(core::LogLevel::Warning, "ui",
                                     "dial: no contact with id " + id.toStdString());
        return;
    }

    if (contact->number.empty()) {
        core::Logger::instance().log(core::LogLevel::Warning, "ui",
                                     "dial: the contact carries no number");
        Q_EMIT dialRefused(Dialling::Refusal::NothingToDial);
        return;
    }

    const auto choice =
        core::chooseAccount(accounts_, contact->accountId, contact->accountAddress, dialAccountId_);
    if (choice.accountId.empty()) {

        core::Logger::instance().log(core::LogLevel::Warning, "ui",
                                     "dial: no account is switched on to call from");
        Q_EMIT dialRefused(Dialling::Refusal::NoLine);
        return;
    }

    Q_EMIT dialRequested(text(contact->number), text(choice.accountId));
}

void ContactsViewModel::refuseDial(Dialling::Refusal reason)
{
    Q_EMIT dialRefused(reason);
}

QVariantMap ContactsViewModel::lineFor(const QString &id) const
{
    QVariantMap answer;
    const auto *contact = find(id);
    if (contact == nullptr) {
        return answer;
    }

    const auto choice =
        core::chooseAccount(accounts_, contact->accountId, contact->accountAddress, dialAccountId_);
    const auto at = std::ranges::find_if(
        accounts_, [&choice](const auto &candidate) { return candidate.id == choice.accountId; });

    answer.insert(QStringLiteral("asked"), choice.asked);
    answer.insert(QStringLiteral("honoured"), choice.honoured);
    answer.insert(QStringLiteral("label"),
                  at == accounts_.end() ? QString{} : text(core::accountLabel(*at)));
    return answer;
}

const core::Contact *ContactsViewModel::find(const QString &id) const
{
    const auto wanted = id.toStdString();
    const auto at = std::ranges::find_if(
        book_, [&wanted](const ContactEntry &entry) { return entry.contact.id == wanted; });
    return at == book_.end() ? nullptr : &at->contact;
}

const core::AccountConfig *ContactsViewModel::accountFor(const core::Contact &contact) const
{

    for (const auto &account : accounts_) {
        if (account.enabled && !contact.accountId.empty() && account.uuid == contact.accountId) {
            return &account;
        }
    }
    for (const auto &account : accounts_) {
        if (account.enabled && !contact.accountAddress.empty() &&
            core::accountAddress(account) == contact.accountAddress) {
            return &account;
        }
    }
    return nullptr;
}

int ContactsViewModel::linesWorthNaming() const
{
    return static_cast<int>(
        std::ranges::count_if(accounts_, [](const auto &account) { return account.enabled; }));
}

std::string ContactsViewModel::lineLabel(const core::Contact &contact) const
{
    if (contact.accountId.empty() && contact.accountAddress.empty()) {
        return {};
    }

    if (const auto *account = accountFor(contact); account != nullptr) {

        return linesWorthNaming() < 2 ? std::string{} : core::accountLabel(*account);
    }

    return contact.accountAddress;
}

}
