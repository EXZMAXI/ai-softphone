// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/busy_lamp.hpp>

#include <softphone/core/logging.hpp>

#include <algorithm>
#include <utility>

namespace softphone::core {

LineState WatchedLine::heard(const DialogInfoNotice &notice)
{

    if (notice.complete) {
        dialogs_.clear();
    }

    for (const auto &dialog : notice.dialogs) {
        const auto known = std::ranges::find(dialogs_, dialog.id, &WatchedDialog::id);

        if (dialog.state == LineState::Idle || dialog.state == LineState::Unknown) {
            if (known != dialogs_.end()) {
                dialogs_.erase(known);
            }
            continue;
        }

        if (known == dialogs_.end()) {
            dialogs_.push_back(dialog);
        } else {
            known->state = dialog.state;
        }
    }

    return showing();
}

LineState WatchedLine::showing() const
{
    if (std::ranges::any_of(dialogs_,
                            [](const auto &one) { return one.state == LineState::Busy; })) {
        return LineState::Busy;
    }
    if (std::ranges::any_of(dialogs_,
                            [](const auto &one) { return one.state == LineState::Ringing; })) {
        return LineState::Ringing;
    }
    return LineState::Idle;
}

void WatchedLine::forget()
{
    dialogs_.clear();
}

namespace {

[[nodiscard]] std::string accountFor(const PhoneButton &button, const std::string &fallback)
{
    return button.accountId.empty() ? fallback : button.accountId;
}

}

BusyLampController::BusyLampController(ILineWatchBackend &backend, StateCallback onChanged,
                                       Complete complete)
    : backend_{backend}, onChanged_{std::move(onChanged)}, complete_{std::move(complete)}
{
    if (!complete_) {
        complete_ = [](const std::string &number, const std::string &) { return number; };
    }

    backend_.setLineWatchObserver(this);
}

BusyLampController::~BusyLampController()
{
    backend_.setLineWatchObserver(nullptr);

    for (const auto &entry : watching_) {
        backend_.stopWatching(entry.first);
    }
}

void BusyLampController::setButtons(std::vector<PhoneButton> buttons, std::string defaultAccountId)
{
    buttons_ = std::move(buttons);
    defaultAccountId_ = std::move(defaultAccountId);

    reconcile();

    for (auto entry = states_.begin(); entry != states_.end();) {
        entry = find(entry->first) == nullptr ? states_.erase(entry) : std::next(entry);
    }
}

void BusyLampController::setAccountRegistered(const std::string &accountId, bool registered)
{
    const bool changed =
        registered ? registered_.insert(accountId).second : registered_.erase(accountId) > 0;
    if (!changed) {
        return;
    }

    reconcile();
}

LampState BusyLampController::stateOf(const std::string &buttonId) const
{
    const auto found = states_.find(buttonId);
    return found == states_.end() ? LampState{} : found->second;
}

std::map<std::string, LampState> BusyLampController::states() const
{
    return states_;
}

const PhoneButton *BusyLampController::find(const std::string &buttonId) const
{
    const auto found = std::ranges::find(buttons_, buttonId, &PhoneButton::id);
    return found == buttons_.end() ? nullptr : &*found;
}

void BusyLampController::reconcile()
{

    std::map<std::string, Watch> wanted;
    std::map<std::string, WatchTrouble> blocked;
    for (const auto &button : buttons_) {
        if (!button.watched || !isUsable(button)) {
            continue;
        }

        const auto account = accountFor(button, defaultAccountId_);
        if (account.empty() || !registered_.contains(account)) {

            Logger::instance().log(LogLevel::Info, "buttons",
                                   "not watching " + button.number + " yet: " +
                                       (account.empty()
                                            ? std::string{"the key names no line and there is no "
                                                          "dialling account to fall back on"}
                                            : "line " + account + " is not registered"));
            blocked.emplace(button.id, WatchTrouble::NoLine);
            continue;
        }

        wanted.emplace(button.id, Watch{account, complete_(button.number, account)});
    }

    for (auto entry = watching_.begin(); entry != watching_.end();) {
        const auto keep = wanted.find(entry->first);
        if (keep != wanted.end() && keep->second == entry->second) {
            ++entry;
            continue;
        }

        backend_.stopWatching(entry->first);
        entry = watching_.erase(entry);
    }

    for (const auto &[buttonId, watch] : wanted) {
        if (watching_.contains(buttonId)) {
            continue;
        }

        watching_.emplace(buttonId, watch);

        report(buttonId, LampState{LineState::Unknown, WatchTrouble::Asking});
        backend_.watchLine(buttonId, watch.accountId, watch.target);
    }

    for (const auto &button : buttons_) {
        if (watching_.contains(button.id)) {
            continue;
        }

        const auto why = blocked.find(button.id);
        report(button.id, LampState{LineState::Unknown,
                                    why == blocked.end() ? WatchTrouble::None : why->second});
    }
}

void BusyLampController::onLineState(const std::string &watchId, LineState state)
{
    if (!watching_.contains(watchId)) {

        return;
    }

    report(watchId, LampState{state, WatchTrouble::None});
}

void BusyLampController::onWatchFailed(const std::string &watchId, int sipStatus)
{
    const auto trouble = troubleFrom(sipStatus);

    Logger::instance().log(LogLevel::Warning, "buttons",
                           "the switch will not report " + watchId + ": " +
                               std::to_string(sipStatus) + " (" + std::string{toString(trouble)} +
                               ")");

    report(watchId, LampState{LineState::Unknown, trouble});

    const bool refused = trouble == WatchTrouble::NoSuchLine || trouble == WatchTrouble::Refused;
    if (!refused) {
        return;
    }

    backend_.stopWatching(watchId);
    watching_.erase(watchId);
}

void BusyLampController::report(const std::string &buttonId, LampState lamp)
{
    const auto found = states_.find(buttonId);
    if (found != states_.end() && found->second == lamp) {
        return;
    }

    states_[buttonId] = lamp;
    if (onChanged_) {
        onChanged_(buttonId, lamp);
    }
}

}
