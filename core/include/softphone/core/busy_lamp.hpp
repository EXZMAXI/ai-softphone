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

#include <softphone/core/buttons.hpp>

#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace softphone::core {

struct WatchedDialog {
    std::string id;
    LineState state{LineState::Unknown};

    friend bool operator==(const WatchedDialog &, const WatchedDialog &) = default;
};

struct DialogInfoNotice {

    bool complete{false};

    std::vector<WatchedDialog> dialogs;

    friend bool operator==(const DialogInfoNotice &, const DialogInfoNotice &) = default;
};

class WatchedLine {
public:

    LineState heard(const DialogInfoNotice &notice);

    [[nodiscard]] LineState showing() const;

    [[nodiscard]] const std::vector<WatchedDialog> &dialogs() const noexcept
    {
        return dialogs_;
    }

    void forget();

private:
    std::vector<WatchedDialog> dialogs_;
};

class ILineWatchBackend {
public:
    class Observer {
    public:
        Observer() = default;
        Observer(const Observer &) = delete;
        Observer &operator=(const Observer &) = delete;
        Observer(Observer &&) = delete;
        Observer &operator=(Observer &&) = delete;
        virtual ~Observer() = default;

        virtual void onLineState(const std::string &watchId, LineState state) = 0;

        virtual void onWatchFailed(const std::string &watchId, int sipStatus) = 0;
    };

    ILineWatchBackend() = default;
    ILineWatchBackend(const ILineWatchBackend &) = delete;
    ILineWatchBackend &operator=(const ILineWatchBackend &) = delete;
    ILineWatchBackend(ILineWatchBackend &&) = delete;
    ILineWatchBackend &operator=(ILineWatchBackend &&) = delete;
    virtual ~ILineWatchBackend() = default;

    virtual void setLineWatchObserver(Observer *observer) = 0;

    virtual void watchLine(const std::string &watchId, const std::string &accountId,
                           const std::string &number) = 0;

    virtual void stopWatching(const std::string &watchId) = 0;
};

class BusyLampController : public ILineWatchBackend::Observer {
public:

    using StateCallback = std::function<void(const std::string &buttonId, LampState lamp)>;

    using Complete =
        std::function<std::string(const std::string &number, const std::string &accountId)>;

    BusyLampController(ILineWatchBackend &backend, StateCallback onChanged, Complete complete = {});

    BusyLampController(const BusyLampController &) = delete;
    BusyLampController &operator=(const BusyLampController &) = delete;
    BusyLampController(BusyLampController &&) = delete;
    BusyLampController &operator=(BusyLampController &&) = delete;
    ~BusyLampController() override;

    void setButtons(std::vector<PhoneButton> buttons, std::string defaultAccountId);

    void setAccountRegistered(const std::string &accountId, bool registered);

    [[nodiscard]] LampState stateOf(const std::string &buttonId) const;

    [[nodiscard]] std::map<std::string, LampState> states() const;

    [[nodiscard]] const PhoneButton *find(const std::string &buttonId) const;

private:
    void onLineState(const std::string &watchId, LineState state) override;
    void onWatchFailed(const std::string &watchId, int sipStatus) override;

    void reconcile();

    void report(const std::string &buttonId, LampState lamp);

    ILineWatchBackend &backend_;
    StateCallback onChanged_;
    Complete complete_;

    std::vector<PhoneButton> buttons_;
    std::string defaultAccountId_;

    std::set<std::string> registered_;

    struct Watch {
        std::string accountId;
        std::string target;

        friend bool operator==(const Watch &, const Watch &) = default;
    };

    std::map<std::string, Watch> watching_;

    std::map<std::string, LampState> states_;
};

}
