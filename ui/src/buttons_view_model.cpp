// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "buttons_view_model.hpp"

#include <softphone/core/logging.hpp>

#include <QVariantMap>

#include <algorithm>
#include <utility>

namespace softphone::ui {
namespace {

[[nodiscard]] QString text(const std::string &value)
{
    return QString::fromStdString(value);
}

[[nodiscard]] ButtonsViewModel::Lamp toLamp(core::LineState state) noexcept
{
    switch (state) {
    case core::LineState::Unknown:
        return ButtonsViewModel::Lamp::Unknown;
    case core::LineState::Idle:
        return ButtonsViewModel::Lamp::Idle;
    case core::LineState::Ringing:
        return ButtonsViewModel::Lamp::Ringing;
    case core::LineState::Busy:
        return ButtonsViewModel::Lamp::Busy;
    }
    return ButtonsViewModel::Lamp::Unknown;
}

[[nodiscard]] ButtonsViewModel::Trouble toTrouble(core::WatchTrouble trouble) noexcept
{
    switch (trouble) {
    case core::WatchTrouble::None:
        return ButtonsViewModel::Trouble::None;
    case core::WatchTrouble::NoLine:
        return ButtonsViewModel::Trouble::NoLine;
    case core::WatchTrouble::Asking:
        return ButtonsViewModel::Trouble::Asking;
    case core::WatchTrouble::NoSuchLine:
        return ButtonsViewModel::Trouble::NoSuchLine;
    case core::WatchTrouble::Refused:
        return ButtonsViewModel::Trouble::Refused;
    case core::WatchTrouble::Silent:
        return ButtonsViewModel::Trouble::Silent;
    }
    return ButtonsViewModel::Trouble::Silent;
}

[[nodiscard]] QVariantMap asVariant(const core::PhoneButton &button)
{
    QVariantMap entry;
    entry.insert(QStringLiteral("id"), text(button.id));
    entry.insert(QStringLiteral("title"), text(button.title));
    entry.insert(QStringLiteral("number"), text(button.number));
    entry.insert(QStringLiteral("accountId"), text(button.accountId));
    entry.insert(QStringLiteral("watched"), button.watched);
    return entry;
}

}

constexpr auto kDeafFor = std::chrono::milliseconds{400};

ButtonsViewModel::ButtonsViewModel(Clock clock, QObject *parent)
    : QObject{parent}, clock_{std::move(clock)}
{
    if (!clock_) {
        clock_ = []() { return std::chrono::steady_clock::now(); };
    }
}

QVariantList ButtonsViewModel::buttons() const
{
    QVariantList rows;
    rows.reserve(static_cast<qsizetype>(buttons_.size()));

    for (const auto &button : buttons_) {
        auto entry = asVariant(button);

        entry.insert(QStringLiteral("label"), text(core::buttonLabel(button)));

        QString lineLabel;
        if (accounts_.size() > 1 && !button.accountId.empty()) {
            const auto found =
                std::ranges::find(accounts_, button.accountId, &core::AccountConfig::id);
            if (found != accounts_.end()) {
                lineLabel = text(core::accountLabel(*found));
            }
        }
        entry.insert(QStringLiteral("lineLabel"), lineLabel);

        const auto found = states_.find(button.id);
        const auto lamp = found == states_.end() ? core::LampState{} : found->second;
        entry.insert(QStringLiteral("state"), static_cast<int>(toLamp(lamp.state)));
        entry.insert(QStringLiteral("trouble"), static_cast<int>(toTrouble(lamp.trouble)));
        rows.append(entry);
    }

    return rows;
}

QVariantList ButtonsViewModel::panel() const
{
    QVariantList rows;
    for (const auto &entry : buttons()) {
        if (!entry.toMap().value(QStringLiteral("number")).toString().isEmpty()) {
            rows.append(entry);
        }
    }
    return rows;
}

bool ButtonsViewModel::any() const
{
    return std::ranges::any_of(
        buttons_, [](const core::PhoneButton &button) { return core::isUsable(button); });
}

QVariantList ButtonsViewModel::lines() const
{
    QVariantList rows;

    QVariantMap any;
    any.insert(QStringLiteral("id"), QString{});
    any.insert(QStringLiteral("label"), QString{});
    rows.append(any);

    for (const auto &account : accounts_) {
        if (!account.enabled) {
            continue;
        }

        QVariantMap entry;
        entry.insert(QStringLiteral("id"), text(account.id));
        entry.insert(QStringLiteral("label"), text(core::accountLabel(account)));
        rows.append(entry);
    }

    return rows;
}

void ButtonsViewModel::applyButtons(std::vector<core::PhoneButton> buttons)
{
    if (buttons == buttons_) {
        return;
    }

    buttons_ = std::move(buttons);

    for (auto entry = states_.begin(); entry != states_.end();) {
        const auto stillThere =
            std::ranges::any_of(buttons_, [&entry](const core::PhoneButton &button) {
                return button.id == entry->first;
            });
        entry = stillThere ? std::next(entry) : states_.erase(entry);
    }

    Q_EMIT buttonsChanged();
}

void ButtonsViewModel::applyAccounts(std::vector<core::AccountConfig> accounts,
                                     std::string dialAccountId)
{
    if (accounts == accounts_ && dialAccountId == dialAccountId_) {
        return;
    }

    accounts_ = std::move(accounts);
    dialAccountId_ = std::move(dialAccountId);
    Q_EMIT linesChanged();

    Q_EMIT buttonsChanged();
}

void ButtonsViewModel::applyState(const QString &buttonId, core::LampState lamp)
{
    const auto id = buttonId.toStdString();
    const auto found = states_.find(id);
    if (found != states_.end() && found->second == lamp) {
        return;
    }

    states_[id] = lamp;
    Q_EMIT buttonsChanged();
}

void ButtonsViewModel::refuseDial(Dialling::Refusal reason)
{
    Q_EMIT dialRefused(reason);
}

void ButtonsViewModel::dial(const QString &buttonId)
{
    const auto now = clock_();
    if (buttonId.toStdString() == dialledId_ && now - dialledAt_ < kDeafFor) {
        core::Logger::instance().log(core::LogLevel::Debug, "ui",
                                     "button dial: ignoring a second press of " +
                                         buttonId.toStdString());
        return;
    }

    const auto *button = find(buttonId);
    if (button == nullptr) {
        core::Logger::instance().log(core::LogLevel::Warning, "ui",
                                     "button dial: no key with id " + buttonId.toStdString());
        return;
    }

    if (button->number.empty()) {
        core::Logger::instance().log(core::LogLevel::Warning, "ui",
                                     "button dial: the key carries no number");
        Q_EMIT dialRefused(Dialling::Refusal::NothingToDial);
        return;
    }

    const auto fallback = button->accountId.empty() ? dialAccountId_ : button->accountId;
    const auto choice = core::chooseAccount(accounts_, {}, {}, fallback);
    if (choice.accountId.empty()) {
        core::Logger::instance().log(core::LogLevel::Warning, "ui",
                                     "button dial: no account is switched on to call from");
        Q_EMIT dialRefused(Dialling::Refusal::NoLine);
        return;
    }

    dialledId_ = button->id;
    dialledAt_ = now;

    Q_EMIT dialRequested(text(button->number), text(choice.accountId));
}

void ButtonsViewModel::select(const QString &buttonId)
{

    if (editing_ && !addingNew_ && selectedId_ == buttonId) {
        cancel();
        return;
    }

    const auto *button = find(buttonId);
    if (button == nullptr) {
        return;
    }

    selectedId_ = buttonId;
    editing_ = true;
    addingNew_ = false;
    draft_ = *button;

    Q_EMIT selectionChanged();
    Q_EMIT editorChanged();
}

void ButtonsViewModel::addButton()
{
    draft_ = core::PhoneButton{};
    draft_.id = core::nextButtonId(buttons_);

    draft_.watched = canWatch();

    selectedId_ = QString::fromStdString(draft_.id);
    editing_ = true;
    addingNew_ = true;

    Q_EMIT selectionChanged();
    Q_EMIT editorChanged();
}

void ButtonsViewModel::save()
{
    if (!editing_ || !canSave()) {
        return;
    }

    auto wanted = draft_;
    wanted.watched = wanted.watched && canWatch();

    wanted.title = QString::fromStdString(wanted.title).trimmed().toStdString();
    wanted.number = QString::fromStdString(wanted.number).trimmed().toStdString();

    if (auto *existing = find(QString::fromStdString(wanted.id)); existing != nullptr) {
        *existing = wanted;
    } else {
        buttons_.push_back(wanted);
    }

    editing_ = false;
    addingNew_ = false;

    Q_EMIT selectionChanged();
    Q_EMIT buttonsChanged();
    announce();
}

void ButtonsViewModel::cancel()
{
    if (!editing_) {
        return;
    }

    editing_ = false;
    addingNew_ = false;
    selectedId_.clear();
    draft_ = core::PhoneButton{};

    Q_EMIT selectionChanged();
    Q_EMIT editorChanged();
}

QString ButtonsViewModel::draftTitle() const
{
    return text(draft_.title);
}

QString ButtonsViewModel::draftNumber() const
{
    return text(draft_.number);
}

QString ButtonsViewModel::draftLine() const
{
    return text(draft_.accountId);
}

bool ButtonsViewModel::draftWatched() const noexcept
{
    return draft_.watched;
}

bool ButtonsViewModel::canWatch() const
{

    const auto enabled = std::ranges::count_if(
        accounts_, [](const core::AccountConfig &account) { return account.enabled; });

    return enabled <= 1 || !draft_.accountId.empty();
}

bool ButtonsViewModel::canSave() const
{
    return editing_ && !draft_.number.empty();
}

void ButtonsViewModel::setDraftTitle(const QString &title)
{
    if (text(draft_.title) == title) {
        return;
    }

    draft_.title = title.toStdString();
    Q_EMIT editorChanged();
}

void ButtonsViewModel::setDraftNumber(const QString &number)
{
    if (text(draft_.number) == number) {
        return;
    }
    draft_.number = number.toStdString();
    Q_EMIT editorChanged();
}

void ButtonsViewModel::setDraftLine(const QString &accountId)
{
    if (text(draft_.accountId) == accountId) {
        return;
    }
    draft_.accountId = accountId.toStdString();

    if (!canWatch()) {
        draft_.watched = false;
    }
    Q_EMIT editorChanged();
}

void ButtonsViewModel::setDraftWatched(bool watched)
{
    if (draft_.watched == watched) {
        return;
    }
    draft_.watched = watched && canWatch();
    Q_EMIT editorChanged();
}

void ButtonsViewModel::removeButton(const QString &buttonId)
{
    const auto id = buttonId.toStdString();
    const auto before = buttons_.size();
    std::erase_if(buttons_, [&id](const core::PhoneButton &button) { return button.id == id; });

    if (buttons_.size() == before) {
        return;
    }

    states_.erase(id);
    if (selectedId_ == buttonId) {
        cancel();
    }

    Q_EMIT buttonsChanged();
    announce();
}

void ButtonsViewModel::moveButton(const QString &buttonId, int by)
{
    const auto id = buttonId.toStdString();
    const auto found = std::ranges::find(buttons_, id, &core::PhoneButton::id);
    if (found == buttons_.end() || by == 0) {
        return;
    }

    const auto at = std::distance(buttons_.begin(), found);
    const auto to = at + by;
    if (to < 0 || to >= static_cast<long>(buttons_.size())) {
        return;
    }

    std::swap(buttons_[static_cast<std::size_t>(at)], buttons_[static_cast<std::size_t>(to)]);
    Q_EMIT buttonsChanged();
    announce();
}

core::PhoneButton *ButtonsViewModel::find(const QString &buttonId)
{
    const auto id = buttonId.toStdString();
    const auto found = std::ranges::find(buttons_, id, &core::PhoneButton::id);
    return found == buttons_.end() ? nullptr : &*found;
}

void ButtonsViewModel::announce()
{

    QVariantList stored;
    for (const auto &button : buttons_) {
        if (core::isUsable(button)) {
            stored.append(asVariant(button));
        }
    }

    Q_EMIT buttonsEdited(stored);
}

}
