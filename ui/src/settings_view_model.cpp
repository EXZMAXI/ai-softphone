// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "settings_view_model.hpp"

#include "hotkeys.hpp"
#include "sounds.hpp"

#include "translations.hpp"

#include <softphone/core/http.hpp>
#include <softphone/core/logging.hpp>
#include <softphone/core/paths.hpp>
#include <softphone/core/ringtones.hpp>
#include <softphone/core/webhook_events.hpp>

#include <QClipboard>
#include <QFileInfo>
#include <QGuiApplication>
#include <QKeySequence>
#include <QStyleHints>
#include <QVariantMap>

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <system_error>
#include <utility>

namespace softphone::ui {
namespace {

[[nodiscard]] QString asOneLine(const QString &secret)
{
    return QString::fromStdString(core::asOneLine(secret.trimmed().toStdString()));
}

}

SettingsViewModel::SettingsViewModel(Dispatcher dispatch, WithdrawCallback withdraw,
                                     AudioCallback chooseAudio, RefreshCallback refresh,
                                     AudioProbe probe, CodecsCallback chooseCodecs,
                                     AppearanceCallback chooseAppearance,
                                     StartupCallback chooseStartup, HotkeysCallback chooseHotkeys,
                                     QObject *parent)
    : QObject{parent}, dispatch_{std::move(dispatch)}, withdraw_{std::move(withdraw)},
      chooseAudio_{std::move(chooseAudio)}, refresh_{std::move(refresh)}, probe_{std::move(probe)},
      chooseCodecs_{std::move(chooseCodecs)}, chooseAppearance_{std::move(chooseAppearance)},
      chooseStartup_{std::move(chooseStartup)}, chooseHotkeys_{std::move(chooseHotkeys)},
      hotkeys_{defaultHotkeys()}
{

    sampler_.setInterval(static_cast<int>(core::kLevelMeterInterval.count()));
    connect(&sampler_, &QTimer::timeout, this, &SettingsViewModel::sample);

    desktopScheme_ = []() {
        const auto *hints = QGuiApplication::styleHints();
        return hints != nullptr ? hints->colorScheme() : Qt::ColorScheme::Unknown;
    };
    if (auto *hints = QGuiApplication::styleHints(); hints != nullptr) {
        connect(hints, &QStyleHints::colorSchemeChanged, this,
                &SettingsViewModel::theDesktopChangedItsMind);
    }

    lastDark_ = darkWanted();

    captureSampler_.setInterval(static_cast<int>(core::kLevelMeterInterval.count()));
    connect(&captureSampler_, &QTimer::timeout, this, [this]() {
        if (!takeCaptureLevels_) {
            return;
        }
        const auto [near, far] = takeCaptureLevels_();
        applyCaptureLevels(near, far);
    });

    volumeSettling_.setSingleShot(true);
    volumeSettling_.setInterval(static_cast<int>(kVolumeSettling.count()));
    connect(&volumeSettling_, &QTimer::timeout, this, [this]() {
        if (!dispatch_ || !chooseAudio_) {
            return;
        }

        auto wanted = audio_;
        wanted.volumes = volumes_;
        if (wanted == audio_) {
            return;
        }

        const bool ring = wanted.volumes.ring != audio_.volumes.ring;
        const bool notices = wanted.volumes.notifications != audio_.volumes.notifications;

        dispatch_([chooseAudio = chooseAudio_, wanted]() { chooseAudio(wanted); });

        if (!canTest_) {
            return;
        }
        if (ring) {
            playTest(core::ToneOutput::Ringer, ringtoneFile(audio_.ringtone),
                     core::ToneLevel::Ring);
        } else if (notices) {

            playTest(core::ToneOutput::Speakers, toneFile(core::CallTone::Busy, audio_.ringtone),
                     core::ToneLevel::Notifications);
        }
    });
}

SettingsViewModel::Schedule SettingsViewModel::pendingSchedule() const noexcept
{
    auto worst = Schedule::Now;
    for (const auto &change : pending_) {
        switch (change.schedule) {
        case core::ApplySchedule::NextStart:
            return Schedule::NextStart;
        case core::ApplySchedule::WhenCallsEnd:
            worst = Schedule::WhenCallsEnd;
            break;
        case core::ApplySchedule::NextCall:
            if (worst == Schedule::Now) {
                worst = Schedule::NextCall;
            }
            break;
        case core::ApplySchedule::Now:
            break;
        }
    }
    return worst;
}

void SettingsViewModel::setFirstRun(bool value)
{
    if (firstRun_ == value) {
        return;
    }
    firstRun_ = value;
    Q_EMIT firstRunChanged();
}

void SettingsViewModel::apply(std::vector<core::SettingChange> pending)
{
    if (pending == pending_) {
        return;
    }
    pending_ = std::move(pending);
    Q_EMIT pendingChanged();
}

[[nodiscard]] QVariantMap entryFor(const core::AudioDevice &device, const QString &description,
                                   bool present)
{
    QVariantMap entry;
    entry["name"] = QString::fromStdString(device.name);
    entry["driver"] = QString::fromStdString(device.driver);
    entry["description"] = description;
    entry["present"] = present;
    return entry;
}

const core::AudioDevice &SettingsViewModel::chosenFor(Picker picker) const noexcept
{
    switch (picker) {
    case Picker::Microphone:
        return audio_.microphone;
    case Picker::Speakers:
        return audio_.speakers;
    case Picker::Ringer:
        break;
    }
    return audio_.ringer;
}

std::vector<core::AudioDevice> SettingsViewModel::devicesFor(Picker picker) const
{

    std::vector<core::AudioDevice> devices{core::AudioDevice{}};

    const auto role = roleOf(picker);
    for (const auto &device : devices_) {
        const bool suits =
            role == core::DeviceRole::Capture ? device.canCapture() : device.canPlay();
        if (suits) {
            devices.push_back(core::AudioDevice{device.driver, device.name});
        }
    }

    const auto &chosen = chosenFor(picker);
    if (!chosen.systemDefault() &&
        std::find(devices.begin(), devices.end(), chosen) == devices.end()) {
        devices.push_back(chosen);
    }

    return devices;
}

QString SettingsViewModel::describe(const core::AudioDevice &device, core::DeviceRole role) const
{
    for (const auto &known : devices_) {
        if (known.name == device.name && known.driver == device.driver) {
            return QString::fromStdString(known.describedAs(role));
        }
    }

    if (const auto seen = seen_.find(device); seen != seen_.end()) {
        return QString::fromStdString(seen->second.describedAs(role));
    }

    return {};
}

QVariantList SettingsViewModel::entriesFor(Picker picker, bool missing) const
{
    QVariantList entries;
    const auto role = roleOf(picker);
    for (const auto &device : devicesFor(picker)) {
        entries.append(
            entryFor(device, describe(device, role), !missing || device != chosenFor(picker)));
    }
    return entries;
}

QVariantList SettingsViewModel::microphones() const
{
    return entriesFor(Picker::Microphone, microphoneMissing());
}

QVariantList SettingsViewModel::speakers() const
{
    return entriesFor(Picker::Speakers, speakersMissing());
}

QVariantList SettingsViewModel::ringers() const
{
    return entriesFor(Picker::Ringer, ringerMissing());
}

QVariantList SettingsViewModel::ringtones() const
{
    QVariantList entries;

    for (const auto &ring : core::bundledRingtones()) {
        entries.append(QVariantMap{
            {QStringLiteral("id"),
             QString::fromUtf8(ring.id.data(), static_cast<qsizetype>(ring.id.size()))},
            {QStringLiteral("own"), false},
            {QStringLiteral("name"), QString{}},
        });
    }

    if (audio_.ringtone.ownFile()) {
        const QFileInfo file{QString::fromStdString(audio_.ringtone.file)};
        entries.append(QVariantMap{
            {QStringLiteral("id"), QString{}},
            {QStringLiteral("own"), true},
            {QStringLiteral("name"), file.fileName()},
        });
    }

    return entries;
}

int SettingsViewModel::ringtoneIndex() const
{
    const auto rings = core::bundledRingtones();
    if (audio_.ringtone.ownFile()) {
        return static_cast<int>(rings.size());
    }

    for (std::size_t index = 0; index < rings.size(); ++index) {
        if (rings[index].id == audio_.ringtone.bundled) {
            return static_cast<int>(index);
        }
    }

    return 0;
}

void SettingsViewModel::chooseRingtone(int index)
{
    if (!dispatch_ || !chooseAudio_) {
        return;
    }

    const auto rings = core::bundledRingtones();
    if (index < 0 || index > static_cast<int>(rings.size())) {
        return;
    }

    auto wanted = audio_;
    if (index == static_cast<int>(rings.size())) {

        return;
    }

    const auto &ring = rings[static_cast<std::size_t>(index)];
    wanted.ringtone.bundled = std::string{ring.id};
    wanted.ringtone.file.clear();

    if (wanted == audio_) {
        return;
    }

    if (ringtoneProblem_ != RingProblem::Playable) {
        ringtoneProblem_ = RingProblem::Playable;
        ringtoneProblemFile_.clear();
        Q_EMIT ringtoneProblemChanged();
    }

    dispatch_([chooseAudio = chooseAudio_, wanted]() { chooseAudio(wanted); });
}

void SettingsViewModel::chooseRingtoneFile(const QUrl &url)
{
    if (!dispatch_ || !chooseAudio_) {
        return;
    }

    const auto path = url.isLocalFile() ? url.toLocalFile() : url.toString();
    if (path.isEmpty()) {
        return;
    }

    const QFileInfo file{path};
    const auto fault = core::inspectRingtoneFile(core::fromUtf8(path.toStdString()));

    ringtoneProblemFile_ = file.fileName();
    ringtoneProblem_ = [fault]() {
        switch (fault) {
        case core::RingtoneFault::None:
            return RingProblem::Playable;
        case core::RingtoneFault::Missing:
            return RingProblem::NoSuchFile;
        case core::RingtoneFault::NotAFile:
            return RingProblem::NotAFile;
        case core::RingtoneFault::Unreadable:
            return RingProblem::Locked;
        case core::RingtoneFault::NotAWave:
            return RingProblem::NotAWave;
        case core::RingtoneFault::Unsupported:
            return RingProblem::WrongFormat;
        case core::RingtoneFault::Empty:
            return RingProblem::NoAudio;
        }
        return RingProblem::Playable;
    }();
    Q_EMIT ringtoneProblemChanged();

    if (ringtoneProblem_ != RingProblem::Playable) {

        core::Logger::instance().log(core::LogLevel::Info, "ui",
                                     "refused a ring file: " + path.toStdString());
        return;
    }

    auto wanted = audio_;
    wanted.ringtone.bundled.clear();
    wanted.ringtone.file = path.toStdString();
    if (wanted == audio_) {
        return;
    }

    dispatch_([chooseAudio = chooseAudio_, wanted]() { chooseAudio(wanted); });
}

void SettingsViewModel::setVolume(int core::Volumes::*field, int percent)
{
    const auto wanted = std::clamp(percent, 0, 100);
    if (volumes_.*field == wanted) {
        return;
    }

    volumes_.*field = wanted;
    Q_EMIT volumesChanged();

    volumeSettling_.start();
}

void SettingsViewModel::setMicrophoneVolume(int percent)
{
    setVolume(&core::Volumes::microphone, percent);
}

void SettingsViewModel::setSpeakerVolume(int percent)
{
    setVolume(&core::Volumes::speakers, percent);
}

void SettingsViewModel::setRingVolume(int percent)
{
    setVolume(&core::Volumes::ring, percent);
}

void SettingsViewModel::setNoticeVolume(int percent)
{
    setVolume(&core::Volumes::notifications, percent);
}

void SettingsViewModel::resetAudio()
{
    if (!dispatch_ || !chooseAudio_) {
        return;
    }

    const core::AudioSettings fresh;
    if (audio_ == fresh && volumes_ == fresh.volumes) {
        return;
    }

    volumeSettling_.stop();

    if (volumes_ != fresh.volumes) {
        volumes_ = fresh.volumes;
        Q_EMIT volumesChanged();
    }

    if (ringtoneProblem_ != RingProblem::Playable) {
        ringtoneProblem_ = RingProblem::Playable;
        ringtoneProblemFile_.clear();
        Q_EMIT ringtoneProblemChanged();
    }

    dispatch_([chooseAudio = chooseAudio_, fresh]() { chooseAudio(fresh); });
}

void SettingsViewModel::inspectChosenRingtone()
{
    const auto before = ringtoneProblem_;
    const auto beforeFile = ringtoneProblemFile_;

    if (!audio_.ringtone.ownFile()) {
        ringtoneProblem_ = RingProblem::Playable;
        ringtoneProblemFile_.clear();
    } else {
        const QFileInfo file{QString::fromStdString(audio_.ringtone.file)};
        const auto fault = core::inspectRingtoneFile(core::fromUtf8(audio_.ringtone.file));
        ringtoneProblem_ =
            fault == core::RingtoneFault::None ? RingProblem::Playable : RingProblem::NoSuchFile;

        ringtoneProblemFile_ =
            ringtoneProblem_ == RingProblem::Playable ? QString{} : file.fileName();
    }

    if (ringtoneProblem_ != before || ringtoneProblemFile_ != beforeFile) {
        Q_EMIT ringtoneProblemChanged();
    }
}

int SettingsViewModel::indexIn(Picker picker) const
{
    const auto devices = devicesFor(picker);
    const auto at = std::find(devices.begin(), devices.end(), chosenFor(picker));
    return at == devices.end() ? 0 : static_cast<int>(at - devices.begin());
}

int SettingsViewModel::microphoneIndex() const
{
    return indexIn(Picker::Microphone);
}

int SettingsViewModel::speakerIndex() const
{
    return indexIn(Picker::Speakers);
}

int SettingsViewModel::ringerIndex() const
{
    return indexIn(Picker::Ringer);
}

void SettingsViewModel::applyAudio(std::vector<core::AudioDeviceInfo> devices,
                                   core::AudioSettings chosen, core::AudioApplyResult result)
{
    if (devices == devices_ && chosen == audio_ && result == result_) {
        return;
    }

    for (const auto &device : devices) {
        seen_[core::AudioDevice{device.driver, device.name}] = device;
    }

    devices_ = std::move(devices);
    audio_ = std::move(chosen);
    result_ = std::move(result);

    if (!volumeSettling_.isActive()) {
        if (volumes_ != audio_.volumes) {
            volumes_ = audio_.volumes;
            Q_EMIT volumesChanged();
        }
    }

    inspectChosenRingtone();
    Q_EMIT audioChanged();
}

void SettingsViewModel::choose(Picker picker, int index)
{
    if (!dispatch_ || !chooseAudio_) {
        return;
    }

    const auto devices = devicesFor(picker);
    if (index < 0 || index >= static_cast<int>(devices.size())) {
        return;
    }

    auto wanted = audio_;
    const auto &chosen = devices.at(static_cast<std::size_t>(index));
    switch (picker) {
    case Picker::Microphone:
        wanted.microphone = chosen;
        break;
    case Picker::Speakers:
        wanted.speakers = chosen;
        break;
    case Picker::Ringer:
        wanted.ringer = chosen;
        break;
    }

    if (wanted == audio_) {
        return;
    }

    dispatch_([chooseAudio = chooseAudio_, wanted]() { chooseAudio(wanted); });
}

void SettingsViewModel::chooseMicrophone(int index)
{
    choose(Picker::Microphone, index);
}

void SettingsViewModel::chooseSpeakers(int index)
{
    choose(Picker::Speakers, index);
}

void SettingsViewModel::chooseRinger(int index)
{
    choose(Picker::Ringer, index);
}

void SettingsViewModel::refreshDevices()
{
    if (!dispatch_ || !refresh_) {
        return;
    }
    dispatch_([refresh = refresh_]() { refresh(); });
}

QVariantList SettingsViewModel::codecs() const
{
    QVariantList entries;
    entries.reserve(static_cast<qsizetype>(codecs_.size()));

    for (const auto &choice : codecs_) {

        const auto known = std::ranges::find(available_, choice.id, &core::CodecInfo::id);
        const auto described = known != available_.end() ? *known : core::describeCodec(choice.id);

        QVariantMap entry;
        entry.insert(QStringLiteral("id"), QString::fromStdString(described.id));
        entry.insert(QStringLiteral("name"), QString::fromStdString(described.name));
        entry.insert(QStringLiteral("clockRate"), described.clockRate);

        entry.insert(QStringLiteral("channels"), described.channels);
        entry.insert(QStringLiteral("wideband"), described.wideband());
        entry.insert(QStringLiteral("enabled"), choice.enabled);
        entries.append(entry);
    }

    return entries;
}

int SettingsViewModel::offeredCodecCount() const noexcept
{
    return static_cast<int>(std::ranges::count(codecs_, true, &core::CodecChoice::enabled));
}

void SettingsViewModel::applyCodecs(std::vector<core::CodecInfo> available,
                                    std::vector<core::CodecChoice> chosen)
{
    if (available == available_ && chosen == codecs_) {
        return;
    }

    available_ = std::move(available);
    codecs_ = std::move(chosen);
    Q_EMIT codecsChanged();
}

void SettingsViewModel::offerCodec(int index, bool offered)
{
    if (index < 0 || index >= static_cast<int>(codecs_.size())) {
        return;
    }

    auto wanted = codecs_;
    auto &choice = wanted.at(static_cast<std::size_t>(index));
    if (choice.enabled == offered) {
        return;
    }

    if (!offered && offeredCodecCount() <= 1) {
        return;
    }

    choice.enabled = offered;
    chooseCodecs(std::move(wanted));
}

void SettingsViewModel::moveCodec(int index, int by)
{
    const auto size = static_cast<int>(codecs_.size());
    const auto to = index + by;
    if (index < 0 || index >= size || to < 0 || to >= size || by == 0) {
        return;
    }

    auto wanted = codecs_;
    std::swap(wanted.at(static_cast<std::size_t>(index)), wanted.at(static_cast<std::size_t>(to)));
    chooseCodecs(std::move(wanted));
}

void SettingsViewModel::chooseCodecs(std::vector<core::CodecChoice> wanted)
{

    codecs_ = wanted;
    Q_EMIT codecsChanged();

    if (!dispatch_ || !chooseCodecs_) {
        return;
    }
    dispatch_([choose = chooseCodecs_, wanted = std::move(wanted)]() mutable {
        choose(std::move(wanted));
    });
}

QString SettingsViewModel::language() const
{
    return QString::fromStdString(appearance_.language);
}

QVariantList SettingsViewModel::languages() const
{
    QVariantList entries;

    const auto entryFor = [](const QString &code) {
        QVariantMap entry;
        entry.insert(QStringLiteral("code"), code);

        entry.insert(QStringLiteral("name"), languageEndonym(code));
        return QVariant{entry};
    };

    entries.append(entryFor(QStringLiteral("en")));
    for (const auto &code : languages_) {
        entries.append(entryFor(code));
    }
    return entries;
}

void SettingsViewModel::setLanguages(QStringList codes)
{

    languages_ = std::move(codes);
}

SettingsViewModel::Theme SettingsViewModel::theme() const noexcept
{
    switch (appearance_.theme) {
    case core::Theme::Light:
        return Theme::Light;
    case core::Theme::Dark:
        return Theme::Dark;
    case core::Theme::Default:
        break;
    }
    return Theme::Default;
}

bool SettingsViewModel::darkWanted() const noexcept
{
    switch (appearance_.theme) {
    case core::Theme::Light:
        return false;
    case core::Theme::Dark:
        return true;
    case core::Theme::Default:
        break;
    }

    return desktopScheme_ && desktopScheme_() == Qt::ColorScheme::Dark;
}

void SettingsViewModel::followTheDesktop(DesktopScheme where)
{
    desktopScheme_ = std::move(where);
    sayIfTheColourMoved();
}

void SettingsViewModel::theDesktopChangedItsMind()
{

    sayIfTheColourMoved();
}

void SettingsViewModel::sayIfTheColourMoved()
{
    const auto dark = darkWanted();
    if (dark == lastDark_) {
        return;
    }
    lastDark_ = dark;
    Q_EMIT darkWantedChanged();
}

void SettingsViewModel::theAppearanceIsNow()
{
    Q_EMIT appearanceChanged();
    sayIfTheColourMoved();
}

void SettingsViewModel::applyAppearance(const core::AppearanceSettings &appearance)
{
    if (appearance == appearance_) {
        return;
    }
    appearance_ = appearance;
    theAppearanceIsNow();
}

SettingsViewModel::Verbosity SettingsViewModel::logLevel() const noexcept
{
    switch (diagnostics_.level) {
    case core::LogLevel::Error:
        return Verbosity::Error;
    case core::LogLevel::Warning:
        return Verbosity::Warning;
    case core::LogLevel::Info:
        return Verbosity::Info;
    case core::LogLevel::Debug:
        return Verbosity::Debug;
    case core::LogLevel::Trace:
        return Verbosity::Trace;
    }
    return Verbosity::Info;
}

void SettingsViewModel::applyDiagnostics(const core::DiagnosticsSettings &diagnostics)
{
    if (diagnostics == diagnostics_) {
        return;
    }
    diagnostics_ = diagnostics;
    Q_EMIT diagnosticsChanged();
}

void SettingsViewModel::setDiagnosticsCallback(DiagnosticsCallback choose)
{
    chooseDiagnostics_ = std::move(choose);
}

void SettingsViewModel::setPaths(QString log, QString config, QString database)
{
    logPath_ = std::move(log);
    configPath_ = std::move(config);
    databasePath_ = std::move(database);
    Q_EMIT pathsChanged();
    refreshLog();
}

namespace {

[[nodiscard]] core::Layout asCore(SettingsViewModel::Layout layout) noexcept
{
    switch (layout) {
    case SettingsViewModel::Layout::Windows:
        return core::Layout::Windows;
    case SettingsViewModel::Layout::Compact:
        return core::Layout::Compact;
    case SettingsViewModel::Layout::Workspace:
        return core::Layout::Workspace;
    case SettingsViewModel::Layout::WorkspaceAndPhone:
        return core::Layout::WorkspaceAndPhone;
    case SettingsViewModel::Layout::Bar:
        return core::Layout::Bar;
    case SettingsViewModel::Layout::NoWindow:
        return core::Layout::NoWindow;
    }
    return core::Layout::Windows;
}

[[nodiscard]] SettingsViewModel::Layout asWindow(core::Layout layout) noexcept
{
    switch (layout) {
    case core::Layout::Windows:
        return SettingsViewModel::Layout::Windows;
    case core::Layout::Compact:
        return SettingsViewModel::Layout::Compact;
    case core::Layout::Workspace:
        return SettingsViewModel::Layout::Workspace;
    case core::Layout::WorkspaceAndPhone:
        return SettingsViewModel::Layout::WorkspaceAndPhone;
    case core::Layout::Bar:
        return SettingsViewModel::Layout::Bar;
    case core::Layout::NoWindow:
        return SettingsViewModel::Layout::NoWindow;
    }
    return SettingsViewModel::Layout::Windows;
}

}

void SettingsViewModel::setRunningLayout(core::Layout layout)
{
    runningLayout_ = layout;
}

SettingsViewModel::Layout SettingsViewModel::layout() const noexcept
{
    return asWindow(appearance_.layout);
}

SettingsViewModel::Layout SettingsViewModel::runningLayout() const noexcept
{
    return asWindow(runningLayout_);
}

bool SettingsViewModel::layoutAwaitsARestart() const noexcept
{
    return appearance_.layout != runningLayout_;
}

void SettingsViewModel::chooseLayout(Layout layout)
{
    const auto wanted = asCore(layout);
    if (wanted == appearance_.layout) {
        return;
    }

    appearance_.layout = wanted;
    theAppearanceIsNow();
    askForAppearance();
}

QString SettingsViewModel::barScreen() const
{
    return QString::fromStdString(appearance_.barScreen);
}

void SettingsViewModel::chooseBarScreen(const QString &name)
{
    const auto wanted = name.toStdString();
    if (wanted == appearance_.barScreen) {
        return;
    }

    appearance_.barScreen = wanted;
    theAppearanceIsNow();
    askForAppearance();
}

void SettingsViewModel::applyTheLayout()
{

    if (!layoutAwaitsARestart()) {
        return;
    }

    core::Logger::instance().log(core::LogLevel::Info, "ui",
                                 std::string{"restarting into the "} +
                                     std::string{toString(appearance_.layout)} + " arrangement");
    Q_EMIT layoutRestartWanted();
}

void SettingsViewModel::chooseLogLevel(Verbosity level)
{
    const auto wanted = level == Verbosity::Error     ? core::LogLevel::Error
                        : level == Verbosity::Warning ? core::LogLevel::Warning
                        : level == Verbosity::Debug   ? core::LogLevel::Debug
                        : level == Verbosity::Trace   ? core::LogLevel::Trace
                                                      : core::LogLevel::Info;
    if (wanted == diagnostics_.level) {
        return;
    }

    diagnostics_.level = wanted;
    Q_EMIT diagnosticsChanged();

    if (dispatch_ && chooseDiagnostics_) {
        dispatch_([choose = chooseDiagnostics_, chosen = diagnostics_]() { choose(chosen); });
    }
}

bool SettingsViewModel::systemLogAvailable() noexcept
{
    return core::SystemLogSink::available();
}

QString SettingsViewModel::systemLogCommand()
{

    return QStringLiteral("journalctl -t %1 -f")
        .arg(QString::fromUtf8(core::SystemLogSink::kIdentity.data(),
                               static_cast<qsizetype>(core::SystemLogSink::kIdentity.size())));
}

void SettingsViewModel::setSystemLog(bool wanted)
{
    if (wanted == diagnostics_.systemLog) {
        return;
    }

    diagnostics_.systemLog = wanted;
    Q_EMIT diagnosticsChanged();

    if (dispatch_ && chooseDiagnostics_) {
        dispatch_([choose = chooseDiagnostics_, chosen = diagnostics_]() { choose(chosen); });
    }
}

void SettingsViewModel::setWebhooksCallback(WebhooksCallback choose)
{
    chooseWebhooks_ = std::move(choose);
}

void SettingsViewModel::setWebhookTokenCallback(TokenCallback store)
{
    storeToken_ = std::move(store);
}

void SettingsViewModel::setWebhookTestCallback(TestCallback test)
{
    testWebhook_ = std::move(test);
}

void SettingsViewModel::applyWebhooks(const core::WebhookSettings &webhooks)
{
    if (webhooks == webhooks_) {
        return;
    }
    webhooks_ = webhooks;
    ++webhookRevision_;
    Q_EMIT webhooksChanged();
}

void SettingsViewModel::setWebhookTokenStored(bool stored)
{
    if (webhookTokenStored_ == stored) {
        return;
    }
    webhookTokenStored_ = stored;
    Q_EMIT webhookTokenChanged();
}

QString SettingsViewModel::apiAddress() const
{
    return QStringLiteral("http://127.0.0.1:%1/").arg(api_.port);
}

void SettingsViewModel::applyApi(const core::ApiSettings &api)
{
    if (api == api_) {
        return;
    }
    api_ = api;
    ++apiRevision_;
    Q_EMIT apiChanged();
}

void SettingsViewModel::applyUpdates(const core::UpdateSettings &updates)
{
    if (updates == updates_) {
        return;
    }
    updates_ = updates;
    Q_EMIT updatesChanged();
}

void SettingsViewModel::setUpdatesCallback(UpdatesCallback choose)
{
    chooseUpdates_ = std::move(choose);
}

void SettingsViewModel::setCallsCallback(CallsCallback choose)
{
    chooseCalls_ = std::move(choose);
}

void SettingsViewModel::applyCallSettings(int intervalSeconds, int giveUpMinutes, int historyDays,
                                          core::CallWaiting waiting)
{
    const auto interval = std::clamp(intervalSeconds, core::kMinRedialIntervalSeconds,
                                     core::kMaxRedialIntervalSeconds);
    const auto giveUp =
        std::clamp(giveUpMinutes, core::kMinRedialGiveUpMinutes, core::kMaxRedialGiveUpMinutes);

    const auto history = std::clamp(historyDays, 0, core::kMaxHistoryDays);

    if (interval == redialIntervalSeconds_ && giveUp == redialGiveUpMinutes_ &&
        history == historyDays_ && waiting == callWaiting_) {
        return;
    }

    redialIntervalSeconds_ = interval;
    redialGiveUpMinutes_ = giveUp;
    historyDays_ = history;
    callWaiting_ = waiting;
    Q_EMIT redialSettingsChanged();
}

void SettingsViewModel::chooseRedialInterval(int seconds)
{
    const auto wanted =
        std::clamp(seconds, core::kMinRedialIntervalSeconds, core::kMaxRedialIntervalSeconds);
    if (wanted == redialIntervalSeconds_) {
        return;
    }

    redialIntervalSeconds_ = wanted;
    Q_EMIT redialSettingsChanged();
    askForCalls();
}

void SettingsViewModel::chooseCallWaiting(CallWaiting waiting)
{
    const auto wanted =
        waiting == CallWaiting::Busy ? core::CallWaiting::Busy : core::CallWaiting::Allowed;
    if (wanted == callWaiting_) {
        return;
    }

    callWaiting_ = wanted;
    Q_EMIT redialSettingsChanged();
    askForCalls();
}

void SettingsViewModel::chooseRedialGiveUp(int minutes)
{
    const auto wanted =
        std::clamp(minutes, core::kMinRedialGiveUpMinutes, core::kMaxRedialGiveUpMinutes);
    if (wanted == redialGiveUpMinutes_) {
        return;
    }

    redialGiveUpMinutes_ = wanted;
    Q_EMIT redialSettingsChanged();
    askForCalls();
}

void SettingsViewModel::chooseHistoryDays(int days)
{
    const auto wanted = std::clamp(days, 0, core::kMaxHistoryDays);
    if (wanted == historyDays_) {
        return;
    }

    historyDays_ = wanted;
    Q_EMIT redialSettingsChanged();
    askForCalls();
}

void SettingsViewModel::askForCalls()
{
    if (!dispatch_ || !chooseCalls_) {
        return;
    }
    dispatch_([choose = chooseCalls_, interval = redialIntervalSeconds_,
               giveUp = redialGiveUpMinutes_, history = historyDays_,
               waiting = callWaiting_]() { choose(interval, giveUp, history, waiting); });
}

void SettingsViewModel::askForUpdates()
{
    if (!dispatch_ || !chooseUpdates_) {
        return;
    }
    dispatch_([choose = chooseUpdates_, wanted = updates_]() { choose(wanted); });
}

void SettingsViewModel::setAutomaticUpdates(bool wanted)
{
    if (updates_.automatic == wanted) {
        return;
    }
    updates_.automatic = wanted;
    Q_EMIT updatesChanged();
    askForUpdates();
}

void SettingsViewModel::setApiTokenStored(bool stored)
{
    if (apiTokenStored_ == stored) {
        return;
    }
    apiTokenStored_ = stored;
    Q_EMIT apiTokenChanged();
}

void SettingsViewModel::setApiCallback(ApiCallback choose)
{
    chooseApi_ = std::move(choose);
}

void SettingsViewModel::setApiTokenCallback(TokenCallback store)
{
    storeApiToken_ = std::move(store);
}

void SettingsViewModel::askForApi()
{
    if (!dispatch_ || !chooseApi_) {
        return;
    }
    dispatch_([choose = chooseApi_, wanted = api_]() { choose(wanted); });
}

void SettingsViewModel::setApiEnabled(bool wanted)
{
    if (api_.enabled == wanted) {
        return;
    }
    api_.enabled = wanted;
    ++apiRevision_;
    Q_EMIT apiChanged();
    askForApi();
}

void SettingsViewModel::setApiPort(int wanted)
{

    if (wanted < 1024 || wanted > 65535 || wanted == api_.port) {
        return;
    }
    api_.port = static_cast<std::uint16_t>(wanted);
    ++apiRevision_;
    Q_EMIT apiChanged();
    askForApi();
}

void SettingsViewModel::setApiResource(const QString &resource, bool wanted)
{

    const auto name = resource.toStdString();
    auto &off = api_.disabled;
    const auto at = std::ranges::find(off, name);

    if (wanted == (at == off.end())) {
        return;
    }

    if (wanted) {
        off.erase(at);
    } else {
        off.push_back(name);
    }

    ++apiRevision_;
    Q_EMIT apiChanged();
    askForApi();
}

void SettingsViewModel::setApiToken(const QString &token)
{
    if (storeApiToken_) {
        storeApiToken_(asOneLine(token));
    }
}

QString SettingsViewModel::postName()
{
    const auto name = core::toString(core::HttpMethod::Post);
    return QString::fromUtf8(name.data(), static_cast<qsizetype>(name.size()));
}

QString SettingsViewModel::getName()
{
    const auto name = core::toString(core::HttpMethod::Get);
    return QString::fromUtf8(name.data(), static_cast<qsizetype>(name.size()));
}

QStringList SettingsViewModel::webhookPlaceholders()
{
    QStringList names;
    for (const auto &placeholder : core::webhookPlaceholders()) {
        names.append(
            QString::fromUtf8(placeholder.data(), static_cast<qsizetype>(placeholder.size())));
    }
    return names;
}

QStringList SettingsViewModel::webhookTemplateProblems() const
{
    QStringList problems;
    for (const auto &unknown : core::unknownPlaceholders(webhooks_.urlTemplate)) {
        problems.append(QString::fromStdString(unknown));
    }
    return problems;
}

void SettingsViewModel::askForWebhooks()
{
    if (!dispatch_ || !chooseWebhooks_) {
        return;
    }

    auto wanted = webhooks_;
    wanted.url = QString::fromStdString(wanted.url).trimmed().toStdString();
    wanted.urlTemplate = QString::fromStdString(wanted.urlTemplate).trimmed().toStdString();
    wanted.headerName = QString::fromStdString(wanted.headerName).trimmed().toStdString();

    dispatch_([choose = chooseWebhooks_, wanted = std::move(wanted)]() { choose(wanted); });
}

void SettingsViewModel::setWebhooksEnabled(bool wanted)
{
    if (webhooks_.enabled == wanted) {
        return;
    }
    webhooks_.enabled = wanted;
    ++webhookRevision_;
    Q_EMIT webhooksChanged();
    askForWebhooks();
}

void SettingsViewModel::setWebhookUrl(const QString &wanted)
{

    const auto text = wanted.toStdString();
    if (webhooks_.url == text) {
        return;
    }
    webhooks_.url = text;
    ++webhookRevision_;
    Q_EMIT webhooksChanged();
    askForWebhooks();
}

void SettingsViewModel::setWebhookPost(bool wanted)
{
    const auto method = wanted ? core::HttpMethod::Post : core::HttpMethod::Get;
    if (webhooks_.method == method) {
        return;
    }
    webhooks_.method = method;
    ++webhookRevision_;
    Q_EMIT webhooksChanged();
    askForWebhooks();
}

void SettingsViewModel::setWebhookEvent(const QString &event, bool wanted)
{

    const auto name = event.toStdString();
    auto &quiet = webhooks_.silenced;
    const auto at = std::ranges::find(quiet, name);

    if (wanted == (at == quiet.end())) {
        return;
    }

    if (wanted) {
        quiet.erase(at);
    } else {
        quiet.push_back(name);
    }

    ++webhookRevision_;
    Q_EMIT webhooksChanged();
    askForWebhooks();
}

void SettingsViewModel::setWebhookTemplate(const QString &wanted)
{

    const auto text = wanted.toStdString();
    if (webhooks_.urlTemplate == text) {
        return;
    }
    webhooks_.urlTemplate = text;
    ++webhookRevision_;
    Q_EMIT webhooksChanged();

    if (webhookTemplateProblems().isEmpty()) {
        askForWebhooks();
    }
}

void SettingsViewModel::setWebhookHeaderName(const QString &wanted)
{

    const auto text = wanted.toStdString();
    if (webhooks_.headerName == text) {
        return;
    }
    webhooks_.headerName = text;
    ++webhookRevision_;
    Q_EMIT webhooksChanged();
    askForWebhooks();
}

void SettingsViewModel::setWebhookToken(const QString &token)
{
    if (storeToken_) {
        storeToken_(asOneLine(token));
    }
}

void SettingsViewModel::sendTestWebhook()
{
    if (testWebhook_) {
        testWebhook_();
    }
}

void SettingsViewModel::openLog()
{
    if (logPath_.isEmpty()) {
        return;
    }
    Q_EMIT logOpenRequested(logPath_);
}

void SettingsViewModel::clearLog()
{

    static_cast<void>(core::Logger::instance().clear());
    refreshLog();
}

QString SettingsViewModel::readablePath(const QString &path) const
{
    if (path.isEmpty()) {
        return path;
    }

    return QString::fromStdString(core::readablePath(core::fromUtf8(path.toStdString())));
}

void SettingsViewModel::copyToClipboard(const QString &text) const
{
    if (auto *clipboard = QGuiApplication::clipboard(); clipboard != nullptr) {
        clipboard->setText(text);
    }
}

qint64 SettingsViewModel::sizeOf(const QString &path)
{
    if (path.isEmpty()) {
        return 0;
    }

    std::error_code error;
    const auto size = std::filesystem::file_size(core::fromUtf8(path.toStdString()), error);
    return error ? qint64{0} : static_cast<qint64>(size);
}

void SettingsViewModel::refreshLog()
{
    const auto bytes = sizeOf(logPath_);
    if (bytes == logBytes_) {
        return;
    }
    logBytes_ = bytes;
    Q_EMIT logChanged();
}

void SettingsViewModel::applyStartup(const core::StartupSettings &startup)
{
    if (startup == startup_) {
        return;
    }
    startup_ = startup;
    Q_EMIT startupChanged();
}

void SettingsViewModel::setCloseToTray(bool value)
{
    if (startup_.closeToTray == value) {
        return;
    }
    startup_.closeToTray = value;
    Q_EMIT startupChanged();
    askForStartup();
}

void SettingsViewModel::setAutostart(bool value)
{
    if (startup_.autostart == value) {
        return;
    }
    startup_.autostart = value;
    Q_EMIT startupChanged();
    askForStartup();
}

void SettingsViewModel::thisRunsFromAnImage(bool fromAnImage)
{
    if (fromAnImage_ == fromAnImage) {
        return;
    }
    fromAnImage_ = fromAnImage;

    Q_EMIT startupChanged();
}

void SettingsViewModel::setMenuEntry(bool wanted)
{
    const auto answer = wanted ? core::MenuEntry::Wanted : core::MenuEntry::NotWanted;
    if (startup_.menuEntry == answer) {
        return;
    }

    startup_.menuEntry = answer;
    Q_EMIT startupChanged();
    askForStartup();
}

void SettingsViewModel::setCallLinkDialsAtOnce(bool value)
{
    if (startup_.callLinkDialsAtOnce == value) {
        return;
    }
    startup_.callLinkDialsAtOnce = value;
    Q_EMIT startupChanged();
    askForStartup();
}

void SettingsViewModel::answersLinksThrough(ILinkHandler *handler)
{
    linkHandler_ = handler;
    refreshLinks();
}

void SettingsViewModel::refreshLinks()
{
    const auto answer = linkHandler_ == nullptr ? LinkHandlerState{} : linkHandler_->state();
    if (answer == links_) {
        return;
    }
    links_ = answer;
    Q_EMIT linksChanged();
}

void SettingsViewModel::claimLinks()
{
    if (linkHandler_ == nullptr) {
        return;
    }

    static_cast<void>(linkHandler_->claim());
    refreshLinks();
}

void SettingsViewModel::setStartMinimised(bool value)
{
    if (startup_.startMinimised == value) {
        return;
    }
    startup_.startMinimised = value;
    Q_EMIT startupChanged();
    askForStartup();
}

void SettingsViewModel::askForStartup()
{
    if (!dispatch_ || !chooseStartup_) {
        return;
    }
    dispatch_([choose = chooseStartup_, wanted = startup_]() { choose(wanted); });
}

QVariantList SettingsViewModel::hotkeys() const
{
    const auto clashing = clashingHotkeys(hotkeys_);

    QVariantList rows;
    rows.reserve(static_cast<qsizetype>(hotkeys_.size()));

    for (const auto &binding : hotkeys_) {
        QVariantMap row;
        row.insert(QStringLiteral("action"),
                   QVariant::fromValue(static_cast<Action>(binding.action)));
        row.insert(QStringLiteral("sequence"), QString::fromStdString(binding.sequence));
        row.insert(QStringLiteral("global"), binding.global);
        row.insert(QStringLiteral("clashing"),
                   std::ranges::find(clashing, binding.action) != clashing.end());

        const auto grabbed = grabs_.find(binding.action);
        row.insert(QStringLiteral("grab"),
                   QVariant::fromValue(grabbed == grabs_.end() ? Grab::Working : grabbed->second));
        rows.append(row);
    }

    return rows;
}

void SettingsViewModel::applyHotkeys(const core::HotkeySettings &hotkeys)
{
    auto planned = planHotkeys(hotkeys.bindings);
    if (planned == hotkeys_) {
        return;
    }

    hotkeys_ = std::move(planned);
    Q_EMIT hotkeysChanged();
}

void SettingsViewModel::applyHotkeyGrabs(std::span<const GrabOutcome> outcomes, bool supported)
{
    std::map<core::HotkeyAction, Grab> grabs;

    for (const auto &outcome : outcomes) {
        switch (outcome.problem) {
        case GrabProblem::None:
            grabs.emplace(outcome.action, Grab::Working);
            break;
        case GrabProblem::Unsupported:
            grabs.emplace(outcome.action, Grab::Unsupported);
            break;
        case GrabProblem::Taken:
            grabs.emplace(outcome.action, Grab::Taken);
            break;
        case GrabProblem::Unreadable:
            grabs.emplace(outcome.action, Grab::NoSuchKey);
            break;
        }
    }

    if (grabs == grabs_ && supported == globalSupported_) {
        return;
    }

    grabs_ = std::move(grabs);
    globalSupported_ = supported;
    Q_EMIT hotkeysChanged();
}

void SettingsViewModel::announceHotkey(Action action)
{
    Q_EMIT hotkeyActivated(action);
}

QString SettingsViewModel::sequenceFor(int key, int modifiers) const
{
    const auto pressed = static_cast<Qt::Key>(key);

    switch (pressed) {
    case Qt::Key_Control:
    case Qt::Key_Alt:
    case Qt::Key_Shift:
    case Qt::Key_Meta:
    case Qt::Key_AltGr:
    case Qt::Key_CapsLock:
    case Qt::Key_NumLock:
    case Qt::Key_ScrollLock:
    case Qt::Key_unknown:
        return {};
    default:
        break;
    }

    const QKeySequence sequence{
        QKeyCombination{static_cast<Qt::KeyboardModifiers>(modifiers), pressed}};
    return sequence.toString(QKeySequence::PortableText);
}

QString SettingsViewModel::shownSequence(const QString &sequence) const
{
    if (sequence.isEmpty()) {
        return {};
    }

    const auto native = QKeySequence::fromString(sequence, QKeySequence::PortableText)
                            .toString(QKeySequence::NativeText);

    return native.isEmpty() ? sequence : native;
}

SettingsViewModel::KeyProblem SettingsViewModel::checkSequence(const QString &sequence) const
{
    switch (checkHotkey(sequence.toStdString())) {
    case HotkeyProblem::None:
        return KeyProblem::None;
    case HotkeyProblem::Unreadable:
        return KeyProblem::Unreadable;
    case HotkeyProblem::NoModifier:
        return KeyProblem::NoModifier;
    }
    return KeyProblem::Unreadable;
}

void SettingsViewModel::bindHotkey(Action action, const QString &sequence)
{
    const auto wanted = static_cast<core::HotkeyAction>(action);
    const auto text = normaliseHotkey(sequence.toStdString());

    auto found = std::ranges::find(hotkeys_, wanted, &core::Hotkey::action);
    if (found == hotkeys_.end()) {
        return;
    }

    const bool takingItBack = text.empty() && found->global;

    if (found->sequence == text && !takingItBack) {
        return;
    }

    found->sequence = text;
    if (text.empty()) {
        found->global = false;
    }

    Q_EMIT hotkeysChanged();
    askForHotkeys();
}

void SettingsViewModel::setHotkeyGlobal(Action action, bool global)
{
    const auto wanted = static_cast<core::HotkeyAction>(action);

    auto found = std::ranges::find(hotkeys_, wanted, &core::Hotkey::action);
    if (found == hotkeys_.end() || found->global == global) {
        return;
    }

    found->global = global;
    Q_EMIT hotkeysChanged();
    askForHotkeys();
}

void SettingsViewModel::resetHotkeys()
{
    auto shipped = defaultHotkeys();
    if (shipped == hotkeys_) {
        return;
    }

    hotkeys_ = std::move(shipped);
    Q_EMIT hotkeysChanged();
    askForHotkeys();
}

void SettingsViewModel::setCapturingKeys(bool capturing)
{
    if (capturingKeys_ == capturing) {
        return;
    }

    capturingKeys_ = capturing;
    Q_EMIT capturingKeysChanged();

    Q_EMIT hotkeysChanged();
}

void SettingsViewModel::askForHotkeys()
{
    if (!dispatch_ || !chooseHotkeys_) {
        return;
    }
    dispatch_(
        [choose = chooseHotkeys_, wanted = core::HotkeySettings{hotkeys_}]() { choose(wanted); });
}

void SettingsViewModel::chooseLanguage(const QString &code)
{
    if (code.toStdString() == appearance_.language) {
        return;
    }

    appearance_.language = code.toStdString();
    theAppearanceIsNow();
    askForAppearance();
}

void SettingsViewModel::chooseTheme(Theme theme)
{
    const auto wanted = theme == Theme::Light  ? core::Theme::Light
                        : theme == Theme::Dark ? core::Theme::Dark
                                               : core::Theme::Default;
    if (wanted == appearance_.theme) {
        return;
    }

    appearance_.theme = wanted;
    theAppearanceIsNow();
    askForAppearance();
}

SettingsViewModel::Notice SettingsViewModel::callNotice() const noexcept
{
    switch (appearance_.callNotice) {
    case core::CallNotice::Banner:
        return Notice::Banner;
    case core::CallNotice::Raise:
        return Notice::Raise;
    case core::CallNotice::Silent:
        return Notice::Silent;
    }
    return Notice::Banner;
}

void SettingsViewModel::chooseCallNotice(Notice notice)
{
    const auto wanted = notice == Notice::Raise    ? core::CallNotice::Raise
                        : notice == Notice::Silent ? core::CallNotice::Silent
                                                   : core::CallNotice::Banner;
    if (wanted == appearance_.callNotice) {
        return;
    }

    appearance_.callNotice = wanted;
    theAppearanceIsNow();
    askForAppearance();
}

SettingsViewModel::Glyphs SettingsViewModel::glyphs() const noexcept
{
    return appearance_.glyphs == core::Glyphs::Outline ? Glyphs::Outline : Glyphs::Solid;
}

QString SettingsViewModel::glyphSet() const
{
    return glyphs() == Glyphs::Outline ? QStringLiteral("qrc:/glyphs/regular")
                                       : QStringLiteral("qrc:/glyphs/filled");
}

void SettingsViewModel::chooseGlyphs(Glyphs glyphs)
{
    const auto wanted = glyphs == Glyphs::Outline ? core::Glyphs::Outline : core::Glyphs::Solid;
    if (wanted == appearance_.glyphs) {
        return;
    }

    appearance_.glyphs = wanted;
    theAppearanceIsNow();
    askForAppearance();
}

SettingsViewModel::Sketch SettingsViewModel::asSketch(core::Visualisation drawing) noexcept
{
    switch (drawing) {
    case core::Visualisation::Off:
        return Sketch::Nothing;
    case core::Visualisation::Levels:
        return Sketch::Levels;
    case core::Visualisation::Histogram:
        return Sketch::Histogram;
    case core::Visualisation::Wave:
        return Sketch::Wave;
    case core::Visualisation::Spectrum:
        return Sketch::Spectrum;
    case core::Visualisation::Aurora:
        return Sketch::Aurora;
    case core::Visualisation::Halo:
        return Sketch::Halo;
    case core::Visualisation::Uprights:
        return Sketch::Uprights;
    }
    return Sketch::Uprights;
}

core::Visualisation SettingsViewModel::asVisualisation(Sketch sketch) noexcept
{
    switch (sketch) {
    case Sketch::Nothing:
        return core::Visualisation::Off;
    case Sketch::Levels:
        return core::Visualisation::Levels;
    case Sketch::Histogram:
        return core::Visualisation::Histogram;
    case Sketch::Wave:
        return core::Visualisation::Wave;
    case Sketch::Spectrum:
        return core::Visualisation::Spectrum;
    case Sketch::Aurora:
        return core::Visualisation::Aurora;
    case Sketch::Halo:
        return core::Visualisation::Halo;
    case Sketch::Uprights:
        return core::Visualisation::Uprights;
    }
    return core::Visualisation::Uprights;
}

QVariantList SettingsViewModel::sketchesInARow()
{

    static constexpr std::array kEvery = {
        core::Visualisation::Off,  core::Visualisation::Levels,   core::Visualisation::Histogram,
        core::Visualisation::Wave, core::Visualisation::Spectrum, core::Visualisation::Aurora,
        core::Visualisation::Halo, core::Visualisation::Uprights};

    QVariantList fitting;
    for (const auto drawing : kEvery) {
        if (core::fitsInARow(drawing)) {
            fitting.append(QVariant::fromValue(static_cast<int>(asSketch(drawing))));
        }
    }
    return fitting;
}

void SettingsViewModel::setCaptureDrawingCallback(CaptureDrawingCallback draw)
{
    drawCapture_ = std::move(draw);
}

void SettingsViewModel::useCaptureDrawing(Sketch sketch)
{
    if (drawCapture_) {
        drawCapture_(asVisualisation(sketch));
    }
}

SettingsViewModel::Sketch SettingsViewModel::captureSketch() const noexcept
{
    return asSketch(capture_.pageDrawing);
}

SettingsViewModel::Sketch SettingsViewModel::captureStripSketch() const noexcept
{
    return asSketch(capture_.stripDrawing);
}

void SettingsViewModel::chooseCaptureSketch(Sketch sketch)
{
    const auto wanted = asVisualisation(sketch);
    if (wanted == capture_.pageDrawing) {
        return;
    }
    capture_.pageDrawing = wanted;
    Q_EMIT captureChanged();
    askForCapture();
}

void SettingsViewModel::chooseCaptureStripSketch(Sketch sketch)
{
    const auto wanted = asVisualisation(sketch);
    if (wanted == capture_.stripDrawing) {
        return;
    }
    capture_.stripDrawing = wanted;
    Q_EMIT captureChanged();
    askForCapture();
}

SettingsViewModel::Sketch SettingsViewModel::sketch() const noexcept
{
    switch (appearance_.visualisation) {
    case core::Visualisation::Off:
        return Sketch::Nothing;
    case core::Visualisation::Levels:
        return Sketch::Levels;
    case core::Visualisation::Histogram:
        return Sketch::Histogram;
    case core::Visualisation::Wave:
        return Sketch::Wave;
    case core::Visualisation::Spectrum:
        return Sketch::Spectrum;
    case core::Visualisation::Aurora:
        return Sketch::Aurora;
    case core::Visualisation::Halo:
        return Sketch::Halo;
    case core::Visualisation::Uprights:
        return Sketch::Uprights;
    }
    return Sketch::Nothing;
}

void SettingsViewModel::chooseSketch(Sketch sketch)
{
    const auto wanted = [sketch]() {
        switch (sketch) {
        case Sketch::Nothing:
            return core::Visualisation::Off;
        case Sketch::Levels:
            return core::Visualisation::Levels;
        case Sketch::Histogram:
            return core::Visualisation::Histogram;
        case Sketch::Wave:
            return core::Visualisation::Wave;
        case Sketch::Spectrum:
            return core::Visualisation::Spectrum;
        case Sketch::Aurora:
            return core::Visualisation::Aurora;
        case Sketch::Halo:
            return core::Visualisation::Halo;
        case Sketch::Uprights:
            return core::Visualisation::Uprights;
        }
        return core::Visualisation::Off;
    }();

    if (wanted == appearance_.visualisation) {
        return;
    }

    appearance_.visualisation = wanted;
    theAppearanceIsNow();
    askForAppearance();
}

void SettingsViewModel::chooseEndedCallSeconds(int seconds)
{
    const auto wanted = std::clamp(seconds, core::kMinEndedCallSeconds, core::kMaxEndedCallSeconds);
    if (wanted == appearance_.endedCallSeconds) {
        return;
    }

    appearance_.endedCallSeconds = wanted;
    theAppearanceIsNow();
    askForAppearance();
}

SettingsViewModel::IconScale SettingsViewModel::iconScale() const noexcept
{
    return appearance_.glyphSize == core::GlyphSize::Large ? IconScale::Large : IconScale::Normal;
}

void SettingsViewModel::chooseIconScale(IconScale scale)
{
    const auto wanted =
        scale == IconScale::Large ? core::GlyphSize::Large : core::GlyphSize::Normal;
    if (wanted == appearance_.glyphSize) {
        return;
    }

    appearance_.glyphSize = wanted;
    theAppearanceIsNow();
    askForAppearance();
}

void SettingsViewModel::askForAppearance()
{
    if (!dispatch_ || !chooseAppearance_) {
        return;
    }
    dispatch_([choose = chooseAppearance_, wanted = appearance_]() { choose(wanted); });
}

void SettingsViewModel::setCanTest(bool allowed)
{
    if (canTest_ == allowed) {
        return;
    }
    canTest_ = allowed;
    Q_EMIT canTestChanged();
}

void SettingsViewModel::playTest(core::ToneOutput output, std::string file, core::ToneLevel level)
{
    if (!dispatch_ || !probe_.play || file.empty()) {
        return;
    }

    const bool ring = level == core::ToneLevel::Ring;
    if (ring != ringPlaying_) {
        ringPlaying_ = ring;
        Q_EMIT ringPlayingChanged();
    }

    dispatch_([play = probe_.play, file = std::move(file), output, level]() {
        play(output, file, level);
    });
}

void SettingsViewModel::stopTest()
{
    if (dispatch_ && probe_.stop) {
        dispatch_([stop = probe_.stop]() { stop(); });
    }

    if (ringPlaying_) {
        ringPlaying_ = false;
        Q_EMIT ringPlayingChanged();
    }
}

void SettingsViewModel::reportToneEnded()
{

    if (ringPlaying_) {
        ringPlaying_ = false;
        Q_EMIT ringPlayingChanged();
    }
}

void SettingsViewModel::testSpeakers()
{
    if (!canTest_) {
        return;
    }

    playTest(core::ToneOutput::Speakers, testSoundFile(), core::ToneLevel::Conversation);
}

void SettingsViewModel::testRinger()
{
    if (!canTest_) {
        return;
    }

    playTest(core::ToneOutput::Ringer, ringtoneFile(audio_.ringtone), core::ToneLevel::Ring);
}

void SettingsViewModel::watchMicrophone(bool watching)
{
    if (!dispatch_ || !probe_.meter) {
        return;
    }

    if (watching == sampler_.isActive()) {
        return;
    }

    if (watching) {
        sampler_.start();
        dispatch_([meter = probe_.meter]() { meter(true); });
        return;
    }

    sampler_.stop();
    dispatch_([meter = probe_.meter]() { meter(false); });

    meter_.reset();
    Q_EMIT levelChanged();
}

void SettingsViewModel::sample()
{
    if (!dispatch_ || !probe_.sample) {
        return;
    }
    dispatch_([sample = probe_.sample]() { sample(); });
}

void SettingsViewModel::reportLevel(unsigned level)
{

    if (!sampler_.isActive()) {
        return;
    }

    meter_.observe(level);
    Q_EMIT levelChanged();
}

void SettingsViewModel::withdrawPending()
{
    if (pending_.empty() || !dispatch_ || !withdraw_) {
        return;
    }

    dispatch_([withdraw = withdraw_]() { withdraw(); });
}

void SettingsViewModel::setRecordingCallback(RecordingCallback choose)
{
    chooseRecording_ = std::move(choose);
}

void SettingsViewModel::applyRecording(const core::RecordingSettings &recording)
{
    if (recording == recording_) {
        return;
    }
    recording_ = recording;
    Q_EMIT recordingChanged();
}

void SettingsViewModel::setTranscriptionCallback(TranscriptionCallback choose)
{
    chooseTranscription_ = std::move(choose);
}

void SettingsViewModel::applyTranscription(const ui::TranscriptionSettings &transcription)
{
    if (transcription == transcription_) {
        return;
    }
    transcription_ = transcription;
    Q_EMIT transcriptionChanged();
}

void SettingsViewModel::askForTranscription()
{
    if (!dispatch_ || !chooseTranscription_) {
        return;
    }

    auto wanted = transcription_;
    wanted.language = QString::fromStdString(wanted.language).trimmed().toStdString();

    dispatch_([choose = chooseTranscription_, wanted = std::move(wanted)]() { choose(wanted); });
}

namespace {

[[nodiscard]] std::int64_t ceilingMicros(const QString &text)
{
    const auto trimmed = text.trimmed();
    if (trimmed.isEmpty()) {
        return 0;
    }
    bool ok = false;
    const auto value = trimmed.toDouble(&ok);
    if (!ok || value <= 0.0) {
        return 0;
    }
    return static_cast<std::int64_t>(std::llround(value * 1'000'000.0));
}

[[nodiscard]] QString ceilingText(std::int64_t micros)
{
    if (micros <= 0) {
        return {};
    }
    auto text = QString::number(static_cast<double>(micros) / 1'000'000.0, 'f', 2);
    while (text.endsWith(QLatin1Char{'0'})) {
        text.chop(1);
    }
    if (text.endsWith(QLatin1Char{'.'})) {
        text.chop(1);
    }
    return text;
}

}

QString SettingsViewModel::monthlyCeiling() const
{
    return ceilingText(pipeline_.monthlyCeilingMicros);
}

QString SettingsViewModel::monthlyTokenCeiling() const
{
    return pipeline_.monthlyCeilingTokens <= 0 ? QString{}
                                               : QString::number(pipeline_.monthlyCeilingTokens);
}

void SettingsViewModel::setPipelineCallback(PipelineCallback choose)
{
    choosePipeline_ = std::move(choose);
}

void SettingsViewModel::applyPipeline(const ui::PipelineSettings &pipeline)
{
    if (pipeline == pipeline_) {
        return;
    }
    pipeline_ = pipeline;
    Q_EMIT pipelineChanged();
}

void SettingsViewModel::setRealtimeCallback(RealtimeCallback choose)
{
    chooseRealtime_ = std::move(choose);
}

void SettingsViewModel::applyRealtime(const ui::RealtimeSettings &realtime)
{
    if (realtime == realtime_) {
        return;
    }
    realtime_ = realtime;
    Q_EMIT realtimeChanged();
}

void SettingsViewModel::askForRealtime()
{
    if (!dispatch_ || !chooseRealtime_) {
        return;
    }
    dispatch_([this, wanted = realtime_] { chooseRealtime_(wanted); });
}

QString SettingsViewModel::realtimeRecogniserCeiling() const
{
    return ceilingText(realtime_.monthlyRecogniserMicros);
}

QString SettingsViewModel::realtimeModelCeiling() const
{
    return ceilingText(realtime_.monthlyModelMicros);
}

void SettingsViewModel::setPrompterOffered(bool wanted)
{
    if (wanted == realtime_.offered) {
        return;
    }
    realtime_.offered = wanted;
    Q_EMIT realtimeChanged();
    askForRealtime();
}

void SettingsViewModel::setRealtimeRecogniserCeiling(const QString &amount)
{
    const auto wanted = ceilingMicros(amount);
    if (wanted == realtime_.monthlyRecogniserMicros) {
        return;
    }
    realtime_.monthlyRecogniserMicros = wanted;
    Q_EMIT realtimeChanged();
    askForRealtime();
}

void SettingsViewModel::setRealtimeModelCeiling(const QString &amount)
{
    const auto wanted = ceilingMicros(amount);
    if (wanted == realtime_.monthlyModelMicros) {
        return;
    }
    realtime_.monthlyModelMicros = wanted;
    Q_EMIT realtimeChanged();
    askForRealtime();
}

void SettingsViewModel::setPrompterColumnText(int pixels)
{
    const auto wanted = std::clamp(pixels, ui::kLeastColumnTextPixels, ui::kMostColumnTextPixels);
    if (wanted == realtime_.columnTextPixels) {
        return;
    }
    realtime_.columnTextPixels = wanted;
    Q_EMIT realtimeChanged();
    askForRealtime();
}

void SettingsViewModel::setPrompterRepeatsTheLast(bool wanted)
{
    if (wanted == realtime_.repeatTheLast) {
        return;
    }
    realtime_.repeatTheLast = wanted;
    Q_EMIT realtimeChanged();
    askForRealtime();
}

void SettingsViewModel::setPrompterRepeatText(int pixels)
{
    const auto wanted = std::clamp(pixels, ui::kLeastRepeatTextPixels, ui::kMostRepeatTextPixels);
    if (wanted == realtime_.repeatTextPixels) {
        return;
    }
    realtime_.repeatTextPixels = wanted;
    Q_EMIT realtimeChanged();
    askForRealtime();
}

void SettingsViewModel::applySpend(const QString &line, bool paused)
{
    if (line == monthSoFar_ && paused == paused_) {
        return;
    }
    monthSoFar_ = line;
    paused_ = paused;
    Q_EMIT spendChanged();
}

void SettingsViewModel::askForPipeline()
{
    if (!dispatch_ || !choosePipeline_) {
        return;
    }

    auto wanted = pipeline_;
    wanted.currency = QString::fromStdString(wanted.currency).trimmed().toStdString();
    dispatch_([this, wanted]() { choosePipeline_(wanted); });
}

void SettingsViewModel::setCaptureCallback(CaptureCallback choose)
{
    chooseCapture_ = std::move(choose);
}

void SettingsViewModel::setCaptureCommand(CaptureCommand run)
{
    runCapture_ = std::move(run);
}

void SettingsViewModel::applyCapture(const ui::CaptureSettings &capture)
{
    if (capture == capture_) {
        return;
    }
    capture_ = capture;
    Q_EMIT captureChanged();
    refreshCaptureSampler();
}

void SettingsViewModel::applyCaptureState(bool available, bool watching, bool running,
                                          const QString &what, bool heldOff)
{
    if (available == captureAvailable_ && watching == captureWatching_ &&
        running == captureRunning_ && what == captureWhat_ && heldOff == captureHeldOff_) {
        return;
    }
    captureAvailable_ = available;
    captureWatching_ = watching;
    captureRunning_ = running;
    captureWhat_ = what;
    captureHeldOff_ = heldOff;
    Q_EMIT captureStateChanged();
    refreshCaptureSampler();
}

void SettingsViewModel::refreshCaptureSampler()
{

    const bool wanted = captureAvailable_ && capture_.enabled;
    if (wanted == captureSampler_.isActive()) {
        return;
    }
    if (wanted) {
        captureSampler_.start();
        return;
    }
    captureSampler_.stop();
    captureNear_.reset();
    captureFar_.reset();
    Q_EMIT captureLevelChanged();
}

void SettingsViewModel::setCaptureLevelSource(CaptureLevelSource take)
{
    takeCaptureLevels_ = std::move(take);
}

void SettingsViewModel::applyCaptureLevels(unsigned near, unsigned far)
{
    captureNear_.observe(near);
    captureFar_.observe(far);
    Q_EMIT captureLevelChanged();
}

void SettingsViewModel::askForCapture()
{
    if (!dispatch_ || !chooseCapture_) {
        return;
    }

    auto wanted = capture_;
    wanted.outputDevice = QString::fromStdString(wanted.outputDevice).trimmed().toStdString();
    wanted.inputDevice = QString::fromStdString(wanted.inputDevice).trimmed().toStdString();
    dispatch_([this, wanted]() { chooseCapture_(wanted); });
}

void SettingsViewModel::setCaptureEnabled(bool enabled)
{
    if (enabled == capture_.enabled) {
        return;
    }
    capture_.enabled = enabled;
    Q_EMIT captureChanged();
    askForCapture();
}

void SettingsViewModel::setCapturePreRoll(int seconds)
{

    const auto wanted =
        static_cast<unsigned>(std::clamp(seconds, 0, static_cast<int>(kMaxCapturePreRollSeconds)));
    if (wanted == capture_.preRollSeconds) {
        return;
    }
    capture_.preRollSeconds = wanted;
    Q_EMIT captureChanged();
    askForCapture();
}

void SettingsViewModel::setCaptureRemind(bool remind)
{
    if (remind == capture_.remind) {
        return;
    }
    capture_.remind = remind;
    Q_EMIT captureChanged();
    askForCapture();
}

void SettingsViewModel::setCaptureNotACall(CaptureNotACall run)
{
    notACall_ = std::move(run);
}

void SettingsViewModel::sayFound(const QString &what, const QString &process)
{
    Q_EMIT captureFound(what, process);
}

void SettingsViewModel::sayLost()
{
    Q_EMIT captureLost();
}

void SettingsViewModel::applyCapturePolicies(
    std::vector<plugins::capture::ApplicationPolicy> policies)
{
    if (policies == capture_.perApplication) {
        return;
    }
    capture_.perApplication = std::move(policies);
    Q_EMIT captureChanged();

    askForCapture();
}

void SettingsViewModel::setCaptureElapsed(CaptureElapsed ask)
{
    captureElapsed_ = std::move(ask);
}

int SettingsViewModel::captureElapsedSeconds() const
{
    return captureElapsed_ ? captureElapsed_() : 0;
}

namespace {

[[nodiscard]] SettingsViewModel::OnFinding asQml(plugins::capture::OnFinding answer)
{
    switch (answer) {
    case plugins::capture::OnFinding::Never:
        return SettingsViewModel::OnFinding::Never;
    case plugins::capture::OnFinding::Ask:
        return SettingsViewModel::OnFinding::Ask;
    case plugins::capture::OnFinding::Record:
        return SettingsViewModel::OnFinding::Record;
    }
    return SettingsViewModel::OnFinding::Ask;
}

[[nodiscard]] plugins::capture::OnFinding fromQml(SettingsViewModel::OnFinding answer)
{
    switch (answer) {
    case SettingsViewModel::OnFinding::Never:
        return plugins::capture::OnFinding::Never;
    case SettingsViewModel::OnFinding::Ask:
        return plugins::capture::OnFinding::Ask;
    case SettingsViewModel::OnFinding::Record:
        return plugins::capture::OnFinding::Record;
    }
    return plugins::capture::OnFinding::Ask;
}

}

bool SettingsViewModel::notACall(const QString &recordingId)
{
    return notACall_ ? notACall_(recordingId) : false;
}

QStringList SettingsViewModel::captureExceptions() const
{
    QStringList all;
    all.reserve(static_cast<qsizetype>(capture_.perApplication.size()));
    for (const auto &one : capture_.perApplication) {
        all.append(QString::fromStdString(one.process));
    }
    return all;
}

SettingsViewModel::OnFinding SettingsViewModel::captureOnFinding() const noexcept
{
    return asQml(capture_.onFinding);
}

void SettingsViewModel::setCaptureOnFinding(OnFinding answer)
{
    const auto wanted = fromQml(answer);
    if (wanted == capture_.onFinding) {
        return;
    }
    capture_.onFinding = wanted;
    Q_EMIT captureChanged();
    askForCapture();
}

SettingsViewModel::OnFinding SettingsViewModel::capturePolicyFor(const QString &process) const
{
    const auto wanted = process.toStdString();
    const auto where = std::ranges::find_if(
        capture_.perApplication, [&wanted](const plugins::capture::ApplicationPolicy &one) {
            return one.process == wanted;
        });
    return where == capture_.perApplication.end() ? asQml(capture_.onFinding)
                                                  : asQml(where->answer);
}

void SettingsViewModel::setCapturePolicyFor(const QString &process, OnFinding answer)
{
    const auto wanted = process.toStdString();
    if (wanted.empty()) {
        return;
    }
    const auto said = fromQml(answer);
    const auto where = std::ranges::find_if(
        capture_.perApplication, [&wanted](const plugins::capture::ApplicationPolicy &one) {
            return one.process == wanted;
        });
    if (where == capture_.perApplication.end()) {
        capture_.perApplication.push_back(plugins::capture::ApplicationPolicy{wanted, said});
    } else if (where->answer == said) {
        return;
    } else {
        where->answer = said;
    }
    Q_EMIT captureChanged();
    askForCapture();
}

void SettingsViewModel::captureFollowsTheDefault(const QString &process)
{
    const auto wanted = process.toStdString();
    const auto where = std::ranges::find_if(
        capture_.perApplication, [&wanted](const plugins::capture::ApplicationPolicy &one) {
            return one.process == wanted;
        });
    if (where == capture_.perApplication.end()) {
        return;
    }
    capture_.perApplication.erase(where);
    Q_EMIT captureChanged();
    askForCapture();
}

bool SettingsViewModel::startRecordingHere(const QString &what)
{
    return runCapture_ ? runCapture_(true, what, QString{}) : false;
}

bool SettingsViewModel::startCaptureOf(const QString &what, const QString &process)
{
    return runCapture_ ? runCapture_(true, what, process) : false;
}

void SettingsViewModel::setCaptureRename(CaptureRename run)
{
    renameCapture_ = std::move(run);
}

bool SettingsViewModel::renameCapture(const QString &what)
{
    const auto called = what.trimmed();
    if (called.isEmpty() || !renameCapture_) {
        return false;
    }
    return renameCapture_(called);
}

void SettingsViewModel::stopRecordingHere()
{
    if (runCapture_) {
        static_cast<void>(runCapture_(false, {}, {}));
    }
}

void SettingsViewModel::setProcessAutomatically(bool automatic)
{
    if (automatic == pipeline_.automatic) {
        return;
    }
    pipeline_.automatic = automatic;
    Q_EMIT pipelineChanged();
    askForPipeline();
}

void SettingsViewModel::setMonthlyCeiling(const QString &amount)
{
    const auto wanted = ceilingMicros(amount);
    if (wanted == pipeline_.monthlyCeilingMicros) {
        return;
    }
    pipeline_.monthlyCeilingMicros = wanted;
    Q_EMIT pipelineChanged();
    askForPipeline();
}

void SettingsViewModel::setMonthlyTokenCeiling(const QString &amount)
{
    bool ok = false;
    const auto value = amount.trimmed().toLongLong(&ok);
    const auto wanted = ok && value > 0 ? value : 0;
    if (wanted == pipeline_.monthlyCeilingTokens) {
        return;
    }
    pipeline_.monthlyCeilingTokens = wanted;
    Q_EMIT pipelineChanged();
    askForPipeline();
}

void SettingsViewModel::setTranscribeAutomatically(bool automatic)
{
    if (automatic == transcription_.automatic) {
        return;
    }
    transcription_.automatic = automatic;
    Q_EMIT transcriptionChanged();
    askForTranscription();
}

void SettingsViewModel::setTranscriptionLanguage(const QString &code)
{
    const auto wanted = code.toStdString();
    if (wanted == transcription_.language) {
        return;
    }
    transcription_.language = wanted;
    Q_EMIT transcriptionChanged();
    askForTranscription();
}

QString SettingsViewModel::recordingLibrary() const
{
    if (!recording_.library.empty()) {
        return QString::fromStdString(recording_.library);
    }
    return QString::fromStdString(core::asUtf8(core::recordingsDirectory()));
}

void SettingsViewModel::askForRecording()
{
    if (!dispatch_ || !chooseRecording_) {
        return;
    }

    auto wanted = recording_;
    wanted.announcement = QString::fromStdString(wanted.announcement).trimmed().toStdString();

    dispatch_([choose = chooseRecording_, wanted = std::move(wanted)]() { choose(wanted); });
}

void SettingsViewModel::setRecordingPolicy(int policy)
{
    const auto wanted = static_cast<core::RecordingPolicy>(policy);
    if (wanted == recording_.policy) {
        return;
    }
    recording_.policy = wanted;
    Q_EMIT recordingChanged();
    askForRecording();
}

void SettingsViewModel::setRecordingConsent(int consent)
{
    const auto wanted = static_cast<core::ConsentPosture>(consent);
    if (wanted == recording_.consent) {
        return;
    }
    recording_.consent = wanted;
    Q_EMIT recordingChanged();
    askForRecording();
}

bool SettingsViewModel::recordsAccount(const QString &accountId) const
{
    const auto wanted = accountId.toStdString();
    return std::find(recording_.accounts.begin(), recording_.accounts.end(), wanted) !=
           recording_.accounts.end();
}

void SettingsViewModel::setRecordsAccount(const QString &accountId, bool records)
{
    const auto wanted = accountId.toStdString();
    const auto at = std::find(recording_.accounts.begin(), recording_.accounts.end(), wanted);

    if (records == (at != recording_.accounts.end())) {
        return;
    }

    if (records) {
        recording_.accounts.push_back(wanted);
    } else {
        recording_.accounts.erase(at);
    }

    Q_EMIT recordingChanged();
    askForRecording();
}

void SettingsViewModel::chooseAnnouncementFile(const QUrl &url)
{
    const auto path = url.isLocalFile() ? url.toLocalFile() : url.toString();
    if (path.isEmpty()) {
        return;
    }

    const auto wanted = path.toStdString();
    if (wanted == recording_.announcement) {
        return;
    }

    recording_.announcement = wanted;
    Q_EMIT recordingChanged();
    askForRecording();
}

void SettingsViewModel::useTheShippedChime()
{
    if (recording_.announcement.empty()) {
        return;
    }

    recording_.announcement.clear();
    Q_EMIT recordingChanged();
    askForRecording();
}

void SettingsViewModel::setRecordingAnnouncement(const QString &file)
{
    const auto wanted = file.toStdString();
    if (wanted == recording_.announcement) {
        return;
    }
    recording_.announcement = wanted;
    Q_EMIT recordingChanged();
    askForRecording();
}

void SettingsViewModel::setRecordingToneInterval(int seconds)
{
    const auto wanted = std::clamp(seconds, core::kMinToneInterval, core::kMaxToneInterval);
    if (wanted == recording_.toneIntervalSeconds) {
        return;
    }
    recording_.toneIntervalSeconds = wanted;
    Q_EMIT recordingChanged();
    askForRecording();
}

void SettingsViewModel::setRecordingKeepStems(bool keeping)
{
    if (keeping == recording_.keepStems) {
        return;
    }
    recording_.keepStems = keeping;
    Q_EMIT recordingChanged();
    askForRecording();
}

void SettingsViewModel::setRecordingKeepNotices(bool keeping)
{
    if (keeping == recording_.keepNotices) {
        return;
    }
    recording_.keepNotices = keeping;
    Q_EMIT recordingChanged();
    askForRecording();
}

void SettingsViewModel::setRecordingMaxAgeDays(int days)
{
    const auto wanted = std::clamp(days, 0, core::kMaxRetentionDays);
    if (wanted == recording_.retention.maxAgeDays) {
        return;
    }
    recording_.retention.maxAgeDays = wanted;
    Q_EMIT recordingChanged();
    askForRecording();
}

void SettingsViewModel::setRecordingMaxSize(int megabytes)
{
    const auto wanted = std::clamp(megabytes, 0, core::kMaxLibraryMegabytes);
    if (wanted == recording_.retention.maxSizeMegabytes) {
        return;
    }
    recording_.retention.maxSizeMegabytes = wanted;
    Q_EMIT recordingChanged();
    askForRecording();
}

void SettingsViewModel::setRecordingStemDays(int days)
{
    const auto wanted = std::clamp(days, 0, core::kMaxRetentionDays);
    if (wanted == recording_.retention.dropStemsAfterDays) {
        return;
    }
    recording_.retention.dropStemsAfterDays = wanted;
    Q_EMIT recordingChanged();
    askForRecording();
}

void SettingsViewModel::setRecordingFreeSpaceFloor(int megabytes)
{
    const auto wanted =
        std::clamp(megabytes, core::kMinFreeSpaceMegabytes, core::kMaxFreeSpaceMegabytes);
    if (wanted == recording_.retention.freeSpaceFloorMegabytes) {
        return;
    }
    recording_.retention.freeSpaceFloorMegabytes = wanted;
    Q_EMIT recordingChanged();
    askForRecording();
}

}
