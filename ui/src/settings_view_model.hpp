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

#include "global_hotkeys.hpp"
#include "link_handler.hpp"
#include "plugin_settings.hpp"

#include <softphone/core/audio_devices.hpp>
#include <softphone/core/codecs.hpp>
#include <softphone/core/level_meter.hpp>
#include <softphone/core/settings.hpp>
#include <softphone/core/tones.hpp>

#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QUrl>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

#include <chrono>
#include <functional>
#include <map>
#include <utility>
#include <vector>

namespace softphone::ui {

class SettingsViewModel : public QObject {
    Q_OBJECT

    QML_ELEMENT
    QML_UNCREATABLE("Created by the application and exposed as `settings`.")

    Q_PROPERTY(int pendingCount READ pendingCount NOTIFY pendingChanged)
    Q_PROPERTY(Schedule pendingSchedule READ pendingSchedule NOTIFY pendingChanged)
    Q_PROPERTY(bool firstRun READ firstRun NOTIFY firstRunChanged)

    Q_PROPERTY(QVariantList microphones READ microphones NOTIFY audioChanged)
    Q_PROPERTY(QVariantList speakers READ speakers NOTIFY audioChanged)
    Q_PROPERTY(QVariantList ringers READ ringers NOTIFY audioChanged)

    Q_PROPERTY(int microphoneIndex READ microphoneIndex NOTIFY audioChanged)
    Q_PROPERTY(int speakerIndex READ speakerIndex NOTIFY audioChanged)
    Q_PROPERTY(int ringerIndex READ ringerIndex NOTIFY audioChanged)

    Q_PROPERTY(QVariantList ringtones READ ringtones NOTIFY audioChanged)
    Q_PROPERTY(int ringtoneIndex READ ringtoneIndex NOTIFY audioChanged)

    Q_PROPERTY(RingProblem ringtoneProblem READ ringtoneProblem NOTIFY ringtoneProblemChanged)
    Q_PROPERTY(QString ringtoneProblemFile READ ringtoneProblemFile NOTIFY ringtoneProblemChanged)

    Q_PROPERTY(
        int microphoneVolume READ microphoneVolume WRITE setMicrophoneVolume NOTIFY volumesChanged)
    Q_PROPERTY(int speakerVolume READ speakerVolume WRITE setSpeakerVolume NOTIFY volumesChanged)
    Q_PROPERTY(int ringVolume READ ringVolume WRITE setRingVolume NOTIFY volumesChanged)
    Q_PROPERTY(int noticeVolume READ noticeVolume WRITE setNoticeVolume NOTIFY volumesChanged)

    Q_PROPERTY(bool audioIsDefault READ audioIsDefault NOTIFY audioChanged)

    Q_PROPERTY(bool microphoneMissing READ microphoneMissing NOTIFY audioChanged)
    Q_PROPERTY(bool speakersMissing READ speakersMissing NOTIFY audioChanged)
    Q_PROPERTY(bool ringerMissing READ ringerMissing NOTIFY audioChanged)

    Q_PROPERTY(double microphoneLevel READ microphoneLevel NOTIFY levelChanged)

    Q_PROPERTY(bool canTest READ canTest NOTIFY canTestChanged)

    Q_PROPERTY(bool ringPlaying READ ringPlaying NOTIFY ringPlayingChanged)

    Q_PROPERTY(QVariantList codecs READ codecs NOTIFY codecsChanged)

    Q_PROPERTY(int offeredCodecCount READ offeredCodecCount NOTIFY codecsChanged)

    Q_PROPERTY(int redialIntervalSeconds READ redialIntervalSeconds NOTIFY redialSettingsChanged)
    Q_PROPERTY(int redialGiveUpMinutes READ redialGiveUpMinutes NOTIFY redialSettingsChanged)

    Q_PROPERTY(CallWaiting callWaiting READ callWaiting NOTIFY redialSettingsChanged)

    Q_PROPERTY(int historyDays READ historyDays NOTIFY redialSettingsChanged)
    Q_PROPERTY(int historyDaysLimit READ historyDaysLimit CONSTANT)
    Q_PROPERTY(int minRedialIntervalSeconds READ minRedialIntervalSeconds CONSTANT)
    Q_PROPERTY(int maxRedialIntervalSeconds READ maxRedialIntervalSeconds CONSTANT)
    Q_PROPERTY(int minRedialGiveUpMinutes READ minRedialGiveUpMinutes CONSTANT)
    Q_PROPERTY(int maxRedialGiveUpMinutes READ maxRedialGiveUpMinutes CONSTANT)

    Q_PROPERTY(QString language READ language NOTIFY appearanceChanged)

    Q_PROPERTY(QVariantList languages READ languages CONSTANT)

    Q_PROPERTY(QVariantList hotkeys READ hotkeys NOTIFY hotkeysChanged)

    Q_PROPERTY(bool globalSupported READ globalSupported NOTIFY hotkeysChanged)

    Q_PROPERTY(
        bool capturingKeys READ capturingKeys WRITE setCapturingKeys NOTIFY capturingKeysChanged)

    Q_PROPERTY(bool closeToTray READ closeToTray NOTIFY startupChanged)
    Q_PROPERTY(bool autostart READ autostart NOTIFY startupChanged)
    Q_PROPERTY(bool startMinimised READ startMinimised NOTIFY startupChanged)
    Q_PROPERTY(bool fromAnImage READ fromAnImage NOTIFY startupChanged)
    Q_PROPERTY(bool menuEntryWanted READ menuEntryWanted NOTIFY startupChanged)
    Q_PROPERTY(bool menuEntryUnanswered READ menuEntryUnanswered NOTIFY startupChanged)

    Q_PROPERTY(bool callLinkDialsAtOnce READ callLinkDialsAtOnce NOTIFY startupChanged)

    Q_PROPERTY(bool linksPossible READ linksPossible NOTIFY linksChanged)
    Q_PROPERTY(bool linksOurs READ linksOurs NOTIFY linksChanged)
    Q_PROPERTY(QString linksHeldBy READ linksHeldBy NOTIFY linksChanged)

    Q_PROPERTY(Layout layout READ layout NOTIFY appearanceChanged)
    Q_PROPERTY(Layout runningLayout READ runningLayout CONSTANT)

    Q_PROPERTY(bool layoutAwaitsARestart READ layoutAwaitsARestart NOTIFY appearanceChanged)

    Q_PROPERTY(QString barScreen READ barScreen NOTIFY appearanceChanged)

    Q_PROPERTY(Theme theme READ theme NOTIFY appearanceChanged)

    Q_PROPERTY(bool darkWanted READ darkWanted NOTIFY darkWantedChanged)

    Q_PROPERTY(Notice callNotice READ callNotice NOTIFY appearanceChanged)

    Q_PROPERTY(Glyphs glyphs READ glyphs NOTIFY appearanceChanged)

    Q_PROPERTY(QString glyphSet READ glyphSet NOTIFY appearanceChanged)

    Q_PROPERTY(Sketch sketch READ sketch NOTIFY appearanceChanged)

    Q_PROPERTY(int endedCallSeconds READ endedCallSeconds NOTIFY appearanceChanged)
    Q_PROPERTY(IconScale iconScale READ iconScale NOTIFY appearanceChanged)

    Q_PROPERTY(Verbosity logLevel READ logLevel NOTIFY diagnosticsChanged)

    Q_PROPERTY(bool systemLog READ systemLog NOTIFY diagnosticsChanged)

    Q_PROPERTY(QString systemLogCommand READ systemLogCommand CONSTANT)

    Q_PROPERTY(bool systemLogAvailable READ systemLogAvailable CONSTANT)

    Q_PROPERTY(QString logPath READ logPath NOTIFY pathsChanged)
    Q_PROPERTY(QString configPath READ configPath NOTIFY pathsChanged)
    Q_PROPERTY(QString databasePath READ databasePath NOTIFY pathsChanged)

    Q_PROPERTY(QString recordingsPath READ recordingsPath NOTIFY recordingChanged)

    Q_PROPERTY(
        int recordingPolicy READ recordingPolicy WRITE setRecordingPolicy NOTIFY recordingChanged)
    Q_PROPERTY(int recordingConsent READ recordingConsent WRITE setRecordingConsent NOTIFY
                   recordingChanged)
    Q_PROPERTY(QString recordingAnnouncement READ recordingAnnouncement WRITE
                   setRecordingAnnouncement NOTIFY recordingChanged)
    Q_PROPERTY(int recordingToneInterval READ recordingToneInterval WRITE setRecordingToneInterval
                   NOTIFY recordingChanged)
    Q_PROPERTY(bool recordingKeepStems READ recordingKeepStems WRITE setRecordingKeepStems NOTIFY
                   recordingChanged)
    Q_PROPERTY(bool recordingKeepNotices READ recordingKeepNotices WRITE setRecordingKeepNotices
                   NOTIFY recordingChanged)

    Q_PROPERTY(QString recordingLibrary READ recordingLibrary NOTIFY recordingChanged)

    Q_PROPERTY(bool recordingSilently READ recordingSilently NOTIFY recordingChanged)

    Q_PROPERTY(int recordingToneMinimum READ recordingToneMinimum CONSTANT)
    Q_PROPERTY(int recordingToneMaximum READ recordingToneMaximum CONSTANT)

    Q_PROPERTY(int recordingMaxAgeDays READ recordingMaxAgeDays WRITE setRecordingMaxAgeDays NOTIFY
                   recordingChanged)
    Q_PROPERTY(int recordingMaxSize READ recordingMaxSize WRITE setRecordingMaxSize NOTIFY
                   recordingChanged)
    Q_PROPERTY(int recordingStemDays READ recordingStemDays WRITE setRecordingStemDays NOTIFY
                   recordingChanged)
    Q_PROPERTY(int recordingFreeSpaceFloor READ recordingFreeSpaceFloor WRITE
                   setRecordingFreeSpaceFloor NOTIFY recordingChanged)

    Q_PROPERTY(int recordingMaxAgeLimit READ recordingMaxAgeLimit CONSTANT)
    Q_PROPERTY(int recordingMaxSizeLimit READ recordingMaxSizeLimit CONSTANT)
    Q_PROPERTY(int recordingFloorMinimum READ recordingFloorMinimum CONSTANT)
    Q_PROPERTY(int recordingFloorMaximum READ recordingFloorMaximum CONSTANT)

    Q_PROPERTY(bool transcribeAutomatically READ transcribeAutomatically WRITE
                   setTranscribeAutomatically NOTIFY transcriptionChanged)

    Q_PROPERTY(QString transcriptionLanguage READ transcriptionLanguage WRITE
                   setTranscriptionLanguage NOTIFY transcriptionChanged)

    Q_PROPERTY(bool captureAvailable READ captureAvailable NOTIFY captureStateChanged)
    Q_PROPERTY(bool captureWatchAvailable READ captureWatchAvailable NOTIFY captureStateChanged)

    Q_PROPERTY(
        bool captureEnabled READ captureEnabled WRITE setCaptureEnabled NOTIFY captureChanged)

    Q_PROPERTY(OnFinding captureOnFinding READ captureOnFinding WRITE setCaptureOnFinding NOTIFY
                   captureChanged)

    Q_PROPERTY(int capturePreRoll READ capturePreRoll WRITE setCapturePreRoll NOTIFY captureChanged)
    Q_PROPERTY(int maxCapturePreRoll READ maxCapturePreRoll CONSTANT)

    Q_PROPERTY(bool captureRemind READ captureRemind WRITE setCaptureRemind NOTIFY captureChanged)

    Q_PROPERTY(QStringList captureExceptions READ captureExceptions NOTIFY captureChanged)

    Q_PROPERTY(double captureNearLevel READ captureNearLevel NOTIFY captureLevelChanged)
    Q_PROPERTY(double captureFarLevel READ captureFarLevel NOTIFY captureLevelChanged)

    Q_PROPERTY(Sketch captureSketch READ captureSketch NOTIFY captureChanged)
    Q_PROPERTY(Sketch captureStripSketch READ captureStripSketch NOTIFY captureChanged)

    Q_PROPERTY(bool captureRunning READ captureRunning NOTIFY captureStateChanged)

    Q_PROPERTY(bool captureHeldOff READ captureHeldOff NOTIFY captureStateChanged)
    Q_PROPERTY(QString captureWhat READ captureWhat NOTIFY captureStateChanged)

    Q_PROPERTY(bool processAutomatically READ processAutomatically WRITE setProcessAutomatically
                   NOTIFY pipelineChanged)

    Q_PROPERTY(
        QString monthlyCeiling READ monthlyCeiling WRITE setMonthlyCeiling NOTIFY pipelineChanged)

    Q_PROPERTY(QString monthlyTokenCeiling READ monthlyTokenCeiling WRITE setMonthlyTokenCeiling
                   NOTIFY pipelineChanged)

    Q_PROPERTY(QString monthSoFar READ monthSoFar NOTIFY spendChanged)
    Q_PROPERTY(bool processingPaused READ processingPaused NOTIFY spendChanged)

    Q_PROPERTY(
        bool prompterOffered READ prompterOffered WRITE setPrompterOffered NOTIFY realtimeChanged)

    Q_PROPERTY(QString realtimeRecogniserCeiling READ realtimeRecogniserCeiling WRITE
                   setRealtimeRecogniserCeiling NOTIFY realtimeChanged)

    Q_PROPERTY(QString realtimeModelCeiling READ realtimeModelCeiling WRITE setRealtimeModelCeiling
                   NOTIFY realtimeChanged)

    Q_PROPERTY(int prompterColumnText READ prompterColumnText WRITE setPrompterColumnText NOTIFY
                   realtimeChanged)

    Q_PROPERTY(bool prompterRepeatsTheLast READ prompterRepeatsTheLast WRITE
                   setPrompterRepeatsTheLast NOTIFY realtimeChanged)

    Q_PROPERTY(int prompterRepeatText READ prompterRepeatText WRITE setPrompterRepeatText NOTIFY
                   realtimeChanged)

    Q_PROPERTY(int leastPrompterColumnText READ leastPrompterColumnText CONSTANT)
    Q_PROPERTY(int mostPrompterColumnText READ mostPrompterColumnText CONSTANT)
    Q_PROPERTY(int leastPrompterRepeatText READ leastPrompterRepeatText CONSTANT)
    Q_PROPERTY(int mostPrompterRepeatText READ mostPrompterRepeatText CONSTANT)

    Q_PROPERTY(bool webhooksEnabled READ webhooksEnabled NOTIFY webhooksChanged)
    Q_PROPERTY(QString webhookUrl READ webhookUrl NOTIFY webhooksChanged)

    Q_PROPERTY(bool webhookPost READ webhookPost NOTIFY webhooksChanged)

    Q_PROPERTY(int webhookRevision READ webhookRevision NOTIFY webhooksChanged)

    Q_PROPERTY(QString webhookTemplate READ webhookTemplate NOTIFY webhooksChanged)
    Q_PROPERTY(QString webhookHeaderName READ webhookHeaderName NOTIFY webhooksChanged)

    Q_PROPERTY(QString postName READ postName CONSTANT)
    Q_PROPERTY(QString getName READ getName CONSTANT)

    Q_PROPERTY(QStringList webhookPlaceholders READ webhookPlaceholders CONSTANT)

    Q_PROPERTY(
        QStringList webhookTemplateProblems READ webhookTemplateProblems NOTIFY webhooksChanged)

    Q_PROPERTY(bool webhookTokenStored READ webhookTokenStored NOTIFY webhookTokenChanged)

    Q_PROPERTY(bool apiEnabled READ apiEnabled NOTIFY apiChanged)

    Q_PROPERTY(bool automaticUpdates READ automaticUpdates NOTIFY updatesChanged)
    Q_PROPERTY(int apiPort READ apiPort NOTIFY apiChanged)

    Q_PROPERTY(int apiRevision READ apiRevision NOTIFY apiChanged)

    Q_PROPERTY(bool apiTokenStored READ apiTokenStored NOTIFY apiTokenChanged)

    Q_PROPERTY(QString apiAddress READ apiAddress NOTIFY apiChanged)

    Q_PROPERTY(qint64 logBytes READ logBytes NOTIFY logChanged)

    Q_PROPERTY(qint64 configBytes READ configBytes NOTIFY pathsChanged)
    Q_PROPERTY(qint64 databaseBytes READ databaseBytes NOTIFY pathsChanged)

public:

    enum class Schedule {
        Now,
        NextCall,
        WhenCallsEnd,
        NextStart,
    };
    Q_ENUM(Schedule)

    enum class OnFinding {
        Never,
        Ask,
        Record,
    };
    Q_ENUM(OnFinding)

    enum class Theme {
        Default,
        Light,
        Dark,
    };
    Q_ENUM(Theme)

    enum class Layout {
        Windows,
        Compact,
        Workspace,
        WorkspaceAndPhone,
        Bar,
        NoWindow,
    };
    Q_ENUM(Layout)

    enum class Notice {
        Banner,
        Raise,
        Silent,
    };
    Q_ENUM(Notice)

    enum class CallWaiting {
        Allowed,
        Busy,
    };
    Q_ENUM(CallWaiting)

    enum class Glyphs {
        Solid,
        Outline,
    };
    Q_ENUM(Glyphs)

    enum class IconScale {
        Normal,
        Large,
    };
    Q_ENUM(IconScale)

    enum class Verbosity {
        Error,
        Warning,
        Info,
        Debug,
        Trace,
    };
    Q_ENUM(Verbosity)

    enum class Action {
        Answer,
        HangUp,
        HoldResume,
        Mute,
        ShowWindow,
    };
    Q_ENUM(Action)

    enum class KeyProblem {
        None,
        Unreadable,
        NoModifier,
    };
    Q_ENUM(KeyProblem)

    enum class Grab {
        Working,
        Unsupported,
        Taken,

        NoSuchKey,
    };
    Q_ENUM(Grab)

    enum class Sketch {
        Nothing,
        Levels,
        Histogram,
        Wave,
        Spectrum,
        Aurora,
        Halo,
        Uprights,
    };
    Q_ENUM(Sketch)

    enum class RingProblem {
        Playable,
        NoSuchFile,
        NotAFile,
        Locked,
        NotAWave,
        WrongFormat,
        NoAudio,
    };
    Q_ENUM(RingProblem)

    using Dispatcher = std::function<void(std::function<void()>)>;

    using WithdrawCallback = std::function<void()>;

    using AudioCallback = std::function<void(core::AudioSettings)>;

    struct AudioProbe {

        std::function<void(core::ToneOutput, std::string, core::ToneLevel)> play;

        std::function<void()> stop;

        std::function<void(bool)> meter;

        std::function<void()> sample;
    };

    using CodecsCallback = std::function<void(std::vector<core::CodecChoice>)>;

    using CallsCallback = std::function<void(int intervalSeconds, int giveUpMinutes,
                                             int historyDays, core::CallWaiting waiting)>;

    using AppearanceCallback = std::function<void(core::AppearanceSettings)>;

    using RefreshCallback = std::function<void()>;

    using StartupCallback = std::function<void(core::StartupSettings)>;

    using HotkeysCallback = std::function<void(core::HotkeySettings)>;

    using DiagnosticsCallback = std::function<void(core::DiagnosticsSettings)>;

    void setLanguages(QStringList codes);

    void setDiagnosticsCallback(DiagnosticsCallback choose);

    using WebhooksCallback = std::function<void(core::WebhookSettings)>;
    void setWebhooksCallback(WebhooksCallback choose);

    using RecordingCallback = std::function<void(core::RecordingSettings)>;
    void setRecordingCallback(RecordingCallback choose);

    using TranscriptionCallback = std::function<void(ui::TranscriptionSettings)>;
    void setTranscriptionCallback(TranscriptionCallback choose);

    using PipelineCallback = std::function<void(ui::PipelineSettings)>;
    void setPipelineCallback(PipelineCallback choose);

    using RealtimeCallback = std::function<void(ui::RealtimeSettings)>;
    void setRealtimeCallback(RealtimeCallback choose);

    using CaptureCallback = std::function<void(ui::CaptureSettings)>;
    void setCaptureCallback(CaptureCallback choose);

    using CaptureCommand = std::function<bool(bool start, QString what, QString process)>;
    void setCaptureCommand(CaptureCommand run);

    using CaptureElapsed = std::function<int()>;
    void setCaptureElapsed(CaptureElapsed ask);

    using CaptureLevelSource = std::function<std::pair<unsigned, unsigned>()>;
    void setCaptureLevelSource(CaptureLevelSource take);

    using CaptureDrawingCallback = std::function<void(core::Visualisation)>;
    void setCaptureDrawingCallback(CaptureDrawingCallback draw);

    using CaptureRename = std::function<bool(const QString &what)>;
    void setCaptureRename(CaptureRename run);

    using CaptureNotACall = std::function<bool(const QString &recordingId)>;
    void setCaptureNotACall(CaptureNotACall run);

    void applyCapturePolicies(std::vector<plugins::capture::ApplicationPolicy> policies);

    void sayFound(const QString &what, const QString &process);
    void sayLost();
    void setCallsCallback(CallsCallback choose);

    void setPaths(QString log, QString config, QString database);

    void setRunningLayout(core::Layout layout);

    explicit SettingsViewModel(Dispatcher dispatch, WithdrawCallback withdraw,
                               AudioCallback chooseAudio = {}, RefreshCallback refresh = {},
                               AudioProbe probe = {}, CodecsCallback chooseCodecs = {},
                               AppearanceCallback chooseAppearance = {},
                               StartupCallback chooseStartup = {},
                               HotkeysCallback chooseHotkeys = {}, QObject *parent = nullptr);

    [[nodiscard]] int pendingCount() const noexcept
    {
        return static_cast<int>(pending_.size());
    }

    [[nodiscard]] Schedule pendingSchedule() const noexcept;

    [[nodiscard]] bool firstRun() const noexcept
    {
        return firstRun_;
    }

    void setFirstRun(bool value);

    [[nodiscard]] QVariantList microphones() const;
    [[nodiscard]] QVariantList speakers() const;
    [[nodiscard]] QVariantList ringers() const;
    [[nodiscard]] QVariantList ringtones() const;
    [[nodiscard]] int ringtoneIndex() const;

    [[nodiscard]] RingProblem ringtoneProblem() const noexcept
    {
        return ringtoneProblem_;
    }

    [[nodiscard]] QString ringtoneProblemFile() const
    {
        return ringtoneProblemFile_;
    }

    [[nodiscard]] int microphoneVolume() const noexcept
    {
        return volumes_.microphone;
    }

    [[nodiscard]] int speakerVolume() const noexcept
    {
        return volumes_.speakers;
    }

    [[nodiscard]] int ringVolume() const noexcept
    {
        return volumes_.ring;
    }

    [[nodiscard]] int noticeVolume() const noexcept
    {
        return volumes_.notifications;
    }

    void setMicrophoneVolume(int percent);
    void setSpeakerVolume(int percent);
    void setRingVolume(int percent);
    void setNoticeVolume(int percent);

    static constexpr std::chrono::milliseconds kVolumeSettling{250};

    [[nodiscard]] bool audioIsDefault() const noexcept
    {
        const core::AudioSettings fresh;
        return audio_ == fresh && volumes_ == fresh.volumes;
    }

    [[nodiscard]] const core::RingtoneChoice &ringtone() const noexcept
    {
        return audio_.ringtone;
    }

    [[nodiscard]] const core::RecordingSettings &recordingSettings() const noexcept
    {
        return recording_;
    }

    [[nodiscard]] bool processAutomatically() const noexcept
    {
        return pipeline_.automatic;
    }

    [[nodiscard]] bool captureAvailable() const noexcept
    {
        return captureAvailable_;
    }

    [[nodiscard]] bool captureWatchAvailable() const noexcept
    {
        return captureWatching_;
    }

    [[nodiscard]] Q_INVOKABLE static QVariantList sketchesInARow();

    [[nodiscard]] bool captureEnabled() const noexcept
    {
        return capture_.enabled;
    }

    [[nodiscard]] OnFinding captureOnFinding() const noexcept;

    [[nodiscard]] static int maxCapturePreRoll() noexcept
    {
        return static_cast<int>(kMaxCapturePreRollSeconds);
    }

    [[nodiscard]] int capturePreRoll() const noexcept
    {
        return static_cast<int>(capture_.preRollSeconds);
    }

    [[nodiscard]] bool captureRemind() const noexcept
    {
        return capture_.remind;
    }

    [[nodiscard]] bool captureRunning() const noexcept
    {
        return captureRunning_;
    }

    [[nodiscard]] double captureNearLevel() const noexcept
    {
        return captureNear_.reading();
    }

    [[nodiscard]] double captureFarLevel() const noexcept
    {
        return captureFar_.reading();
    }

    [[nodiscard]] QString captureWhat() const
    {
        return captureWhat_;
    }

    [[nodiscard]] QString monthlyCeiling() const;
    [[nodiscard]] QString monthlyTokenCeiling() const;

    [[nodiscard]] QString monthSoFar() const
    {
        return monthSoFar_;
    }

    [[nodiscard]] bool processingPaused() const noexcept
    {
        return paused_;
    }

    [[nodiscard]] bool transcribeAutomatically() const noexcept
    {
        return transcription_.automatic;
    }

    [[nodiscard]] QString transcriptionLanguage() const
    {
        return QString::fromStdString(transcription_.language);
    }

    [[nodiscard]] const ui::TranscriptionSettings &transcription() const noexcept
    {
        return transcription_;
    }

    [[nodiscard]] int recordingPolicy() const noexcept
    {
        return static_cast<int>(recording_.policy);
    }

    [[nodiscard]] int recordingConsent() const noexcept
    {
        return static_cast<int>(recording_.consent);
    }

    [[nodiscard]] QString recordingAnnouncement() const
    {
        return QString::fromStdString(recording_.announcement);
    }

    Q_INVOKABLE [[nodiscard]] bool recordsAccount(const QString &accountId) const;
    Q_INVOKABLE void setRecordsAccount(const QString &accountId, bool records);

    Q_INVOKABLE void chooseAnnouncementFile(const QUrl &url);
    Q_INVOKABLE void useTheShippedChime();

    [[nodiscard]] int recordingToneInterval() const noexcept
    {
        return recording_.toneIntervalSeconds;
    }

    [[nodiscard]] bool recordingKeepStems() const noexcept
    {
        return recording_.keepStems;
    }

    [[nodiscard]] bool recordingKeepNotices() const noexcept
    {
        return recording_.keepNotices;
    }

    [[nodiscard]] QString recordingLibrary() const;

    [[nodiscard]] bool recordingSilently() const noexcept
    {
        return recording_.consent == core::ConsentPosture::Silent;
    }

    [[nodiscard]] static int recordingToneMinimum() noexcept
    {
        return core::kMinToneInterval;
    }

    [[nodiscard]] static int recordingToneMaximum() noexcept
    {
        return core::kMaxToneInterval;
    }

    [[nodiscard]] int recordingMaxAgeDays() const noexcept
    {
        return recording_.retention.maxAgeDays;
    }

    [[nodiscard]] int recordingMaxSize() const noexcept
    {
        return recording_.retention.maxSizeMegabytes;
    }

    [[nodiscard]] int recordingStemDays() const noexcept
    {
        return recording_.retention.dropStemsAfterDays;
    }

    [[nodiscard]] int recordingFreeSpaceFloor() const noexcept
    {
        return recording_.retention.freeSpaceFloorMegabytes;
    }

    [[nodiscard]] static int recordingMaxAgeLimit() noexcept
    {
        return core::kMaxRetentionDays;
    }

    [[nodiscard]] static int recordingMaxSizeLimit() noexcept
    {
        return core::kMaxLibraryMegabytes;
    }

    [[nodiscard]] static int recordingFloorMinimum() noexcept
    {
        return core::kMinFreeSpaceMegabytes;
    }

    [[nodiscard]] static int recordingFloorMaximum() noexcept
    {
        return core::kMaxFreeSpaceMegabytes;
    }

    void setRecordingMaxAgeDays(int days);
    void setRecordingMaxSize(int megabytes);
    void setRecordingStemDays(int days);
    void setRecordingFreeSpaceFloor(int megabytes);

    void setRecordingPolicy(int policy);
    void setRecordingConsent(int consent);
    void setRecordingAnnouncement(const QString &file);
    void setRecordingToneInterval(int seconds);

    void applyRecording(const core::RecordingSettings &recording);
    void applyTranscription(const ui::TranscriptionSettings &transcription);
    void applyPipeline(const ui::PipelineSettings &pipeline);
    void applyRealtime(const ui::RealtimeSettings &realtime);

    [[nodiscard]] bool prompterOffered() const noexcept
    {
        return realtime_.offered;
    }

    [[nodiscard]] QString realtimeRecogniserCeiling() const;
    [[nodiscard]] QString realtimeModelCeiling() const;

    [[nodiscard]] int prompterColumnText() const noexcept
    {
        return realtime_.columnTextPixels;
    }

    [[nodiscard]] bool prompterRepeatsTheLast() const noexcept
    {
        return realtime_.repeatTheLast;
    }

    [[nodiscard]] int prompterRepeatText() const noexcept
    {
        return realtime_.repeatTextPixels;
    }

    [[nodiscard]] static int leastPrompterColumnText() noexcept
    {
        return ui::kLeastColumnTextPixels;
    }

    [[nodiscard]] static int mostPrompterColumnText() noexcept
    {
        return ui::kMostColumnTextPixels;
    }

    [[nodiscard]] static int leastPrompterRepeatText() noexcept
    {
        return ui::kLeastRepeatTextPixels;
    }

    [[nodiscard]] static int mostPrompterRepeatText() noexcept
    {
        return ui::kMostRepeatTextPixels;
    }

    void applyCapture(const ui::CaptureSettings &capture);

    void applyCaptureState(bool available, bool watching, bool running, const QString &what,
                           bool heldOff);

    void applyCaptureLevels(unsigned near, unsigned far);

    void applySpend(const QString &line, bool paused);

    [[nodiscard]] int microphoneIndex() const;
    [[nodiscard]] int speakerIndex() const;
    [[nodiscard]] int ringerIndex() const;

    [[nodiscard]] bool microphoneMissing() const noexcept
    {
        return result_.microphone.use == core::DeviceUse::Missing;
    }

    [[nodiscard]] bool speakersMissing() const noexcept
    {
        return result_.speakers.use == core::DeviceUse::Missing;
    }

    [[nodiscard]] bool ringerMissing() const noexcept
    {
        return result_.ringer.use == core::DeviceUse::Missing;
    }

    [[nodiscard]] bool captureHeldOff() const noexcept
    {
        return captureHeldOff_;
    }

    [[nodiscard]] double microphoneLevel() const noexcept
    {
        return meter_.reading();
    }

    [[nodiscard]] bool canTest() const noexcept
    {
        return canTest_;
    }

    [[nodiscard]] bool ringPlaying() const noexcept
    {
        return ringPlaying_;
    }

    void reportToneEnded();

    void setCanTest(bool allowed);

    void reportLevel(unsigned level);

    [[nodiscard]] QVariantList codecs() const;

    [[nodiscard]] int offeredCodecCount() const noexcept;

    void applyCodecs(std::vector<core::CodecInfo> available, std::vector<core::CodecChoice> chosen);

    void applyCallSettings(int intervalSeconds, int giveUpMinutes, int historyDays,
                           core::CallWaiting waiting);

    [[nodiscard]] int redialIntervalSeconds() const noexcept
    {
        return redialIntervalSeconds_;
    }

    [[nodiscard]] int redialGiveUpMinutes() const noexcept
    {
        return redialGiveUpMinutes_;
    }

    [[nodiscard]] CallWaiting callWaiting() const noexcept
    {
        return callWaiting_ == core::CallWaiting::Busy ? CallWaiting::Busy : CallWaiting::Allowed;
    }

    [[nodiscard]] int historyDays() const noexcept
    {
        return historyDays_;
    }

    [[nodiscard]] static int historyDaysLimit() noexcept
    {
        return core::kMaxHistoryDays;
    }

    [[nodiscard]] static int minRedialIntervalSeconds() noexcept
    {
        return core::kMinRedialIntervalSeconds;
    }

    [[nodiscard]] static int maxRedialIntervalSeconds() noexcept
    {
        return core::kMaxRedialIntervalSeconds;
    }

    [[nodiscard]] static int minRedialGiveUpMinutes() noexcept
    {
        return core::kMinRedialGiveUpMinutes;
    }

    [[nodiscard]] static int maxRedialGiveUpMinutes() noexcept
    {
        return core::kMaxRedialGiveUpMinutes;
    }

    Q_INVOKABLE void chooseRedialInterval(int seconds);
    Q_INVOKABLE void chooseRedialGiveUp(int minutes);
    Q_INVOKABLE void chooseHistoryDays(int days);

    Q_INVOKABLE void chooseCallWaiting(CallWaiting waiting);

    [[nodiscard]] QString language() const;
    [[nodiscard]] QVariantList languages() const;
    [[nodiscard]] Theme theme() const noexcept;
    [[nodiscard]] bool darkWanted() const noexcept;

    using DesktopScheme = std::function<Qt::ColorScheme()>;
    void followTheDesktop(DesktopScheme where);

    void theDesktopChangedItsMind();
    [[nodiscard]] Notice callNotice() const noexcept;
    [[nodiscard]] Glyphs glyphs() const noexcept;
    [[nodiscard]] QString glyphSet() const;
    [[nodiscard]] Sketch sketch() const noexcept;

    [[nodiscard]] int endedCallSeconds() const noexcept
    {
        return appearance_.endedCallSeconds;
    }

    [[nodiscard]] IconScale iconScale() const noexcept;

    [[nodiscard]] bool closeToTray() const noexcept
    {
        return startup_.closeToTray;
    }

    [[nodiscard]] bool autostart() const noexcept
    {
        return startup_.autostart;
    }

    [[nodiscard]] bool startMinimised() const noexcept
    {
        return startup_.startMinimised;
    }

    [[nodiscard]] bool fromAnImage() const noexcept
    {
        return fromAnImage_;
    }

    [[nodiscard]] bool menuEntryWanted() const noexcept
    {
        return startup_.menuEntry == core::MenuEntry::Wanted;
    }

    [[nodiscard]] bool callLinkDialsAtOnce() const noexcept
    {
        return startup_.callLinkDialsAtOnce;
    }

    [[nodiscard]] bool linksPossible() const noexcept
    {
        return links_.possible;
    }

    [[nodiscard]] bool linksOurs() const noexcept
    {
        return links_.preferred;
    }

    [[nodiscard]] QString linksHeldBy() const
    {
        return QString::fromStdString(links_.heldBy);
    }

    Q_INVOKABLE void refreshLinks();

    Q_INVOKABLE void claimLinks();

    void answersLinksThrough(ILinkHandler *handler);

    [[nodiscard]] bool menuEntryUnanswered() const noexcept
    {
        return fromAnImage_ && startup_.menuEntry == core::MenuEntry::NotAsked;
    }

    void thisRunsFromAnImage(bool fromAnImage);

    void applyStartup(const core::StartupSettings &startup);

    [[nodiscard]] QVariantList hotkeys() const;

    void applyHotkeys(const core::HotkeySettings &hotkeys);

    [[nodiscard]] const std::vector<core::Hotkey> &bindings() const noexcept
    {
        return hotkeys_;
    }

    [[nodiscard]] bool globalSupported() const noexcept
    {
        return globalSupported_;
    }

    [[nodiscard]] bool capturingKeys() const noexcept
    {
        return capturingKeys_;
    }

    void applyHotkeyGrabs(std::span<const GrabOutcome> outcomes, bool supported);

    void announceHotkey(Action action);

    Q_INVOKABLE [[nodiscard]] QString sequenceFor(int key, int modifiers) const;

    Q_INVOKABLE [[nodiscard]] KeyProblem checkSequence(const QString &sequence) const;

    Q_INVOKABLE [[nodiscard]] QString shownSequence(const QString &sequence) const;

    void applyAppearance(const core::AppearanceSettings &appearance);

    [[nodiscard]] Verbosity logLevel() const noexcept;

    [[nodiscard]] bool systemLog() const noexcept
    {
        return diagnostics_.systemLog;
    }

    [[nodiscard]] static bool systemLogAvailable() noexcept;
    [[nodiscard]] static QString systemLogCommand();

    [[nodiscard]] bool webhooksEnabled() const noexcept
    {
        return webhooks_.enabled;
    }

    [[nodiscard]] QString webhookUrl() const
    {
        return QString::fromStdString(webhooks_.url);
    }

    [[nodiscard]] bool webhookPost() const noexcept
    {
        return webhooks_.method == core::HttpMethod::Post;
    }

    [[nodiscard]] Q_INVOKABLE bool webhookWants(const QString &event) const
    {
        return webhooks_.wants(event.toStdString());
    }

    [[nodiscard]] int webhookRevision() const noexcept
    {
        return webhookRevision_;
    }

    [[nodiscard]] QString webhookTemplate() const
    {
        return QString::fromStdString(webhooks_.urlTemplate);
    }

    [[nodiscard]] QString webhookHeaderName() const
    {
        return QString::fromStdString(webhooks_.headerName);
    }

    [[nodiscard]] bool apiEnabled() const noexcept
    {
        return api_.enabled;
    }

    [[nodiscard]] bool automaticUpdates() const noexcept
    {
        return updates_.automatic;
    }

    [[nodiscard]] int apiPort() const noexcept
    {
        return api_.port;
    }

    [[nodiscard]] Q_INVOKABLE bool apiServes(const QString &resource) const
    {
        return api_.serves(resource.toStdString());
    }

    [[nodiscard]] int apiRevision() const noexcept
    {
        return apiRevision_;
    }

    [[nodiscard]] bool apiTokenStored() const noexcept
    {
        return apiTokenStored_;
    }

    [[nodiscard]] QString apiAddress() const;

    void applyApi(const core::ApiSettings &api);
    void applyUpdates(const core::UpdateSettings &updates);
    void setApiTokenStored(bool stored);

    [[nodiscard]] static QString postName();
    [[nodiscard]] static QString getName();

    [[nodiscard]] static QStringList webhookPlaceholders();
    [[nodiscard]] QStringList webhookTemplateProblems() const;

    [[nodiscard]] bool webhookTokenStored() const noexcept
    {
        return webhookTokenStored_;
    }

    void applyWebhooks(const core::WebhookSettings &webhooks);

    void setWebhookTokenStored(bool stored);

    using TokenCallback = std::function<void(QString)>;
    void setWebhookTokenCallback(TokenCallback store);

    using TestCallback = std::function<void()>;
    void setWebhookTestCallback(TestCallback test);

    using ApiCallback = std::function<void(core::ApiSettings)>;
    void setApiCallback(ApiCallback choose);

    using UpdatesCallback = std::function<void(core::UpdateSettings)>;
    void setUpdatesCallback(UpdatesCallback choose);
    void setApiTokenCallback(TokenCallback store);

    [[nodiscard]] QString logPath() const
    {
        return logPath_;
    }

    [[nodiscard]] QString configPath() const
    {
        return configPath_;
    }

    [[nodiscard]] QString databasePath() const
    {
        return databasePath_;
    }

    [[nodiscard]] QString recordingsPath() const
    {
        return recordingLibrary();
    }

    [[nodiscard]] qint64 configBytes() const noexcept
    {
        return sizeOf(configPath_);
    }

    [[nodiscard]] qint64 databaseBytes() const noexcept
    {
        return sizeOf(databasePath_);
    }

    [[nodiscard]] qint64 logBytes() const noexcept
    {
        return logBytes_;
    }

    void applyDiagnostics(const core::DiagnosticsSettings &diagnostics);

    void apply(std::vector<core::SettingChange> pending);

    void applyAudio(std::vector<core::AudioDeviceInfo> devices, core::AudioSettings chosen,
                    core::AudioApplyResult result);

public Q_SLOTS:
    void withdrawPending();

    void setRecordingKeepStems(bool keeping);
    void setRecordingKeepNotices(bool keeping);
    void setTranscribeAutomatically(bool automatic);
    void setTranscriptionLanguage(const QString &code);
    void setProcessAutomatically(bool automatic);
    void setCaptureEnabled(bool enabled);
    void setCaptureOnFinding(OnFinding answer);

    Q_INVOKABLE [[nodiscard]] OnFinding capturePolicyFor(const QString &process) const;
    Q_INVOKABLE void setCapturePolicyFor(const QString &process, OnFinding answer);

    Q_INVOKABLE void captureFollowsTheDefault(const QString &process);
    void setCapturePreRoll(int seconds);
    void setCaptureRemind(bool remind);

    Q_INVOKABLE bool startRecordingHere(const QString &what);

    Q_INVOKABLE bool startCaptureOf(const QString &what, const QString &process);
    Q_INVOKABLE void stopRecordingHere();

    Q_INVOKABLE bool renameCapture(const QString &what);

    Q_INVOKABLE int captureElapsedSeconds() const;

    [[nodiscard]] Sketch captureSketch() const noexcept;
    [[nodiscard]] Sketch captureStripSketch() const noexcept;
    Q_INVOKABLE void chooseCaptureSketch(Sketch sketch);
    Q_INVOKABLE void chooseCaptureStripSketch(Sketch sketch);

    Q_INVOKABLE void useCaptureDrawing(Sketch sketch);

    [[nodiscard]] static Sketch asSketch(core::Visualisation drawing) noexcept;
    [[nodiscard]] static core::Visualisation asVisualisation(Sketch sketch) noexcept;

    Q_INVOKABLE bool notACall(const QString &recordingId);

    Q_INVOKABLE [[nodiscard]] QStringList captureExceptions() const;

    void setMonthlyCeiling(const QString &amount);
    void setMonthlyTokenCeiling(const QString &amount);

    void setPrompterOffered(bool wanted);
    void setRealtimeRecogniserCeiling(const QString &amount);
    void setRealtimeModelCeiling(const QString &amount);

    void setPrompterColumnText(int pixels);
    void setPrompterRepeatsTheLast(bool wanted);
    void setPrompterRepeatText(int pixels);

    void chooseMicrophone(int index);
    void chooseSpeakers(int index);
    void chooseRinger(int index);

    void chooseRingtone(int index);

    void chooseRingtoneFile(const QUrl &url);

    void stopTest();

    void resetAudio();

    void refreshDevices();

    void testSpeakers();
    void testRinger();

    void offerCodec(int index, bool offered);

    void moveCodec(int index, int by);

    void chooseLanguage(const QString &code);

    [[nodiscard]] Layout layout() const noexcept;
    [[nodiscard]] QString barScreen() const;
    [[nodiscard]] Layout runningLayout() const noexcept;
    [[nodiscard]] bool layoutAwaitsARestart() const noexcept;

    Q_INVOKABLE void chooseLayout(Layout layout);

    Q_INVOKABLE void chooseBarScreen(const QString &name);

    Q_INVOKABLE void applyTheLayout();
    void chooseTheme(Theme theme);
    void chooseCallNotice(Notice notice);
    void chooseGlyphs(Glyphs glyphs);

    Q_INVOKABLE void chooseSketch(Sketch sketch);

    Q_INVOKABLE void chooseEndedCallSeconds(int seconds);
    void chooseIconScale(IconScale scale);

    void bindHotkey(Action action, const QString &sequence);

    void setHotkeyGlobal(Action action, bool global);

    void resetHotkeys();

    void setCapturingKeys(bool capturing);

    void setCloseToTray(bool value);
    void setAutostart(bool value);

    void setMenuEntry(bool wanted);
    void setStartMinimised(bool value);
    void setCallLinkDialsAtOnce(bool value);

    void chooseLogLevel(Verbosity level);

    void setWebhooksEnabled(bool wanted);
    void setWebhookUrl(const QString &wanted);
    void setWebhookPost(bool wanted);

    void setWebhookEvent(const QString &event, bool wanted);
    void setWebhookTemplate(const QString &wanted);
    void setWebhookHeaderName(const QString &wanted);

    void setWebhookToken(const QString &token);

    void setApiEnabled(bool wanted);
    void setApiPort(int wanted);
    void setApiResource(const QString &resource, bool wanted);
    void setApiToken(const QString &token);

    void setAutomaticUpdates(bool wanted);

    void sendTestWebhook();
    void setSystemLog(bool wanted);

    void openLog();

    void clearLog();

    Q_INVOKABLE void copyToClipboard(const QString &text) const;

    Q_INVOKABLE [[nodiscard]] QString readablePath(const QString &path) const;

    void refreshLog();

    void watchMicrophone(bool watching);

Q_SIGNALS:
    void pendingChanged();
    void firstRunChanged();
    void audioChanged();
    void levelChanged();
    void canTestChanged();
    void ringtoneProblemChanged();
    void volumesChanged();
    void ringPlayingChanged();
    void codecsChanged();
    void redialSettingsChanged();
    void appearanceChanged();
    void darkWantedChanged();
    void startupChanged();
    void linksChanged();
    void hotkeysChanged();
    void capturingKeysChanged();
    void hotkeyActivated(Action action);

    void layoutRestartWanted();
    void diagnosticsChanged();
    void webhooksChanged();
    void recordingChanged();
    void transcriptionChanged();
    void pipelineChanged();
    void realtimeChanged();
    void captureChanged();

    void captureFound(const QString &what, const QString &process);

    void captureLost();
    void captureStateChanged();
    void captureLevelChanged();
    void spendChanged();
    void webhookTokenChanged();
    void apiChanged();
    void updatesChanged();
    void apiTokenChanged();
    void pathsChanged();
    void logChanged();

    void logOpenRequested(const QString &path);

private:
    QStringList languages_;

    [[nodiscard]] QString describe(const core::AudioDevice &device, core::DeviceRole role) const;

    enum class Picker { Microphone, Speakers, Ringer };

    [[nodiscard]] static core::DeviceRole roleOf(Picker picker) noexcept
    {
        return picker == Picker::Microphone ? core::DeviceRole::Capture
                                            : core::DeviceRole::Playback;
    }

    void askForWebhooks();
    void askForApi();
    void askForCalls();
    void askForUpdates();

    [[nodiscard]] const core::AudioDevice &chosenFor(Picker picker) const noexcept;

    [[nodiscard]] std::vector<core::AudioDevice> devicesFor(Picker picker) const;

    [[nodiscard]] QVariantList entriesFor(Picker picker, bool missing) const;

    [[nodiscard]] int indexIn(Picker picker) const;

    void choose(Picker picker, int index);

    void sample();

    Dispatcher dispatch_;
    WithdrawCallback withdraw_;
    AudioCallback chooseAudio_;
    RefreshCallback refresh_;
    std::vector<core::SettingChange> pending_;
    std::vector<core::AudioDeviceInfo> devices_;

    std::map<core::AudioDevice, core::AudioDeviceInfo> seen_;
    core::AudioSettings audio_;
    core::AudioApplyResult result_;
    bool firstRun_{false};

    void chooseCodecs(std::vector<core::CodecChoice> wanted);

    void askForAppearance();

    void askForStartup();

    void theAppearanceIsNow();

    void sayIfTheColourMoved();

    AudioProbe probe_;
    CodecsCallback chooseCodecs_;
    AppearanceCallback chooseAppearance_;

    core::Layout runningLayout_{core::Layout::Windows};

    core::AppearanceSettings appearance_;
    DesktopScheme desktopScheme_;

    bool lastDark_{false};
    StartupCallback chooseStartup_;
    core::StartupSettings startup_;

    bool fromAnImage_{false};

    ILinkHandler *linkHandler_{nullptr};
    LinkHandlerState links_;
    HotkeysCallback chooseHotkeys_;
    DiagnosticsCallback chooseDiagnostics_;
    WebhooksCallback chooseWebhooks_;

    core::DiagnosticsSettings diagnostics_;
    core::WebhookSettings webhooks_;
    int webhookRevision_{0};
    RecordingCallback chooseRecording_;
    TranscriptionCallback chooseTranscription_;
    PipelineCallback choosePipeline_;
    RealtimeCallback chooseRealtime_;
    CaptureCallback chooseCapture_;
    CaptureCommand runCapture_;
    CaptureRename renameCapture_;
    CaptureNotACall notACall_;
    CaptureElapsed captureElapsed_;
    core::RecordingSettings recording_;
    ui::TranscriptionSettings transcription_;
    ui::PipelineSettings pipeline_;
    ui::RealtimeSettings realtime_;
    ui::CaptureSettings capture_;
    bool captureAvailable_{false};
    bool captureWatching_{false};
    bool captureRunning_{false};
    bool captureHeldOff_{false};
    QString captureWhat_;
    QString monthSoFar_;
    bool paused_{false};

    void askForRecording();
    void askForTranscription();
    void askForPipeline();
    void askForRealtime();
    void askForCapture();
    void refreshCaptureSampler();
    bool webhookTokenStored_{false};
    core::ApiSettings api_;
    int apiRevision_{0};
    bool apiTokenStored_{false};
    ApiCallback chooseApi_;
    UpdatesCallback chooseUpdates_;
    core::UpdateSettings updates_;
    TokenCallback storeApiToken_;
    TokenCallback storeToken_;
    TestCallback testWebhook_;
    QString logPath_;

    [[nodiscard]] static qint64 sizeOf(const QString &path);

    QString configPath_;
    QString databasePath_;
    qint64 logBytes_{0};
    std::vector<core::Hotkey> hotkeys_;
    std::map<core::HotkeyAction, Grab> grabs_;
    bool capturingKeys_ = false;
    bool globalSupported_{false};

    void askForHotkeys();
    std::vector<core::CodecInfo> available_;
    std::vector<core::CodecChoice> codecs_;

    core::CallWaiting callWaiting_{core::CallWaiting::Allowed};
    int redialIntervalSeconds_{15};
    int redialGiveUpMinutes_{30};
    int historyDays_{0};
    CallsCallback chooseCalls_;
    QTimer sampler_;
    core::LevelMeter meter_;
    QTimer captureSampler_;
    CaptureLevelSource takeCaptureLevels_;
    CaptureDrawingCallback drawCapture_;
    core::LevelMeter captureNear_;
    core::LevelMeter captureFar_;
    bool canTest_{true};
    core::Volumes volumes_;
    bool ringPlaying_{false};

    void playTest(core::ToneOutput output, std::string file, core::ToneLevel level);

    QTimer volumeSettling_;

    void setVolume(int core::Volumes::*field, int percent);

    RingProblem ringtoneProblem_{RingProblem::Playable};
    QString ringtoneProblemFile_;

    void inspectChosenRingtone();
};

}
