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

#include <softphone/core/call.hpp>
#include <softphone/core/call_backend.hpp>
#include <softphone/core/logging.hpp>
#include <softphone/core/scheduler.hpp>
#include <softphone/core/settings.hpp>

#include <atomic>
#include <chrono>
#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace softphone::core {

struct CallPolicy {

    std::chrono::milliseconds ringTimeout{std::chrono::seconds{60}};

    std::size_t maxConcurrentCalls{16};

    std::chrono::milliseconds transferTimeout{std::chrono::minutes{5}};

    DtmfTransport dtmfTransport{DtmfTransport::Rfc2833};

    CallWaiting callWaiting{CallWaiting::Allowed};
};

[[nodiscard]] bool busyBecauseCallWaitingIsOff(CallWaiting waiting, const AutoAnswerRequest &asked,
                                               const OtherCalls &others) noexcept;

struct AccountCallPolicy {
    DtmfTransport dtmfTransport{DtmfTransport::Rfc2833};

    AutoAnswerPolicy autoAnswer;

    friend bool operator==(const AccountCallPolicy &, const AccountCallPolicy &) = default;
};

[[nodiscard]] AccountCallPolicy callPolicyFor(const AccountConfig &account);

class CallController : public ICallBackend::Observer {
public:

    using CallCallback = std::function<void(const CallInfo &call)>;

    CallController(ICallBackend &backend, IScheduler &scheduler, CallPolicy policy,
                   CallCallback onCallChanged,
                   std::function<std::chrono::system_clock::time_point()> wallClock = {});

    CallController(const CallController &) = delete;
    CallController &operator=(const CallController &) = delete;
    CallController(CallController &&) = delete;
    CallController &operator=(CallController &&) = delete;
    ~CallController() override;

    void setPolicy(CallPolicy policy);

    void setCallWaiting(CallWaiting waiting);

private:

    void tellTheBackendWhatASecondCallerMeets();

public:

    void setDoNotDisturb(bool refusing)
    {
        doNotDisturb_.store(refusing, std::memory_order_relaxed);

        backend_.refuseIncomingCalls(refusing);
    }

    [[nodiscard]] bool doNotDisturb() const noexcept
    {
        return doNotDisturb_.load(std::memory_order_relaxed);
    }

    [[nodiscard]] std::string lastDialled() const
    {
        const std::lock_guard guard{lastDialledLock_};
        return lastDialled_;
    }

    [[nodiscard]] const CallPolicy &policy() const noexcept
    {
        return policy_;
    }

    std::string place(const std::string &accountId, const std::string &target,
                      std::string dialed = {}, int redialAttempt = 0,
                      std::string redialContinues = {}, std::string seance = {});

    void answer(const std::string &callId, AnswerMode mode = AnswerMode::HoldOthers);

    void reject(const std::string &callId);

    void hangUp(const std::string &callId);
    void setMuted(const std::string &callId, bool muted);

    void sendDtmf(const std::string &callId, char digit);

    void setAccountPolicy(const std::string &accountId, AccountCallPolicy policy);
    void forgetAccountPolicy(const std::string &accountId);

    [[nodiscard]] AccountCallPolicy accountPolicy(const std::string &accountId) const;

    void hold(const std::string &callId);

    void activate(const std::string &callId);

    void beginTransfer(const std::string &callId);

    void cancelTransfer(const std::string &callId);

    void transferBlind(const std::string &callId, const std::string &target);

    void transferAttended(const std::string &callId, const std::string &otherCallId);

    void merge(const std::vector<std::string> &callIds);

    void split(const std::string &keep);

    void endConference();

    [[nodiscard]] std::vector<CallInfo> conferenceCandidates() const;

    [[nodiscard]] std::vector<CallInfo> transferCandidates(const std::string &callId) const;

    void onIncomingCall(const IncomingCall &incoming) override;
    void onCallProgress(const CallProgress &progress) override;
    void onRemoteHoldChanged(const std::string &callId, bool held) override;
    void onEarlyMediaChanged(const std::string &callId, bool arriving) override;
    void onRemoteIdentityChanged(const std::string &callId, const SipAddress &identity) override;
    void onTransferProgress(const TransferProgress &progress) override;
    void onTransferRequested(const std::string &callId, const std::string &newCallId,
                             const std::string &target, const std::string &referredBy) override;
    void onCallReplaced(const std::string &oldCallId, const std::string &newCallId) override;
    void onSwitchRequest(const std::string &accountId, SwitchRequest what,
                         const std::string &callId) override;

    [[nodiscard]] std::optional<CallInfo> active() const;

    [[nodiscard]] std::optional<CallInfo> current() const;

    [[nodiscard]] bool busy() const noexcept
    {
        return !calls_.empty();
    }

    [[nodiscard]] std::vector<CallInfo> calls() const;

    void audioDeviceChanged();

private:

    static void note(LogLevel level, const CallInfo &call, const std::string &what);

    static void note(LogLevel level, const std::string &callId, const std::string &line,
                     const std::string &what);

    void routeAudio();

    void refreshConference();

    void muteConference(bool muted);

    void holdOthers(const std::string &keep);

    void finish(const std::string &callId, CallEndInfo end);
    void scheduleRingTimeout(const std::string &callId);
    void cancelRingTimeout(const std::string &callId);

    [[nodiscard]] OtherCalls otherCalls(const std::string &callId, const std::string &line) const;

    void actOnSwitchRequest(const CallInfo &target, SwitchRequest what);

    void scheduleAutoAnswer(const std::string &callId, std::chrono::seconds after);
    void answerAutomatically(const std::string &callId);
    void cancelAutoAnswer(const std::string &callId);

    void finishTransferred(const std::string &callId);

    void failTransfer(const std::string &callId, int sipStatus);

    void scheduleTransferTimeout(const std::string &callId);
    void cancelTransferTimeout(const std::string &callId);
    void notify(const CallInfo &call);

    void refused(const IncomingCall &incoming, CallEndReason why);

    ICallBackend &backend_;
    IScheduler &scheduler_;
    CallPolicy policy_;
    std::function<std::chrono::system_clock::time_point()> wallClock_;
    std::atomic<bool> doNotDisturb_{false};

    mutable std::mutex lastDialledLock_;
    std::string lastDialled_;
    std::map<std::string, AccountCallPolicy> accountPolicies_;
    CallCallback onCallChanged_;

    std::map<std::string, CallInfo> calls_;
    std::map<std::string, IScheduler::TimerId> ringTimers_;
    std::map<std::string, IScheduler::TimerId> autoAnswerTimers_;
    std::map<std::string, IScheduler::TimerId> transferTimers_;

    std::set<std::string> parkedForTransfer_;
    std::map<std::string, CallInfo> lastNotified_;
    unsigned nextCallNumber_{0};
};

}
