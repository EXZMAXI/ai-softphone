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

#include <softphone/core/auto_answer.hpp>
#include <softphone/core/call.hpp>

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace softphone::core {

enum class BackendCallState {
    Calling,

    Ringing,
    Connected,
    Disconnected,
};

struct CallProgress {
    std::string callId;
    BackendCallState state{BackendCallState::Disconnected};
    int sipStatus{0};
    std::string reason;

    bool endedLocally{false};

    LocalFailure localFailure{LocalFailure::None};

    bool completedElsewhere{false};
};

struct RtpDirection {
    unsigned packets{0};
    std::uint64_t bytes{0};

    unsigned lost{0};

    unsigned discarded{0};

    unsigned jitterMeanUsec{0};
    unsigned jitterMaxUsec{0};
};

struct CallMediaStats {

    RtpDirection sent;

    RtpDirection received;

    unsigned roundTripMeanUsec{0};
    unsigned roundTripMaxUsec{0};

    std::string codec;
    unsigned clockRate{0};

    unsigned sentPayloadType{0};
    unsigned receivedPayloadType{0};

    std::string localAddress;
    std::string remoteAddress;

    bool encrypted{false};

    [[nodiscard]] unsigned packetsSent() const noexcept
    {
        return sent.packets;
    }

    [[nodiscard]] unsigned packetsReceived() const noexcept
    {
        return received.packets;
    }

    [[nodiscard]] bool flowingBothWays() const noexcept
    {
        return sent.packets > 0 && received.packets > 0;
    }
};

[[nodiscard]] bool streamRestarted(const CallMediaStats &previous,
                                   const CallMediaStats &current) noexcept;

struct CallMedia {
    std::string callId;

    std::string number;
    std::string name;
    CallState state{CallState::Idle};
    CallDirection direction{CallDirection::Outgoing};

    std::chrono::seconds duration{0};

    bool hasMedia{false};

    CallMediaStats stats;
};

struct TransferProgress {

    std::string callId;

    int sipStatus{0};

    bool finalNotify{false};

    std::string reason;
};

struct IncomingCall {
    std::string callId;
    std::string accountId;
    std::string remoteUri;
    std::string remoteDisplayName;

    std::string referredBy;

    AutoAnswerRequest autoAnswer;

    CallEndReason refusedBeforeItRang{CallEndReason::None};

    std::string seanceId{};

    bool fromThisLine{false};
};

class ICallBackend {
public:
    class Observer {
    public:
        Observer() = default;
        Observer(const Observer &) = delete;
        Observer &operator=(const Observer &) = delete;
        Observer(Observer &&) = delete;
        Observer &operator=(Observer &&) = delete;
        virtual ~Observer() = default;

        virtual void onIncomingCall(const IncomingCall &call) = 0;
        virtual void onCallProgress(const CallProgress &progress) = 0;

        virtual void onRemoteHoldChanged(const std::string &callId, bool held) = 0;

        virtual void onEarlyMediaChanged(const std::string &callId, bool arriving) = 0;

        virtual void onRemoteIdentityChanged(const std::string &callId,
                                             const SipAddress &identity) = 0;

        virtual void onTransferProgress(const TransferProgress &progress) = 0;

        virtual void onTransferRequested(const std::string &callId, const std::string &newCallId,
                                         const std::string &target,
                                         const std::string &referredBy) = 0;

        virtual void onCallReplaced(const std::string &oldCallId, const std::string &newCallId) = 0;

        virtual void onSwitchRequest(const std::string &accountId, SwitchRequest what,
                                     const std::string &callId) = 0;
    };

    ICallBackend() = default;
    ICallBackend(const ICallBackend &) = delete;
    ICallBackend &operator=(const ICallBackend &) = delete;
    ICallBackend(ICallBackend &&) = delete;
    ICallBackend &operator=(ICallBackend &&) = delete;
    virtual ~ICallBackend() = default;

    virtual void setCallObserver(Observer *observer) = 0;

    virtual void placeCall(const std::string &callId, const std::string &accountId,
                           const std::string &target) = 0;

    virtual void noteSeance(const std::string & , const std::string & ) {}

    virtual void answerCall(const std::string &callId) = 0;

    virtual void rejectCall(const std::string &callId, int sipStatus) = 0;

    virtual void refuseIncomingCalls(bool refusing) = 0;

    virtual void refuseSecondCalls(bool refusing) = 0;

    virtual void hangUpCall(const std::string &callId) = 0;
    virtual void setCallMuted(const std::string &callId, bool muted) = 0;

    virtual void holdCall(const std::string &callId) = 0;
    virtual void resumeCall(const std::string &callId) = 0;

    virtual void routeAudioTo(const std::vector<std::string> &callIds) = 0;

    virtual void reconnectAudio() = 0;

    virtual void transferCall(const std::string &callId, const std::string &target) = 0;

    virtual void transferCallToCall(const std::string &callId,
                                    const std::string &consultationCallId) = 0;

    virtual void sendDtmf(const std::string &callId, char digit, DtmfTransport transport) = 0;

    [[nodiscard]] virtual std::optional<CallMediaStats>
    callMediaStats(const std::string &callId) const = 0;
};

}
