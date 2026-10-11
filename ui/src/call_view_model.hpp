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

#include <softphone/core/account_config.hpp>
#include <softphone/core/call.hpp>
#include <softphone/core/call_controller.hpp>
#include <softphone/core/contacts.hpp>
#include <softphone/core/recording_controller.hpp>
#include <softphone/core/redial.hpp>
#include <softphone/core/settings.hpp>
#include <softphone/core/tones.hpp>

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

#include <chrono>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace softphone::ui {

class CallViewModel : public QObject {
    Q_OBJECT

    QML_ELEMENT
    QML_UNCREATABLE("Created by the application and exposed as `calls`.")

    Q_PROPERTY(bool busy READ busy NOTIFY callChanged)

    Q_PROPERTY(bool hasCall READ hasCall NOTIFY hasCallChanged)

    Q_PROPERTY(bool ended READ ended NOTIFY callChanged)

    Q_PROPERTY(Ending ending READ ending NOTIFY callChanged)

    Q_PROPERTY(State state READ state NOTIFY callChanged)
    Q_PROPERTY(Direction direction READ direction NOTIFY callChanged)
    Q_PROPERTY(QString remoteLabel READ remoteLabel NOTIFY callChanged)

    Q_PROPERTY(QString currentCallId READ currentCallId NOTIFY callChanged)

    Q_PROPERTY(QString accountId READ accountId NOTIFY callChanged)

    Q_PROPERTY(QString remoteDetail READ remoteDetail NOTIFY callChanged)
    Q_PROPERTY(bool muted READ muted NOTIFY callChanged)
    Q_PROPERTY(bool incoming READ incoming NOTIFY callChanged)
    Q_PROPERTY(bool talking READ talking NOTIFY callChanged)

    Q_PROPERTY(bool canSendDtmf READ canSendDtmf NOTIFY callChanged)

    Q_PROPERTY(Transfer transfer READ transfer NOTIFY callChanged)

    Q_PROPERTY(QString transferTarget READ transferTarget NOTIFY callChanged)

    Q_PROPERTY(int transferStatus READ transferStatus NOTIFY callChanged)

    Q_PROPERTY(bool transferUnanswered READ transferUnanswered NOTIFY callChanged)

    Q_PROPERTY(bool canTransfer READ canTransfer NOTIFY callChanged)

    Q_PROPERTY(QVariantList transferTargets READ transferTargets NOTIFY callChanged)

    Q_PROPERTY(QString referredBy READ referredBy NOTIFY callChanged)

    Q_PROPERTY(bool inConference READ inConference NOTIFY callChanged)

    Q_PROPERTY(QVariantList conferenceList READ conferenceList NOTIFY callChanged)

    Q_PROPERTY(bool canMerge READ canMerge NOTIFY callChanged)

    Q_PROPERTY(QString mergePartner READ mergePartner NOTIFY callChanged)

    Q_PROPERTY(bool held READ held NOTIFY callChanged)
    Q_PROPERTY(bool heldByThem READ heldByThem NOTIFY callChanged)

    Q_PROPERTY(bool offeringWhileBusy READ offeringWhileBusy NOTIFY callChanged)
    Q_PROPERTY(QString offeredLabel READ offeredLabel NOTIFY callChanged)
    Q_PROPERTY(QString offeredDetail READ offeredDetail NOTIFY callChanged)

    Q_PROPERTY(QVariantList callList READ callList NOTIFY callChanged)

    Q_PROPERTY(bool moreThanTheCard READ moreThanTheCard NOTIFY callChanged)

    Q_PROPERTY(qint64 connectedAtMs READ connectedAtMs NOTIFY callChanged)
    Q_PROPERTY(qint64 nowMs READ nowMs NOTIFY nowMsChanged)

    Q_PROPERTY(qint64 startedAtMs READ startedAtMs NOTIFY callChanged)

    Q_PROPERTY(bool redialling READ redialling NOTIFY redialChanged)
    Q_PROPERTY(QString redialNumber READ redialNumber NOTIFY redialChanged)
    Q_PROPERTY(int redialAttempts READ redialAttempts NOTIFY redialChanged)
    Q_PROPERTY(qint64 redialNextAtMs READ redialNextAtMs NOTIFY redialChanged)
    Q_PROPERTY(qint64 redialGiveUpAtMs READ redialGiveUpAtMs NOTIFY redialChanged)

    Q_PROPERTY(bool redialWaiting READ redialWaiting NOTIFY redialChanged)

    Q_PROPERTY(Redial redialEnded READ redialEnded NOTIFY redialChanged)

    Q_PROPERTY(bool canRedial READ canRedial NOTIFY callChanged)

    Q_PROPERTY(qint64 endedAtMs READ endedAtMs NOTIFY callChanged)

    Q_PROPERTY(bool recording READ recording NOTIFY recordingChanged)
    Q_PROPERTY(bool recordingPaused READ recordingPaused NOTIFY recordingChanged)
    Q_PROPERTY(bool recordingKeeping READ recordingKeeping NOTIFY recordingChanged)

    Q_PROPERTY(bool recordingShown READ recordingShown NOTIFY recordingChanged)

    Q_PROPERTY(bool askingToRecord READ askingToRecord NOTIFY recordingChanged)

    Q_PROPERTY(bool canRecord READ canRecord NOTIFY recordingChanged)
    Q_PROPERTY(bool canStartRecording READ canStartRecording NOTIFY recordingChanged)

    Q_PROPERTY(bool doNotDisturb READ doNotDisturb NOTIFY doNotDisturbChanged)

    Q_PROPERTY(QString lastDialled READ lastDialled NOTIFY lastDialledChanged)

    Q_PROPERTY(QString dialText READ dialText WRITE setDialText NOTIFY dialTextChanged)
    Q_PROPERTY(bool canDial READ canDial NOTIFY dialTextChanged)

public:

    enum class State {
        Idle,
        Dialing,
        RingingOut,
        RingingIn,
        Active,
        Ended,
    };
    Q_ENUM(State)

    enum class Direction {
        Outgoing,
        Incoming,
    };
    Q_ENUM(Direction)

    enum class Ending {
        None,
        LocalHangup,
        RemoteHangup,
        Transferred,
        Busy,
        Declined,
        NoAnswer,
        Cancelled,
        Unavailable,
        NumberNotFound,
        NotAllowed,
        MediaMismatch,
        ServerError,
        Unreachable,
        NoAudioDevice,
        Failed,

        PickedUpElsewhere,
    };
    Q_ENUM(Ending)

    enum class Transfer {
        NoTransfer,
        TransferRequested,
        TransferRinging,
        TransferSucceeded,
        TransferFailed,
    };
    Q_ENUM(Transfer)

    enum class Tone {
        None,
        Ringtone,
        Ringback,
        Disconnect,
        AutoAnswered,

        Engaged,
    };
    Q_ENUM(Tone)

    enum class Redial {

        Running,

        Answered,

        Stopped,

        Pointless,

        Exhausted,
    };
    Q_ENUM(Redial)

    using Dispatcher = std::function<void(std::function<void()>)>;

    using Clock = std::function<std::chrono::steady_clock::time_point()>;

    CallViewModel(core::CallController &calls, core::RedialController &redial, Dispatcher dispatch,
                  Clock clock = {}, QObject *parent = nullptr);
    ~CallViewModel() override;

    [[nodiscard]] bool recording() const noexcept
    {
        return recordingOfTheCard() != nullptr;
    }

    [[nodiscard]] bool recordingPaused() const noexcept
    {
        const auto *mine = recordingOfTheCard();
        return mine != nullptr && mine->paused;
    }

    [[nodiscard]] bool recordingKeeping() const noexcept
    {
        const auto *mine = recordingOfTheCard();
        return mine != nullptr && mine->keeping;
    }

    [[nodiscard]] bool anyRecording() const noexcept
    {
        return !recordings_.empty();
    }

    [[nodiscard]] int recordingCount() const noexcept
    {
        return static_cast<int>(recordings_.size());
    }

    [[nodiscard]] int recordingsShown() const noexcept
    {
        return recordingCount();
    }

    [[nodiscard]] bool recordingShown() const noexcept
    {
        return recording();
    }

    [[nodiscard]] bool canRecord() const noexcept;
    [[nodiscard]] bool canStartRecording() const noexcept;

    [[nodiscard]] const core::RecordingController::State *recordingOfTheCard() const noexcept;

    [[nodiscard]] bool recordingCovers(const std::string &callId) const noexcept;

    [[nodiscard]] bool askingToRecord() const noexcept;

    void askAboutRecording(const QString &callId);

    Q_INVOKABLE void answerRecording(bool yes);

    Q_INVOKABLE void toggleDoNotDisturb();

    [[nodiscard]] bool doNotDisturb() const noexcept
    {
        return calls_.doNotDisturb();
    }

    [[nodiscard]] QString lastDialled() const
    {
        return QString::fromStdString(calls_.lastDialled());
    }

    Q_INVOKABLE void offerLastDialled();

    void noticeTheLastNumber();

    void forgetTheQuestion(const QString &callId);

    [[nodiscard]] bool busy() const noexcept;
    [[nodiscard]] bool hasCall() const noexcept;
    [[nodiscard]] bool ended() const noexcept;
    [[nodiscard]] Ending ending() const noexcept;

    static constexpr std::chrono::milliseconds kEndedLinger{3000};

    [[nodiscard]] std::chrono::milliseconds endedLinger() const noexcept
    {
        return endedLinger_;
    }

    void setEndedLinger(std::chrono::milliseconds linger)
    {
        endedLinger_ = linger;
    }

    [[nodiscard]] State state() const noexcept;
    [[nodiscard]] Direction direction() const noexcept;
    [[nodiscard]] QString remoteLabel() const;
    [[nodiscard]] QString currentCallId() const;
    [[nodiscard]] QString accountId() const;
    [[nodiscard]] QString remoteDetail() const;
    [[nodiscard]] bool muted() const noexcept;

    [[nodiscard]] bool incoming() const noexcept;

    [[nodiscard]] bool talking() const noexcept;
    [[nodiscard]] bool canSendDtmf() const noexcept;
    [[nodiscard]] bool held() const noexcept;
    [[nodiscard]] bool heldByThem() const noexcept;
    [[nodiscard]] bool inConference() const noexcept;
    [[nodiscard]] QVariantList conferenceList() const;
    [[nodiscard]] bool canMerge() const;
    [[nodiscard]] QString mergePartner() const;
    [[nodiscard]] bool offeringWhileBusy() const noexcept;
    [[nodiscard]] QString offeredLabel() const;
    [[nodiscard]] QString offeredDetail() const;
    [[nodiscard]] QVariantList callList() const;
    [[nodiscard]] bool moreThanTheCard() const;
    [[nodiscard]] qint64 connectedAtMs() const;
    [[nodiscard]] qint64 endedAtMs() const;
    [[nodiscard]] qint64 startedAtMs() const;
    [[nodiscard]] qint64 nowMs() const;

    [[nodiscard]] bool redialling() const noexcept
    {
        return campaign_.running;
    }

    [[nodiscard]] QString redialNumber() const;

    [[nodiscard]] int redialAttempts() const noexcept
    {
        return campaign_.attempts;
    }

    [[nodiscard]] qint64 redialNextAtMs() const;
    [[nodiscard]] qint64 redialGiveUpAtMs() const;

    [[nodiscard]] bool redialWaiting() const noexcept
    {
        return campaign_.waitingForTheLine;
    }

    [[nodiscard]] Redial redialEnded() const noexcept;
    [[nodiscard]] bool canRedial() const;

    void applyRedial(const core::RedialController::State &state);

    [[nodiscard]] QString dialText() const
    {
        return dialText_;
    }

    void setDialText(const QString &value);

    [[nodiscard]] Q_INVOKABLE QString dialable(const QString &typed) const;

    [[nodiscard]] bool canDial() const;

    [[nodiscard]] bool handedOverAndGone(const core::CallInfo &call) const noexcept;

    [[nodiscard]] Transfer transfer() const;
    [[nodiscard]] QString transferTarget() const;
    [[nodiscard]] int transferStatus() const;
    [[nodiscard]] bool transferUnanswered() const;
    [[nodiscard]] bool canTransfer() const;
    [[nodiscard]] QVariantList transferTargets() const;
    [[nodiscard]] QString referredBy() const;

    void setAccount(const core::AccountConfig &account);

    [[nodiscard]] const std::vector<core::CallInfo> &knownCalls() const noexcept
    {
        return calls_mirror_;
    }

    void apply(const core::CallInfo &call);

public Q_SLOTS:
    void dial();

    void answer();

    void answerEndingOthers();

    void reject();
    void hangUp();
    void toggleMute();

    void toggleHold();

    void toggleHoldCall(const QString &callId);

    void select(const QString &callId);

    void sendDtmf(const QString &digit);

    void startRedial();
    void cancelRedial();

    void merge();

    void split();

    void rejectCall(const QString &callId);

    void hangUpCall(const QString &callId);

    void hangUpConference();

    void beginTransfer();

    void cancelTransfer();

    void transferNow(const QString &target);

    void askFirst(const QString &target);

    void transferToCall(const QString &callId);

    void setContacts(std::vector<core::Contact> contacts);

    void setLines(std::vector<core::AccountConfig> lines);

    void toggleRecording();

    void tick();

public:

    void setRecordingStates(std::vector<core::RecordingController::State> states);

    void setRecordingActions(std::function<void(const QString &callId)> start,
                             std::function<void(const QString &callId, bool paused)> pause);

    void refuseRecording(const QString &callId, core::EncoderStatus why);

    enum class Refusal {

        NoRoom,

        Unknown,
    };
    Q_ENUM(Refusal)

Q_SIGNALS:
    void callChanged();

    void hasCallChanged();
    void recordingChanged();

    void recordingRefused(Refusal why);
    void nowMsChanged();
    void dialTextChanged();
    void doNotDisturbChanged();
    void lastDialledChanged();

    void ringingStarted();
    void ringingStopped();

    void toneWanted(Tone tone);

    void redialChanged();

    void redialGotThrough();

    void dtmfPressed(QString digit);

private:
    core::CallController &calls_;

    QString lastDialledWas_;
    core::RedialController &redial_;
    Dispatcher dispatch_;

    std::vector<core::RecordingController::State> recordings_;

    std::function<void(const QString &)> startRecording_;
    std::function<void(const QString &, bool)> pauseRecording_;

    std::vector<QString> asked_;
    core::AccountConfig account_;

    std::vector<core::CallInfo> calls_mirror_;
    core::ContactBook contacts_;
    std::vector<core::AccountConfig> lines_;

    std::map<std::string, std::string> saidBySwitch_;

    std::set<std::string> announcedAnswerOf_;

    std::map<std::string, std::chrono::steady_clock::time_point> endedAt_;

    std::string selected_;

    mutable std::string showing_;

    [[nodiscard]] const core::CallInfo *foreground() const noexcept;
    [[nodiscard]] const core::CallInfo *offered() const noexcept;

    void announceCallChanged();

    bool hasCallAnnounced_{false};

    struct Explanation {
        std::chrono::steady_clock::time_point at;
        bool faded{false};
    };

    std::map<std::string, Explanation> explainedAt_;

    bool expireExplanations();

    bool expireEnded();

    std::set<std::string> routedByTheSwitch_;

    [[nodiscard]] bool carriedOnHere(const core::CallInfo &call) const noexcept;

    [[nodiscard]] const core::AccountConfig &lineOf(const core::CallInfo &call) const noexcept;

    void forget(const std::string &callId);

    void announceTone(const core::CallInfo &call);

    Clock clock_;
    std::chrono::milliseconds endedLinger_{kEndedLinger};
    QString dialText_;
    bool ringing_{false};

    core::CallTone tone_{core::CallTone::None};

    std::string startedRedialFrom_;

    core::RedialController::State campaign_;
};

[[nodiscard]] core::CallTone toCoreTone(CallViewModel::Tone tone) noexcept;

}
