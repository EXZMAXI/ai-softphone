// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "accounts_view_model.hpp"

#include "api_handlers.hpp"

#include "autostart.hpp"
#include "global_hotkeys.hpp"
#include "link_handler.hpp"
#include "menu_entry.hpp"
#include "plugin_directory.hpp"
#include "plugins_view_model.hpp"

#include "artifacts_view_model.hpp"
#include "automations_view_model.hpp"
#include "buttons_view_model.hpp"
#include "call_view_model.hpp"
#include "contacts_view_model.hpp"
#include "dashboard_view_model.hpp"
#include "desktop.hpp"
#include "desktop_colours.hpp"
#include "desktop_platform.hpp"
#include "diagnostics_view_model.hpp"
#include "events.hpp"
#include "history_view_model.hpp"
#include "jobs_view_model.hpp"
#include "session_away.hpp"
#include "wheel_acceleration.hpp"

#include "demo_calls.hpp"
#include "demo_capture.hpp"
#include "demo_prompter.hpp"
#include "providers_view_model.hpp"
#include "recordings_view_model.hpp"
#include "screenshots.hpp"
#include "sections.hpp"
#include "storage_view_model.hpp"
#include "taxonomy_view_model.hpp"
#include "templates_view_model.hpp"
#include "vocabulary.hpp"
#include <softphone/container/loader.hpp>
#include <softphone/shell/ui_loader.hpp>

#include <softphone/core/telemetry_service.hpp>

#include <softphone/plugins/capture/control.hpp>
#include <softphone/plugins/capture/names.hpp>
#include <softphone/plugins/diagnostics/plugin.hpp>
#include <softphone/plugins/diagnostics/ui.hpp>
#include <softphone/plugins/integration/plugin.hpp>
#include <softphone/plugins/integration/ui.hpp>
#include <softphone/plugins/monolith/plugin.hpp>
#include <softphone/plugins/processing/plugin.hpp>
#include <softphone/plugins/realtime/plugin.hpp>
#include <softphone/plugins/realtime/session.hpp>
#include <softphone/plugins/recording/consent_policy.hpp>
#include <softphone/plugins/recording/plugin.hpp>
#include <softphone/plugins/softphone/plugin.hpp>
#include <softphone/plugins/softphone/ui.hpp>
#include <softphone/plugins/storage/plugin.hpp>
#include <softphone/plugins/taxonomy/events.hpp>
#include <softphone/plugins/taxonomy/plugin.hpp>
#include <softphone/plugins/transcription/plugin.hpp>
#include <softphone/plugins/webui/plugin.hpp>

#include "about.hpp"
#include "plugin_settings.hpp"
#include "settings_sink.hpp"
#include "settings_view_model.hpp"
#include "signal_drawing.hpp"
#include "signal_geometry.hpp"
#include "single_instance.hpp"
#include "sound_view_model.hpp"
#include "sounds.hpp"
#include "startup_state.hpp"
#include "transcript_view_model.hpp"

#if defined(__APPLE__)
#include "dock_reopen_apple.hpp"
#include "tray_activation_apple.hpp"
#endif
#include <softphone/core/generation_service.hpp>
#include <softphone/core/pipeline_engine.hpp>
#include <softphone/core/provider_knobs.hpp>
#include <softphone/core/settings_groups.hpp>
#include <softphone/core/transcription_service.hpp>
#include <softphone/http/anthropic_chat.hpp>
#include <softphone/http/assemblyai_asr.hpp>
#include <softphone/http/deepgram_asr.hpp>
#include <softphone/http/elevenlabs_asr.hpp>
#include <softphone/http/httplib_http_client.hpp>
#include <softphone/http/ix_web_socket.hpp>
#include <softphone/http/nghttp2_grpc.hpp>
#include <softphone/http/openai_asr.hpp>
#include <softphone/http/openai_chat.hpp>
#include <softphone/http/soniox_asr.hpp>
#include <softphone/http/speechmatics_asr.hpp>
#include <softphone/http/streaming_recognisers.hpp>
#include <softphone/http/streaming_yandex.hpp>
#include <softphone/http/xai_asr.hpp>
#include <softphone/http/yandex_asr.hpp>
#include <softphone/media/asr_audio.hpp>
#include <softphone/media/mp3.hpp>
#include <softphone/media/sound_file.hpp>
#include <softphone/media/wav.hpp>

#include "phone_shape.hpp"
#include "prompter_view_model.hpp"
#include "telemetry_view_model.hpp"
#include "translations.hpp"
#include "tray_view_model.hpp"
#include "update_view_model.hpp"
#include "window_memory.hpp"
#include "window_shape.hpp"
#include <algorithm>
#include <chrono>
#include <softphone/core/call_historian.hpp>
#include <softphone/core/crash.hpp>
#include <softphone/core/dial_link.hpp>
#include <softphone/core/joining_thread.hpp>
#include <softphone/core/logging.hpp>
#include <softphone/core/metrics.hpp>
#include <softphone/core/paths.hpp>
#include <softphone/core/recording_library.hpp>
#include <softphone/core/rehearsal_feed.hpp>
#include <softphone/core/streamed_transcription.hpp>
#include <softphone/core/uuid.hpp>
#include <softphone/core/webhook_notifier.hpp>
#include <softphone/http/httplib_transport.hpp>
#include <softphone/http/httplib_update_transport.hpp>
#include <softphone/http/openssl_signatures.hpp>
#include <softphone/media/waveform.hpp>
#include <softphone/storage/sqlite_store.hpp>

#include <softphone/core/updates.hpp>
#include <softphone/core/version.hpp>

#include <QDateTime>
#include <QDesktopServices>
#include <QEvent>
#include <QFileOpenEvent>
#include <QGuiApplication>
#include <QIcon>
#include <QLocale>
#include <QMetaObject>
#include <QNetworkInformation>
#include <QPointer>
#include <QProcess>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlError>
#include <QQmlIncubator>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QString>
#include <QStyleHints>
#include <QThreadPool>
#include <QTimer>
#include <QUrl>

#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <optional>
#include <set>
#include <span>
#include <string_view>
#include <thread>
#include <tuple>
#include <utility>
#include <vector>

namespace {

[[nodiscard]] QStringList &qtComplaints()
{
    static QStringList kept;
    return kept;
}

QtMessageHandler previousQtHandler = nullptr;

void carryQtMessages(QtMsgType type, const QMessageLogContext &context, const QString &message)
{

    const bool submenuAskedTheTheme =
        message.contains(QStringLiteral("No native Menu implementation available"));

    const auto level = [type, submenuAskedTheTheme]() {
        if (submenuAskedTheTheme) {
            return softphone::core::LogLevel::Debug;
        }
        switch (type) {
        case QtDebugMsg:
            return softphone::core::LogLevel::Debug;
        case QtInfoMsg:
            return softphone::core::LogLevel::Info;
        case QtWarningMsg:
            return softphone::core::LogLevel::Warning;
        case QtCriticalMsg:
        case QtFatalMsg:
            return softphone::core::LogLevel::Error;
        }
        return softphone::core::LogLevel::Info;
    }();

    softphone::core::Logger::instance().log(level, "qt", message.toStdString());

    if (!submenuAskedTheTheme &&
        (type == QtWarningMsg || type == QtCriticalMsg || type == QtFatalMsg)) {
        auto &kept = qtComplaints();
        kept.append(message);
        while (kept.size() > 20) {
            kept.removeFirst();
        }
    }

    if (previousQtHandler != nullptr) {
        previousQtHandler(type, context, message);
    }
}

softphone::ui::AccountsViewModel::Keyring keyringStateFor(softphone::secrets::SecretStatus status,
                                                          softphone::secrets::StoreKind kind)
{
    using Keyring = softphone::ui::AccountsViewModel::Keyring;

    const bool sealed = kind == softphone::secrets::StoreKind::Sealed;

    switch (status) {
    case softphone::secrets::SecretStatus::Ok:
    case softphone::secrets::SecretStatus::NotFound:
        return sealed ? Keyring::Sealed : Keyring::Working;
    case softphone::secrets::SecretStatus::Denied:

        return sealed ? Keyring::Unreadable : Keyring::Locked;
    case softphone::secrets::SecretStatus::ServiceUnavailable:
    case softphone::secrets::SecretStatus::Failed:
        break;
    }
    return Keyring::Unavailable;
}

class LeavesWhenAsked final : public QObject {
public:
    explicit LeavesWhenAsked(std::function<void()> leaving) : leaving_{std::move(leaving)} {}

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (event != nullptr && event->type() == QEvent::Quit) {
            leaving_();
        }
        return QObject::eventFilter(watched, event);
    }

private:
    std::function<void()> leaving_;
};

class TakesTheSplashAway final : public QObject {
public:
    TakesTheSplashAway(QQuickWindow *splash, std::function<void()> dismiss)
        : splash_{splash}, dismiss_{std::move(dismiss)}
    {}

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (event != nullptr && event->type() == QEvent::Expose && dismiss_) {
            auto *const window = qobject_cast<QQuickWindow *>(watched);
            if (window != nullptr && window != splash_ && window->isExposed()) {
                std::exchange(dismiss_, nullptr)();
            }
        }
        return QObject::eventFilter(watched, event);
    }

private:
    QQuickWindow *splash_{nullptr};
    std::function<void()> dismiss_;
};

class AnswersALink final : public QObject {
public:
    explicit AnswersALink(std::function<void(QString)> asked) : asked_{std::move(asked)} {}

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (event != nullptr && event->type() == QEvent::FileOpen) {
            const auto *const opening = static_cast<QFileOpenEvent *>(event);
            const auto link = softphone::core::dialLink(opening->url().toString().toStdString());
            if (link.ours && !link.target.empty() && asked_) {
                asked_(QString::fromStdString(link.target));
            }
        }
        return QObject::eventFilter(watched, event);
    }

private:
    std::function<void(QString)> asked_;
};

}

int main(int argc, char *argv[])
{

    if (const auto answered = softphone::core::answeredCrashDumpRequest(argc, argv)) {
        return *answered;
    }

    bool askedToQuit = false;

    std::string linkToDial;

    for (int argument = 1; argument < argc; ++argument) {
        const std::string_view given{argv[argument]};

        if (const auto link = softphone::core::dialLink(given); link.ours) {

            if (linkToDial.empty()) {
                linkToDial = link.target;
            }
            continue;
        }

        if (given == "--quit") {
            askedToQuit = true;
        }

        if (given == "--version") {
            std::cout << "ai-softphone " << softphone::core::versionString() << '\n';
            return EXIT_SUCCESS;
        }

        if (given == "--help") {
            std::cout << "ai-softphone " << softphone::core::versionString() << "\n\n"
                      << "A desktop SIP softphone. What it can be told is in its settings\n"
                         "window, and what it keeps is under the directories the desktop\n"
                         "gives it; the options here exist for the installer rather than\n"
                         "for a person.\n\n"
                         "  --quit          tell the phone this user is running to close,\n"
                         "                  and wait until it has\n"
                         "  --log-level     error, warning, info, debug or trace\n"
                         "  --version       print the version and exit\n"
                         "  --help          print this and exit\n";
            return EXIT_SUCCESS;
        }
    }

    softphone::core::installCrashHandler(softphone::core::stateDirectory());

    const auto startedAt = std::chrono::steady_clock::now();
    const auto sinceStart = [startedAt]() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::steady_clock::now() - startedAt)
            .count();
    };

    std::vector<std::string> earlyMarks;
    bool logIsOpen = false;
    const auto mark = [&sinceStart, &earlyMarks, &logIsOpen](const char *what) {
        auto line = std::string{"start: "} + what + " at " + std::to_string(sinceStart()) + " ms";
        if (logIsOpen) {
            softphone::core::Logger::instance().log(softphone::core::LogLevel::Info, "ui",
                                                    std::move(line));
            return;
        }
        earlyMarks.push_back(std::move(line));
    };
    const auto marksReachTheLog = [&earlyMarks, &logIsOpen]() {
        logIsOpen = true;
        for (const auto &line : earlyMarks) {
            softphone::core::Logger::instance().log(softphone::core::LogLevel::Info, "ui", line);
        }
        earlyMarks.clear();
    };

#if defined(Q_OS_LINUX)
    if (::qEnvironmentVariableIsEmpty("QT_QPA_PLATFORMTHEME")) {
        ::qputenv("QT_QPA_PLATFORMTHEME", "xdgdesktopportal");
        earlyMarks.emplace_back("file dialogs are asked of the desktop portal where one answers");
    }
#endif

    QGuiApplication app(argc, argv);

    previousQtHandler = qInstallMessageHandler(&carryQtMessages);

#if !defined(Q_OS_MACOS)
    QGuiApplication::setWindowIcon(QIcon{QStringLiteral(":/icons/ai-softphone.svg")});
#endif

#if defined(Q_OS_MACOS)
    const auto trayGuarded = softphone::ui::keepTheTrayFromAborting();
#endif

    QGuiApplication::setQuitOnLastWindowClosed(false);

    QGuiApplication::setOrganizationName(QStringLiteral("bpmkit"));
    QGuiApplication::setApplicationName(QStringLiteral("AI Softphone"));

    QGuiApplication::setDesktopFileName(QStringLiteral("ai-softphone"));

    const auto coreVersion = softphone::core::versionString();
    QGuiApplication::setApplicationVersion(
        QString::fromUtf8(coreVersion.data(), static_cast<qsizetype>(coreVersion.size())));

    switch (QGuiApplication::styleHints()->colorScheme()) {
    case Qt::ColorScheme::Dark:
        mark("the desktop asks for a dark interface");
        break;
    case Qt::ColorScheme::Light:
        mark("the desktop asks for a light interface");
        break;
    case Qt::ColorScheme::Unknown:
        mark("the desktop has no opinion about light or dark; following the system will be light");
        break;
    }

    const auto instanceName = softphone::ui::defaultInstanceName();

    if (askedToQuit) {
        using softphone::ui::QuitOutcome;

        switch (softphone::ui::askTheRunningPhoneToQuit(instanceName, std::chrono::seconds{30})) {
        case QuitOutcome::NobodyThere:
            std::cout << "No phone is running.\n";
            return EXIT_SUCCESS;
        case QuitOutcome::Left:
            std::cout << "The phone has closed.\n";
            return EXIT_SUCCESS;
        case QuitOutcome::StillRunning:
            std::cerr << "The phone was asked to close and is still running.\n";
            return EXIT_FAILURE;
        }
    }

    softphone::ui::SingleInstance instance{instanceName};
    if (!instance.claim(QString::fromStdString(linkToDial))) {
        std::cerr << (linkToDial.empty()
                          ? "The phone is already running; asked it to show itself.\n"
                          : "The phone is already running; handed it the number to dial.\n");
        return EXIT_SUCCESS;
    }

#if defined(Q_OS_MACOS)
    const auto dockHooked =
        softphone::ui::whenTheDockIconIsClicked([&instance] { instance.askedForTheWindow(); });
#endif

    softphone::ui::installTranslations(app,
                                       std::span<const softphone::shell::UiRegistry::Catalogue>{});

    QQuickStyle::setStyle(QStringLiteral("Material"));

    std::optional<softphone::core::LogLevel> level;

    bool startsHidden = false;

    const auto arguments = QGuiApplication::arguments();
    for (qsizetype i = 1; i < arguments.size(); ++i) {
        if (arguments.at(i) ==
            QString::fromUtf8(softphone::ui::kMinimisedArgument.data(),
                              static_cast<qsizetype>(softphone::ui::kMinimisedArgument.size()))) {
            startsHidden = true;
            continue;
        }

        if (arguments.at(i) != QStringLiteral("--log-level")) {
            continue;
        }

        const auto name = i + 1 < arguments.size() ? arguments.at(i + 1) : QString{};
        level = softphone::core::parseLogLevel(name.toStdString());
        if (!level.has_value()) {
            std::cerr << "--log-level takes one of: error, warning, info, debug, trace\n";
            return EXIT_FAILURE;
        }
    }

    softphone::container::Loader fromDisk{softphone::plugins::monolith::logThroughTheCore()};

    softphone::container::Host plugins{
        softphone::plugins::monolith::logThroughTheCore(), softphone::core::defaultSettingsFile(),
        softphone::core::restrictToOwner, std::string{softphone::core::kLegacySettingsSection}};
    softphone::plugins::monolith::MonolithPlugin monolith;

    softphone::plugins::softphone::EngineHost host{
        false,
        level,
        {},

        {},
        softphone::ui::contentLanguage({}),
        softphone::plugins::monolith::settingsDocument(*plugins.settings())};

#if defined(Q_OS_MACOS)
    {
        const auto moved = trayGuarded == softphone::ui::TrayGuard::qtHasMoved ||
                           dockHooked == softphone::ui::DockHook::qtHasMoved;
        const softphone::core::LogAttribute fields[] = {
            {"tray", softphone::ui::toString(trayGuarded)},
            {"dock", softphone::ui::toString(dockHooked)}};
        softphone::core::Logger::instance().log(moved ? softphone::core::LogLevel::Warning
                                                      : softphone::core::LogLevel::Info,
                                                "ui", "what this system's two hooks found", fields);
    }
#endif

    std::function<void(bool)> telephonePower;

    softphone::plugins::softphone::SoftphonePlugin telephone{&host, [&telephonePower](bool on) {
                                                                 if (telephonePower) {
                                                                     telephonePower(on);
                                                                 }
                                                             }};

    softphone::shell::UiRegistry uiRegistry;
    softphone::ui::registerSettingsGroups(uiRegistry);
    softphone::ui::registerMonolithSections(uiRegistry);
    softphone::ui::registerMonolithSlotItems(uiRegistry);
    softphone::ui::registerMonolithApiResources(uiRegistry);
    softphone::ui::registerMonolithOutboundEvents(uiRegistry);
    softphone::ui::registerMonolithCatalogue(uiRegistry);

    softphone::shell::UiLoader uiFromDisk{softphone::plugins::monolith::logThroughTheCore()};

    uiFromDisk.offerBuiltIn("integration", softphone::plugins::integration::contributeInterface);
    uiFromDisk.offerBuiltIn("diagnostics", softphone::plugins::diagnostics::contributeInterface);
    uiFromDisk.offerBuiltIn("softphone", softphone::plugins::softphone::contributeInterface);

    softphone::ui::PluginsViewModel pluginsModel;

    std::optional<QQmlApplicationEngine> engine;
    engine.emplace();
    mark("qt and the qml engine");

    const auto chosenLanguage = softphone::ui::installTranslations(
        app, uiRegistry,
        softphone::ui::offeredLanguage(
            uiRegistry, QString::fromStdString(
                            softphone::core::appearanceFrom(
                                softphone::ui::groupText(host.storedConfiguration(), "appearance"))
                                .language)));
    if (!chosenLanguage.isEmpty()) {
        engine->setUiLanguage(chosenLanguage);
    }

    const auto runningLayout = softphone::ui::shapeToRunIn(
        softphone::core::appearanceFrom(
            softphone::ui::groupText(host.storedConfiguration(), "appearance"))
            .layout);

    if (runningLayout == softphone::core::Layout::NoWindow) {
        startsHidden = true;
    }

    QString languageNow = chosenLanguage;

    const auto contentLanguage = softphone::ui::contentLanguage(QString::fromStdString(
        softphone::core::appearanceFrom(
            softphone::ui::groupText(host.storedConfiguration(), "appearance"))
            .language));

    std::cerr << "Logging to " << softphone::core::asUtf8(softphone::core::defaultLogFile())
              << '\n';

    softphone::ui::StartupState startupState;

    softphone::core::JoiningThread restarter;

    const bool telephoneWanted = [&plugins]() {
        if (plugins.settings() == nullptr) {
            return true;
        }
        const auto off = plugins.settings()->disabledPlugins();
        return std::ranges::find(off, std::string{softphone::plugins::softphone::kOwner}) ==
               off.end();
    }();

    softphone::core::JoiningThread starter{[&host, &app, &mark, &startupState, telephoneWanted]() {
        if (!telephoneWanted) {
            QMetaObject::invokeMethod(
                &app,
                [&startupState, &mark]() {
                    mark("the sip stack");

                    startupState.nothingToWaitFor();
                    softphone::core::Logger::instance().log(
                        softphone::core::LogLevel::Info, "ui",
                        "the telephone is off: the stack is not started");
                },
                Qt::QueuedConnection);
            return;
        }

        auto failure = host.start();
        QMetaObject::invokeMethod(
            &app,
            [&startupState, &mark, failure = std::move(failure)]() {
                mark("the sip stack");
                if (failure.has_value()) {
                    softphone::core::Logger::instance().log(softphone::core::LogLevel::Error, "ui",
                                                            "the sip stack would not start: " +
                                                                *failure);
                    startupState.failed(QString::fromStdString(*failure));
                    return;
                }
                startupState.succeeded();
            },
            Qt::QueuedConnection);
    }};

    softphone::ui::StorageViewModel storageModel;

    const auto offer = [&plugins](softphone::container::Manifest manifest,
                                  softphone::container::IPlugin *plugin, std::string_view what) {
        if (const auto added = plugins.add(std::move(manifest), plugin);
            added != softphone::container::AddProblem::None) {
            softphone::core::Logger::instance().log(softphone::core::LogLevel::Error, "container",
                                                    std::string{what} + ": " +
                                                        std::string{toString(added)});
        }
    };

    softphone::plugins::storage::StoragePlugin storagePlugin{
        {}, contentLanguage, [&storageModel, &app](softphone::core::StoreStatus opened) {
            QMetaObject::invokeMethod(
                &app, [&storageModel, opened]() { storageModel.applyStatus(opened); },
                Qt::QueuedConnection);
        }};

    offer(softphone::plugins::storage::storageManifest(), &storagePlugin,
          "the storage plugin was refused");

    softphone::plugins::integration::IntegrationPlugin outward{[&host]() {
        std::string token;
        static_cast<void>(
            host.keyring().fetch(std::string{softphone::core::secretKeys::kApiToken}, token));
        return token;
    }};

    offer(softphone::plugins::integration::integrationManifest(), &outward,
          "the integration plugin was refused");

    softphone::plugins::diagnostics::DiagnosticsPlugin whatWentWrong;

    offer(softphone::plugins::diagnostics::diagnosticsManifest(), &whatWentWrong,
          "the diagnostics plugin was refused");

    offer(softphone::plugins::softphone::softphoneManifest(), &telephone,
          "the telephone plugin was refused");

    const auto onDisk =
        fromDisk.load(softphone::ui::pluginDirectory(), plugins,
                      plugins.settings() != nullptr ? plugins.settings()->disabledPlugins()
                                                    : std::vector<std::string>{});
    static_cast<void>(onDisk);
    offer(softphone::plugins::monolith::monolithManifest(), &monolith,
          "the built-in plugin was refused");
    if (!plugins.start().clean()) {
        softphone::core::Logger::instance().log(softphone::core::LogLevel::Warning, "container",
                                                "not every plugin started");
    }

    pluginsModel.observe(&plugins, plugins.settings());

    std::function<void()> replayStoredSettings;

    std::function<void()> registerTheApi;

    std::function<void()> armTheModules;

    std::function<void()> wireTheCapture;

    std::function<void()> tellThePages;

    pluginsModel.wire(
        {[&fromDisk, &plugins](const QString &id) {
             return fromDisk.open(softphone::ui::pluginDirectory(), id.toStdString(), plugins);
         },
         [&uiFromDisk, &uiRegistry](const QString &id) {
             return uiFromDisk
                 .contribute(softphone::ui::pluginDirectory(), id.toStdString(), uiRegistry)
                 .why;
         },
         [&uiFromDisk, &uiRegistry, &armTheModules](const QString &id) {
             uiFromDisk.withdraw(id.toStdString(), uiRegistry);

             if (armTheModules) {
                 armTheModules();
             }
         },
         [&replayStoredSettings, &armTheModules](const QString &id) {
             softphone::core::Logger::instance().log(softphone::core::LogLevel::Info, "container",
                                                     id.toStdString() +
                                                         " is on: applying what is stored");
             if (replayStoredSettings) {
                 replayStoredSettings();
             }
             if (armTheModules) {
                 armTheModules();
             }
         }});

    const auto loadOrder = plugins.plugins().order();
    static_cast<void>(uiFromDisk.load(softphone::ui::pluginDirectory(),
                                      {loadOrder.begin(), loadOrder.end()}, uiRegistry));
    mark("the plugins");

    auto *const storageAccess =
        plugins.services().find<softphone::plugins::storage::IStorageAccess>(
            softphone::plugins::storage::kStorageAccess);
    if (storageAccess == nullptr) {
        std::cerr << "the storage plugin did not start; there is nowhere to keep anything\n";
        return EXIT_FAILURE;
    }
    auto &storage = *storageAccess;

    const auto consentWanted = std::make_shared<softphone::core::ConsentPosture>(
        softphone::core::ConsentPosture::Announce);

    const auto consentNow = [&plugins]() {
        return plugins.services().find<softphone::plugins::recording::IConsentPolicy>(
            softphone::plugins::recording::kConsent);
    };

    softphone::ui::Vocabulary vocabulary{plugins.services(),
                                         std::string{softphone::plugins::taxonomy::kTaxonomyStore}};
    if (!vocabulary.present()) {
        softphone::core::Logger::instance().log(
            softphone::core::LogLevel::Warning, "container",
            "no dictionaries: nothing will be filed until that plugin is back");
    }

    const auto announceDictionaries = [&plugins]() {
        if (auto *const announcer =
                plugins.services().find<softphone::plugins::taxonomy::IDictionaryAnnouncer>(
                    softphone::plugins::taxonomy::kAnnouncer);
            announcer != nullptr) {
            announcer->announceChange();
        }
    };

    storageModel.setRecreator([&storage,
                               &app](std::function<void(softphone::core::StoreStatus)> done) {
        storage.startAgain([done = std::move(done), &app](softphone::core::StoreStatus made) {
            QMetaObject::invokeMethod(&app, [done, made]() { done(made); }, Qt::QueuedConnection);
        });
    });

    auto book = std::make_shared<softphone::core::ContactBook>();

    softphone::core::TelemetryStore telemetry{softphone::core::stateDirectory() / "telemetry.json"};

    host.backend().setSwitchListener([&telemetry](std::string said) {
        const auto kind = softphone::core::sipServerKind(said);
        if (kind.empty()) {
            return;
        }
        telemetry.sawServer(kind);
    });

    host.engine().setServerListener([&telemetry](const std::string &, const std::string &said) {
        const auto kind = softphone::core::sipServerKind(said);
        telemetry.sawServer(kind);

        const softphone::core::LogAttribute fields[] = {{"said", said}, {"sent", kind}};
        softphone::core::Logger::instance().log(softphone::core::LogLevel::Debug, "telemetry",
                                                "a registrar said what it is", fields);
    });

    softphone::core::CallHistorian historian{storage.callLog(), host.scheduler()};
    historian.setAddressLookup([&host](const std::string &accountId) {
        softphone::core::CallHistorian::AccountIdentity line;
        if (const auto *account = softphone::core::find(host.storedConfiguration(), accountId);
            account != nullptr) {
            line.uuid = account->uuid;
            line.address = softphone::core::accountAddress(*account);
        }
        return line;
    });
    historian.setNameLookup([book](const std::string &number) { return book->nameFor(number); });

    auto *const outbound =
        plugins.services().find<softphone::plugins::integration::IOutboundEvents>(
            softphone::plugins::integration::kOutboundEvents);
    if (outbound != nullptr) {

        outbound->declare(std::string{softphone::core::webhookEvent::kStarted});
        outbound->declare(std::string{softphone::core::webhookEvent::kEnded});
        outbound->declare(std::string{softphone::core::webhookEvent::kStateChanged});
    }

    softphone::core::WebhookNotifier notifier{host.scheduler(),
                                              [outbound](softphone::core::WebhookRequest request) {
                                                  if (outbound != nullptr) {
                                                      outbound->post(std::move(request));
                                                  }
                                              }};
    notifier.setAddressLookup([&host](const std::string &accountId) {
        softphone::core::CallHistorian::AccountIdentity line;
        if (const auto *account = softphone::core::find(host.storedConfiguration(), accountId);
            account != nullptr) {
            line.uuid = account->uuid;
            line.address = softphone::core::accountAddress(*account);
        }
        return line;
    });
    notifier.setNameLookup([book](const std::string &number) { return book->nameFor(number); });

    notifier.setSecretLookup([&host]() {
        std::string token;
        static_cast<void>(
            host.keyring().fetch(std::string{softphone::core::secretKeys::kWebhookHeader}, token));
        return token;
    });

    softphone::ui::AccountsViewModel accounts{
        host.engine(), [&host](std::function<void()> work) { host.post(std::move(work)); }};

    softphone::ui::ContactsViewModel contacts;
    softphone::ui::HistoryViewModel history;

    historian.setOnRecorded([&history, &app](const softphone::core::CallRecord &written) {
        if (written.direction != softphone::core::CallDirection::Incoming ||
            softphone::core::outcome(written) != softphone::core::CallOutcome::Missed) {
            return;
        }
        QMetaObject::invokeMethod(
            &app, [&history]() { history.missedOne(); }, Qt::QueuedConnection);
    });
    softphone::ui::RecordingsViewModel recordings;

    softphone::ui::DashboardViewModel progressModel{
        std::make_unique<softphone::core::ProgressStore>(softphone::core::stateDirectory() /
                                                         "progress.json")};

    softphone::ui::TranscriptViewModel transcript;
    softphone::ui::ProvidersViewModel providers;
    softphone::ui::TaxonomyViewModel taxonomy;
    softphone::ui::ProvidersViewModel models;
    softphone::ui::ArtifactsViewModel artifacts;
    softphone::ui::TemplatesViewModel templates;

    templates.setContentLanguage(QString::fromStdString(contentLanguage));
    softphone::ui::AutomationsViewModel automations;
    softphone::ui::JobsViewModel jobsModel;
    softphone::ui::ButtonsViewModel buttons;

    softphone::ui::CallViewModel *callModel = nullptr;

    softphone::core::GenerationService *generationService = nullptr;

    std::optional<softphone::core::SettingsController> settingsController;

    const auto configurationNow =
        [&host, &settingsController]() -> const softphone::core::StoredConfiguration & {
        return settingsController ? settingsController->effective() : host.storedConfiguration();
    };

    const auto publishLines = [&accounts, &contacts, &history, &buttons, &automations, &host,
                               consentNow,
                               &callModel](const softphone::core::StoredConfiguration &running) {
        const auto dials = softphone::core::dialingAccount(running);
        accounts.applyAccounts(running.accounts, dials);
        contacts.applyAccounts(running.accounts, dials);
        history.applyAccounts(running.accounts, dials);
        buttons.applyAccounts(running.accounts, dials);

        std::vector<std::pair<QString, QString>> lines;
        lines.reserve(running.accounts.size());
        for (const auto &account : running.accounts) {
            lines.emplace_back(QString::fromStdString(account.id),
                               QString::fromStdString(account.displayName.empty()
                                                          ? account.username
                                                          : account.displayName));
        }
        automations.setAccounts(std::move(lines));

        host.post([&host, consentNow, accounts = running.accounts]() mutable {
            host.setKnownAccounts(accounts);

            if (auto *const consent = consentNow(); consent != nullptr) {
                consent->accountsChanged(std::move(accounts));
            }
        });

        if (callModel != nullptr) {
            callModel->setLines(running.accounts);
        }
    };

    publishLines(host.storedConfiguration());

    accounts.setKeyring(
        keyringStateFor(softphone::secrets::SecretStatus::NotFound, host.keyringKind()));

    if (!host.storedConfiguration().accounts.empty()) {
        host.post([&host, &app, &accounts]() {
            const auto storedPassword = host.storedPasswordStatus();
            const auto kind = host.keyringKind();
            QMetaObject::invokeMethod(
                &app,
                [&accounts, storedPassword, kind]() {
                    accounts.setKeyring(keyringStateFor(storedPassword, kind));
                },
                Qt::QueuedConnection);
        });
    }

    const auto bringTheLinesUp = [&host, &app, &accounts, &configurationNow]() {
        auto lines = softphone::core::enabledAccounts(configurationNow());
        host.post([&host, &app, &accounts, lines = std::move(lines)]() {
            for (const auto &account : lines) {
                const auto name = softphone::core::credentialName(account);
                if (name.empty()) {
                    continue;
                }

                std::string password;
                const auto said = host.keyring().fetch(name, password);

                if (said != softphone::secrets::SecretStatus::Ok) {
                    softphone::core::Logger::instance().log(
                        said == softphone::secrets::SecretStatus::NotFound
                            ? softphone::core::LogLevel::Info
                            : softphone::core::LogLevel::Warning,
                        "accounts",
                        "no password for " + account.id + ": " + std::string{toString(said)});
                }

                QMetaObject::invokeMethod(
                    &app,
                    [&accounts, id = account.id, said]() {
                        if (said == softphone::secrets::SecretStatus::Ok) {
                            accounts.connectAccount(QString::fromStdString(id));
                            return;
                        }

                        if (said != softphone::secrets::SecretStatus::NotFound) {
                            accounts.couldNotReadThePassword(id, toString(said));
                        }
                    },
                    Qt::QueuedConnection);
            }
        });
    };

    bringTheLinesUp();

    if (QNetworkInformation::loadDefaultBackend()) {
        auto *const network = QNetworkInformation::instance();
        QObject::connect(network, &QNetworkInformation::reachabilityChanged, &app,
                         [&accounts](QNetworkInformation::Reachability reachability) {
                             if (reachability != QNetworkInformation::Reachability::Online) {
                                 return;
                             }
                             static_cast<void>(accounts.reconnectWhatIsDown());
                         });
    } else {
        softphone::core::Logger::instance().log(
            softphone::core::LogLevel::Debug, "registration",
            "this system offers no way to hear about the network coming back; the lines will "
            "come back on the retry clock alone");
    }

    softphone::ui::SoundViewModel sound{
        &host.taps(), [&host](std::function<void()> work) { host.post(std::move(work)); }};

    softphone::ui::DemoCapture demoCapture;

    class CaptureTaps final : public softphone::core::ISignalTaps {
    public:
        CaptureTaps(std::function<softphone::plugins::capture::ICaptureControl *()> find,
                    softphone::ui::DemoCapture &demo)
            : find_{std::move(find)}, demo_{demo}
        {}

        void watchSignals(const std::string &name) override
        {
            wanted_ = name;
            if (demo_.running()) {
                demo_.watchSignals(name);
                return;
            }
            if (auto *const controller = find_()) {
                controller->drawings().watchSignals(name);
            }
        }

        [[nodiscard]] softphone::core::CallSignals &feed() noexcept override
        {

            if (demo_.running()) {
                demo_.watchSignals(wanted_);
                return demo_.feed();
            }

            if (auto *const controller = find_()) {

                controller->drawings().watchSignals(wanted_);
                return controller->drawings().feed();
            }
            return nothing_;
        }

    private:
        std::function<softphone::plugins::capture::ICaptureControl *()> find_;
        softphone::ui::DemoCapture &demo_;
        std::string wanted_;
        softphone::core::CallSignals nothing_{16000};
    };

    CaptureTaps captureTaps{
        [&plugins]() {
            return plugins.services().find<softphone::plugins::capture::ICaptureControl>(
                softphone::plugins::capture::kController);
        },
        demoCapture};

    softphone::ui::SoundViewModel captureSound{&captureTaps, [&app](std::function<void()> work) {

                                                   QMetaObject::invokeMethod(&app, std::move(work),
                                                                             Qt::QueuedConnection);
                                               }};

    std::optional<softphone::ui::ShellSettingsSink> settingsSink;

    softphone::ui::About about;

    about.speak(languageNow);

    softphone::http::HttplibUpdateTransport updateTransport;
    softphone::http::OpenSslSignatures updateSignatures;
    std::optional<softphone::core::UpdateService> updateService;

    if (const auto channel = softphone::core::buildChannel(); channel.configured()) {
        updateService.emplace(channel, softphone::core::version(), updateTransport,
                              updateSignatures, softphone::core::cacheDirectory() / "updates");
        mark(channel.selfReplacing() ? "updates: this build can replace itself"
                                     : "updates: this build will offer, not install");
    } else {
        mark("updates: this build has no update channel and will not look");
    }

    softphone::ui::UpdateViewModel updates{updateService ? &*updateService : nullptr};
    softphone::ui::TelemetryViewModel telemetryModel;

    softphone::http::HttplibHttpClient telemetryTransport;
    softphone::core::TelemetryService telemetryService{
        telemetry, telemetryTransport, []() { return std::chrono::system_clock::now(); }};
    telemetryService.setFacts(softphone::core::machineFacts());

    telemetryService.setLayout(std::string{softphone::core::toString(runningLayout)});

    telemetryService.setLanguage(softphone::ui::contentLanguage(languageNow));

    QObject::connect(&about, &softphone::ui::About::countryChanged, &app,
                     [&about, &telemetryService, &updateService]() {
                         const auto country = about.country().toStdString();
                         telemetryService.setCountry(country);
                         if (updateService) {
                             updateService->setCountry(country);
                         }
                     });

    softphone::ui::RestartCommand pendingRestart;
    QObject::connect(&updates, &softphone::ui::UpdateViewModel::restartWanted, &app,
                     [&pendingRestart](const QString &image) {
                         pendingRestart = softphone::ui::RestartCommand{image, {}};
                     });

    softphone::ui::PrompterViewModel prompter;

    prompter.setContentLanguage(QString::fromStdString(contentLanguage));

    auto promptable = std::make_shared<std::array<bool, 2>>(std::array<bool, 2>{false, false});

    std::function<void()> tellPrompter = [] {};

    std::function<void()> endThePrompter = [] {};

    softphone::ui::SettingsViewModel settings{
        [&host](std::function<void()> work) { host.post(std::move(work)); },
        [&settingsController]() {
            if (settingsController) {
                settingsController->withdrawPending();
            }
        },
        [&host, &settingsController](softphone::core::AudioSettings audio) {
            if (!settingsController) {
                return;
            }

            auto wanted = settingsController->desired();
            wanted.settings.audio = std::move(audio);
            static_cast<void>(settingsController->request(std::move(wanted), host.calls().calls()));
        },
        [&settingsSink]() {
            if (settingsSink) {
                settingsSink->republish();
            }
        },

        softphone::ui::SettingsViewModel::AudioProbe{
            [&host](softphone::core::ToneOutput output, std::string file,
                    softphone::core::ToneLevel loudness) {

                if (file.empty()) {
                    return;
                }
                host.startTone(file, false, output, loudness);
            },
            [&host]() { host.stopTone(); },

            [&host, &startupState](bool watching) {
                if (!startupState.ready()) {
                    return;
                }
                if (watching) {
                    host.audio().startMetering();
                } else {
                    host.audio().stopMetering();
                }
            },
            [&host, &app, &settings, &startupState]() {
                if (!startupState.ready()) {
                    return;
                }
                const auto levels = host.audio().audioLevels();

                const unsigned heard = levels ? levels->capture : 0U;
                QMetaObject::invokeMethod(
                    &app, [&settings, heard]() { settings.reportLevel(heard); },
                    Qt::QueuedConnection);
            }},
        [&host, &settingsController](std::vector<softphone::core::CodecChoice> codecs) {
            if (!settingsController) {
                return;
            }
            auto wanted = settingsController->desired();
            wanted.settings.calls.codecs = std::move(codecs);
            static_cast<void>(settingsController->request(std::move(wanted), host.calls().calls()));
        },
        [&host, &settingsController](softphone::core::AppearanceSettings appearance) {
            if (!settingsController) {
                return;
            }
            auto wanted = settingsController->desired();
            softphone::ui::putGroup(wanted, "appearance",
                                    softphone::core::asText(std::move(appearance)),
                                    softphone::core::defaultConfigLayout().application);
            static_cast<void>(settingsController->request(std::move(wanted), host.calls().calls()));
        },
        [&host, &settingsController](softphone::core::StartupSettings startup) {
            if (!settingsController) {
                return;
            }
            auto wanted = settingsController->desired();
            softphone::ui::putGroup(wanted, "startup", softphone::core::asText(startup),
                                    softphone::core::defaultConfigLayout().application);
            static_cast<void>(settingsController->request(std::move(wanted), host.calls().calls()));
        },
        [&host, &settingsController](softphone::core::HotkeySettings hotkeys) {
            if (!settingsController) {
                return;
            }
            auto wanted = settingsController->desired();
            softphone::ui::putGroup(wanted, "hotkeys", softphone::core::asText(std::move(hotkeys)),
                                    softphone::core::defaultConfigLayout().application);
            static_cast<void>(settingsController->request(std::move(wanted), host.calls().calls()));
        }};

    settings.setDiagnosticsCallback(
        [&host, &settingsController](softphone::core::DiagnosticsSettings diagnostics) {
            if (!settingsController) {
                return;
            }
            auto wanted = settingsController->desired();
            softphone::ui::putGroup(wanted, "diagnostics", softphone::core::asText(diagnostics),
                                    softphone::core::defaultConfigLayout().diagnostics);
            static_cast<void>(settingsController->request(std::move(wanted), host.calls().calls()));
        });

    settings.setWebhooksCallback(
        [&host, &settingsController](softphone::core::WebhookSettings chosen) {
            if (!settingsController) {
                return;
            }
            auto wanted = settingsController->desired();
            softphone::ui::putGroup(wanted, "webhooks", softphone::core::asText(std::move(chosen)),
                                    softphone::core::defaultConfigLayout().integration);
            static_cast<void>(settingsController->request(std::move(wanted), host.calls().calls()));
        });

    settings.setRecordingCallback(
        [&host, &settingsController](softphone::core::RecordingSettings chosen) {
            if (!settingsController) {
                return;
            }
            auto wanted = settingsController->desired();
            softphone::ui::putGroup(wanted, "recording", softphone::core::asText(std::move(chosen)),
                                    softphone::core::defaultConfigLayout().recording);
            static_cast<void>(settingsController->request(std::move(wanted), host.calls().calls()));
        });

    std::function<void()> sweepTheCallLog;

    settings.setCallsCallback([&host, &settingsController, &sweepTheCallLog](
                                  int intervalSeconds, int giveUpMinutes, int historyDays,
                                  softphone::core::CallWaiting waiting) {
        if (!settingsController) {
            return;
        }
        auto wanted = settingsController->desired();
        wanted.settings.calls.redialIntervalSeconds = intervalSeconds;
        wanted.settings.calls.redialGiveUpMinutes = giveUpMinutes;
        wanted.settings.calls.historyDays = historyDays;
        wanted.settings.calls.callWaiting = waiting;
        static_cast<void>(settingsController->request(std::move(wanted), host.calls().calls()));

        if (sweepTheCallLog) {
            sweepTheCallLog();
        }
    });

    settings.setApiCallback([&host, &settingsController](softphone::core::ApiSettings chosen) {
        if (!settingsController) {
            return;
        }
        auto wanted = settingsController->desired();
        softphone::ui::putGroup(wanted, "api", softphone::core::asText(chosen),
                                softphone::core::defaultConfigLayout().integration);
        static_cast<void>(settingsController->request(std::move(wanted), host.calls().calls()));
    });

    settings.setUpdatesCallback(
        [&host, &settingsController](softphone::core::UpdateSettings chosen) {
            if (!settingsController) {
                return;
            }
            auto wanted = settingsController->desired();
            softphone::ui::putGroup(wanted, "updates", softphone::core::asText(chosen),
                                    softphone::core::defaultConfigLayout().application);
            static_cast<void>(settingsController->request(std::move(wanted), host.calls().calls()));
        });

    about.setRegionCallback([&host, &settingsController](softphone::core::RegionSettings chosen) {
        if (!settingsController) {
            return;
        }
        auto wanted = settingsController->desired();
        softphone::ui::putGroup(wanted, "region", softphone::core::asText(chosen),
                                softphone::core::defaultConfigLayout().application);
        static_cast<void>(settingsController->request(std::move(wanted), host.calls().calls()));
    });

    {
        const auto storedRegion = softphone::core::regionFrom(
            softphone::ui::groupText(host.storedConfiguration(), "region"));

        about.applyRegion(storedRegion);

        if (!storedRegion.decided) {
            about.detectCountry();
        }
    }

    QObject::connect(&telemetryModel, &softphone::ui::TelemetryViewModel::noticeAccepted,
                     [&telemetryService]() { telemetryService.setNoticeAnswered(true); });

    telemetryModel.bind(
        &telemetry,
        [&telemetry, &telemetryService](softphone::core::TelemetryMode chosen) {
            telemetryService.setMode(chosen);

            if (chosen != softphone::core::TelemetryMode::Extended) {
                telemetry.forgetCounters();
            }
        },
        [&host, &settingsController](softphone::core::TelemetryMode chosen) {
            if (!settingsController) {
                return;
            }
            auto wanted = settingsController->desired();
            softphone::ui::putGroup(
                wanted, "telemetry",
                softphone::core::asText(softphone::core::TelemetrySettings{.mode = chosen}),
                softphone::core::defaultConfigLayout().application);
            static_cast<void>(settingsController->request(std::move(wanted), host.calls().calls()));
        });

    telemetryModel.setOverviewListener(
        [&telemetryService](int done) { telemetryService.setOverviewDone(done); });

    telemetryModel.setReportSender([&telemetryService, &telemetryModel, &app]() {
        std::thread{[&telemetryService, &telemetryModel, &app]() {
            const bool accepted = telemetryService.send({});

            const auto answered = QString::fromStdString(telemetryService.lastHost());
            QMetaObject::invokeMethod(
                &app,
                [&telemetryModel, accepted, answered]() {
                    telemetryModel.reportFinished(accepted, answered);
                },
                Qt::QueuedConnection);
        }}.detach();
    });

    telemetryModel.setPaths(softphone::core::logFileIn(softphone::core::stateDirectory()),
                            softphone::core::stateDirectory());
    telemetryModel.setFeedbackSender([&telemetryService, &telemetryModel,
                                      &app](softphone::core::FeedbackReport report, bool attach) {
        std::thread{[&telemetryService, &telemetryModel, &app, report = std::move(report),
                     attach]() mutable {

            auto attached = attach
                                ? softphone::core::attachmentsFor(
                                      softphone::core::logFileIn(softphone::core::stateDirectory()),
                                      softphone::core::stateDirectory())
                                : std::vector<softphone::core::Attachment>{};
            const bool arrived =
                telemetryService.sendFeedback(std::move(report), std::move(attached), {});
            QMetaObject::invokeMethod(
                &app, [&telemetryModel, arrived]() { telemetryModel.feedbackFinished(arrived); },
                Qt::QueuedConnection);
        }}.detach();
    });

    telemetryService.setNoticeAnswered(!telemetryModel.noticeWanted());

    settings.setApiTokenCallback([&host, &settings](const QString &token) {
        const auto key = std::string{softphone::core::secretKeys::kApiToken};
        const auto status = token.isEmpty() ? host.keyring().erase(key)
                                            : host.keyring().store(key, token.toStdString());
        if (status != softphone::secrets::SecretStatus::Ok &&
            status != softphone::secrets::SecretStatus::NotFound) {
            softphone::core::Logger::instance().log(softphone::core::LogLevel::Warning, "ui",
                                                    "the keyring would not keep the API token");
        }

        std::string stored;
        settings.setApiTokenStored(host.keyring().fetch(key, stored) ==
                                   softphone::secrets::SecretStatus::Ok);
    });

    {
        std::string stored;
        settings.setApiTokenStored(
            host.keyring().fetch(std::string{softphone::core::secretKeys::kApiToken}, stored) ==
            softphone::secrets::SecretStatus::Ok);
    }

    settings.setWebhookTokenCallback([&host, &settings](const QString &token) {
        const auto key = std::string{softphone::core::secretKeys::kWebhookHeader};
        const auto status = token.isEmpty() ? host.keyring().erase(key)
                                            : host.keyring().store(key, token.toStdString());
        if (status != softphone::secrets::SecretStatus::Ok &&
            status != softphone::secrets::SecretStatus::NotFound) {
            softphone::core::Logger::instance().log(softphone::core::LogLevel::Warning, "ui",
                                                    "the keyring would not keep the webhook token");
        }

        std::string stored;
        settings.setWebhookTokenStored(host.keyring().fetch(key, stored) ==
                                       softphone::secrets::SecretStatus::Ok);
    });

    settings.setWebhookTestCallback([&host, &notifier]() {

        host.post([&notifier]() { notifier.sendTestEvent(); });
    });

    {
        std::string stored;
        settings.setWebhookTokenStored(
            host.keyring().fetch(std::string{softphone::core::secretKeys::kWebhookHeader},
                                 stored) == softphone::secrets::SecretStatus::Ok);
    }

    const auto asText = [](const std::filesystem::path &path) {
        return QString::fromStdString(softphone::core::asUtf8(path));
    };

    settings.setLanguages(softphone::ui::availableLanguages(uiRegistry));
    settings.setPaths(asText(softphone::core::defaultLogFile()),
                      asText(host.configuration().filePath()), asText(storage.file()));

    settings.setRunningLayout(runningLayout);

    QObject::connect(&settings, &softphone::ui::SettingsViewModel::layoutRestartWanted, &app,
                     [&pendingRestart]() { pendingRestart = softphone::ui::howToStartAgain(); });

    softphone::ui::DesktopColours desktopColours;
    settings.followTheDesktop([&desktopColours]() { return desktopColours.scheme(); });
    QObject::connect(&desktopColours, &softphone::ui::DesktopColours::changed, &settings,
                     &softphone::ui::SettingsViewModel::theDesktopChangedItsMind);

#if defined(Q_OS_MACOS)

    const auto frameFollowsTheTheme = [&settings]() {
        using Theme = softphone::ui::SettingsViewModel::Theme;
        switch (settings.theme()) {
        case Theme::Light:
            QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Light);
            return;
        case Theme::Dark:
            QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Dark);
            return;
        case Theme::Default:
            break;
        }
        QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Unknown);
    };
    QObject::connect(&settings, &softphone::ui::SettingsViewModel::appearanceChanged, &app,
                     frameFollowsTheTheme);
    frameFollowsTheTheme();
#endif

    QObject::connect(&settings, &softphone::ui::SettingsViewModel::logOpenRequested, &app,
                     [](const QString &path) {

                         QDesktopServices::openUrl(QUrl::fromLocalFile(path));
                     });

    settings.setFirstRun(host.storedConfiguration().accounts.empty());

    host.setToneEndedCallback([&settings, &app]() {
        QMetaObject::invokeMethod(
            &app, [&settings]() { settings.reportToneEnded(); }, Qt::QueuedConnection);
    });

    const auto autostart = softphone::ui::makeAutostart();

    const auto menuEntry = softphone::ui::makeMenuEntry();

    settingsSink.emplace(
        host.audio(), host.codecs(), host.engine(), host.calls(), host.redial(),
        [&settings, &app](softphone::ui::AudioReport report) {

            QMetaObject::invokeMethod(
                &app,
                [&settings, report = std::move(report)]() mutable {
                    settings.applyAudio(std::move(report.devices), std::move(report.chosen),
                                        std::move(report.result));
                },
                Qt::QueuedConnection);
        },
        [&settings, &app](softphone::ui::CodecReport report) {
            QMetaObject::invokeMethod(
                &app,
                [&settings, report = std::move(report)]() mutable {
                    settings.applyCodecs(std::move(report.available), std::move(report.chosen));

                    settings.applyCallSettings(report.calls.redialIntervalSeconds,
                                               report.calls.redialGiveUpMinutes,
                                               report.calls.historyDays, report.calls.callWaiting);
                },
                Qt::QueuedConnection);
        },
        [&settings, &app, &engine, &contacts, &sound, &callModel, &generationService, &taxonomy,
         &templates, &automations, &storage, &about, &languageNow, &tellThePages, &pluginsModel,
         &uiRegistry, &telemetryModel, &telemetryService,
         &prompter](softphone::core::AppearanceSettings wanted) {
            QMetaObject::invokeMethod(
                &app,
                [&settings, &app, &engine, &contacts, &sound, &callModel, &generationService,
                 &taxonomy, &templates, &automations, &storage, &about, &languageNow, &tellThePages,
                 &pluginsModel, &uiRegistry, &telemetryModel, &telemetryService, &prompter,
                 wanted = std::move(wanted)]() {
                    settings.applyAppearance(wanted);
                    sound.chooseDrawing(wanted.visualisation);

                    if (callModel != nullptr) {
                        callModel->setEndedLinger(std::chrono::seconds{wanted.endedCallSeconds});
                    }

                    const auto language = softphone::ui::installTranslations(
                        app, uiRegistry, QString::fromStdString(wanted.language));
                    if (engine) {
                        engine->setUiLanguage(language);
                        engine->retranslate();
                    }

                    uiRegistry.retitle();

                    storage.setContentLanguage(softphone::ui::contentLanguage(language));

                    templates.setContentLanguage(
                        QString::fromStdString(softphone::ui::contentLanguage(language)));

                    prompter.setContentLanguage(
                        QString::fromStdString(softphone::ui::contentLanguage(language)));

                    if (generationService != nullptr) {
                        generationService->setInterfaceLanguage(language.toStdString());
                    }

                    taxonomy.retranslated();
                    templates.retranslated();
                    automations.retranslated();

                    pluginsModel.retranslated();

                    languageNow = language;
                    if (tellThePages) {
                        tellThePages();
                    }

                    about.speak(language);

                    telemetryModel.retranslate();

                    telemetryService.setLanguage(softphone::ui::contentLanguage(language));

                    contacts.setLocale(QLocale{language});
                },
                Qt::QueuedConnection);
        },
        [&host](const softphone::core::AccountConfig &account) {

            if (host.forgetPassword(account) != softphone::secrets::SecretStatus::Ok) {
                softphone::core::Logger::instance().log(
                    softphone::core::LogLevel::Warning, "ui",
                    "the keyring kept the password of the account " + account.id +
                        " that was just removed");
            }
        },
        [&settings, &app](softphone::core::StartupSettings startup) {

            QMetaObject::invokeMethod(
                &app, [&settings, startup]() { settings.applyStartup(startup); },
                Qt::QueuedConnection);
        },
        autostart.get(),
        [&settings, &app](softphone::core::HotkeySettings hotkeys) {
            QMetaObject::invokeMethod(
                &app,
                [&settings, hotkeys = std::move(hotkeys)]() { settings.applyHotkeys(hotkeys); },
                Qt::QueuedConnection);
        });

    settingsSink->setButtonsReporter([&host, &buttons, &app,
                                      &settingsController](softphone::core::ButtonSettings chosen) {
        const auto dials = settingsController
                               ? softphone::core::dialingAccount(settingsController->effective())
                               : softphone::core::dialingAccount(host.storedConfiguration());

        host.setButtons(chosen.buttons, dials);

        QMetaObject::invokeMethod(
            &app, [&buttons, chosen]() { buttons.applyButtons(chosen.buttons); },
            Qt::QueuedConnection);
    });

    host.setLampListener(
        [&buttons, &app](const std::string &buttonId, softphone::core::LampState lamp) {
            QMetaObject::invokeMethod(
                &app,
                [&buttons, buttonId, lamp]() {
                    buttons.applyState(QString::fromStdString(buttonId), lamp);
                },
                Qt::QueuedConnection);
        });

    QObject::connect(
        &buttons, &softphone::ui::ButtonsViewModel::buttonsEdited, &app,
        [&host, &settingsController](const QVariantList &edited) {
            if (!settingsController) {
                return;
            }

            softphone::core::ButtonSettings wantedButtons;
            for (const auto &entry : edited) {
                const auto row = entry.toMap();
                softphone::core::PhoneButton button;
                button.id = row.value(QStringLiteral("id")).toString().toStdString();
                button.title = row.value(QStringLiteral("title")).toString().toStdString();
                button.number = row.value(QStringLiteral("number")).toString().toStdString();
                button.accountId = row.value(QStringLiteral("accountId")).toString().toStdString();
                button.watched = row.value(QStringLiteral("watched")).toBool();
                wantedButtons.buttons.push_back(std::move(button));
            }

            host.post([&host, &settingsController, wantedButtons]() {
                auto wanted = settingsController->desired();
                softphone::ui::putGroup(wanted, "buttons", softphone::core::asText(wantedButtons),
                                        softphone::core::defaultConfigLayout().buttons);
                static_cast<void>(
                    settingsController->request(std::move(wanted), host.calls().calls()));
            });
        });

    settingsSink->setApiReporter([&outward, &settings, &app](softphone::core::ApiSettings applied) {

        outward.applyApi(applied);

        QMetaObject::invokeMethod(
            &app, [&settings, applied]() { settings.applyApi(applied); }, Qt::QueuedConnection);
    });

    settingsSink->setUpdateReporter(
        [&settings, &updates, &app](softphone::core::UpdateSettings applied) {

            QMetaObject::invokeMethod(
                &app,
                [&settings, &updates, applied]() {
                    settings.applyUpdates(applied);
                    updates.automatic(applied.automatic);
                },
                Qt::QueuedConnection);
        });

    settingsSink->setRecordingReporter([&settings, &app, &host, consentNow,
                                        consentWanted](softphone::core::RecordingSettings applied) {

        *consentWanted = applied.consent;

        const auto notice = softphone::ui::noticeFile(applied.announcement);
        const auto beep = softphone::ui::recordingToneFile();
        const auto shipped = softphone::ui::shippedNoticeFile();
        host.post([&host, consentNow, applied, notice, beep, shipped]() {

            host.applyRetention(applied.retention);
            if (auto *const consent = consentNow(); consent != nullptr) {
                consent->apply(applied, notice, beep, shipped);
            }
        });

        QMetaObject::invokeMethod(
            &app, [&settings, applied]() { settings.applyRecording(applied); },
            Qt::QueuedConnection);
    });

    settingsSink->setWebhooksReporter(
        [&settings, &app, &host, &notifier, &outward](softphone::core::WebhookSettings applied) {

            host.post([&notifier, applied]() { notifier.setSettings(applied); });

            outward.applyWebhooks(applied);

            QMetaObject::invokeMethod(
                &app, [&settings, applied]() { settings.applyWebhooks(applied); },
                Qt::QueuedConnection);
        });

    settingsSink->setMenuEntry(menuEntry.get());

    settings.thisRunsFromAnImage(menuEntry != nullptr);

    const auto linkHandler = softphone::ui::makeLinkHandler();
    settings.answersLinksThrough(linkHandler.get());

    settingsSink->setDiagnosticsReporter(
        [&settings, &app, &host](softphone::core::DiagnosticsSettings diagnostics) {

            host.setStackLogLevel(diagnostics.level);

            QMetaObject::invokeMethod(
                &app, [&settings, diagnostics]() { settings.applyDiagnostics(diagnostics); },
                Qt::QueuedConnection);
        });

    settingsController.emplace(
        *settingsSink, host.storedConfiguration(),
        [&host](const softphone::core::StoredConfiguration &wanted) {

            static_cast<void>(host.configuration().save(wanted));
        },
        [&settings, &accounts, &app, &settingsController, &publishLines]() {
            QMetaObject::invokeMethod(
                &app,
                [&settings, &accounts, &publishLines, waiting = settingsController->pending(),
                 running = settingsController->effective()]() mutable {
                    settings.apply(waiting);

                    publishLines(running);
                    accounts.applyPending(waiting);
                },
                Qt::QueuedConnection);
        });

    const auto publishAccounts = [&accounts, &app, &settingsController, &publishLines]() {
        QMetaObject::invokeMethod(
            &app,
            [&accounts, &publishLines, running = settingsController->effective(),
             waiting = settingsController->pending()]() mutable {
                publishLines(running);
                accounts.applyPending(waiting);
            },
            Qt::QueuedConnection);
    };

    const auto makeStoredTrue = [&settingsSink, &host, &configurationNow, &publishAccounts,
                                 &registerTheApi, &telemetryModel, &about]() {

        const auto &whole = configurationNow();
        const auto &stored = whole.settings;

        const auto text = [&whole](const char *name) {
            return softphone::ui::groupText(whole, name);
        };

        settingsSink->applyAppearance(softphone::core::appearanceFrom(text("appearance")));
        settingsSink->applyHotkeys(softphone::core::hotkeysFrom(text("hotkeys")));
        settingsSink->applyDiagnostics(softphone::core::diagnosticsFrom(text("diagnostics")));
        settingsSink->applyWebhooks(softphone::core::webhooksFrom(text("webhooks")));
        settingsSink->applyApi(softphone::core::apiFrom(text("api")));
        settingsSink->applyUpdates(softphone::core::updatesFrom(text("updates")));
        about.applyRegion(softphone::core::regionFrom(text("region")));

        telemetryModel.applyMode(softphone::core::telemetryFrom(text("telemetry")).mode);

        settingsSink->applyRecording(softphone::core::recordingFrom(text("recording")));

        if (host.running()) {

            host.post([&settingsSink, audio = stored.audio, calls = stored.calls,
                       buttons = softphone::core::buttonsFrom(text("buttons"))]() {

                settingsSink->reapplyAudio(audio);
                settingsSink->applyCallSettings(calls);
                settingsSink->applyButtons(buttons);
            });
        }

        if (registerTheApi) {
            registerTheApi();
        }

        publishAccounts();
    };

    replayStoredSettings = makeStoredTrue;

    telephonePower = [&host, &app, &startupState, &restarter, &makeStoredTrue, &bringTheLinesUp,
                      &settingsSink](bool on) {
        if (!on) {

            host.stop();
            startupState.nothingToWaitFor();
            softphone::core::Logger::instance().log(softphone::core::LogLevel::Info, "ui",
                                                    "the telephone is off: the stack is stopped");
            return;
        }

        if (host.running()) {
            return;
        }

        startupState.starting();
        restarter = softphone::core::JoiningThread{
            [&host, &app, &startupState, &makeStoredTrue, &bringTheLinesUp, &settingsSink]() {
                auto failure = host.start();
                QMetaObject::invokeMethod(
                    &app,
                    [&startupState, &makeStoredTrue, &bringTheLinesUp, &settingsSink,
                     failure = std::move(failure)]() {
                        if (failure.has_value()) {
                            softphone::core::Logger::instance().log(
                                softphone::core::LogLevel::Error, "ui",
                                "the sip stack would not start again: " + *failure);
                            startupState.failed(QString::fromStdString(*failure));
                            return;
                        }
                        startupState.succeeded();

                        settingsSink->forgetTheAudio();
                        makeStoredTrue();

                        bringTheLinesUp();
                    },
                    Qt::QueuedConnection);
            }};
    };

    host.post([&settingsSink, &host, &settingsController, &telemetryModel, &about,
               autostart = autostart.get()]() {
        settingsSink->applyAudio(host.storedConfiguration().settings.audio);
        settingsSink->applyCallSettings(host.storedConfiguration().settings.calls);
        settingsSink->applyButtons(softphone::core::buttonsFrom(
            softphone::ui::groupText(host.storedConfiguration(), "buttons")));
        settingsSink->applyAppearance(softphone::core::appearanceFrom(
            softphone::ui::groupText(host.storedConfiguration(), "appearance")));
        settingsSink->applyHotkeys(softphone::core::hotkeysFrom(
            softphone::ui::groupText(host.storedConfiguration(), "hotkeys")));
        settingsSink->applyDiagnostics(softphone::core::diagnosticsFrom(
            softphone::ui::groupText(host.storedConfiguration(), "diagnostics")));
        settingsSink->applyWebhooks(softphone::core::webhooksFrom(
            softphone::ui::groupText(host.storedConfiguration(), "webhooks")));
        settingsSink->applyRecording(softphone::core::recordingFrom(
            softphone::ui::groupText(host.storedConfiguration(), "recording")));
        settingsSink->applyApi(
            softphone::core::apiFrom(softphone::ui::groupText(host.storedConfiguration(), "api")));
        settingsSink->applyUpdates(softphone::core::updatesFrom(
            softphone::ui::groupText(host.storedConfiguration(), "updates")));
        about.applyRegion(softphone::core::regionFrom(
            softphone::ui::groupText(host.storedConfiguration(), "region")));

        telemetryModel.applyMode(
            softphone::core::telemetryFrom(
                softphone::ui::groupText(host.storedConfiguration(), "telemetry"))
                .mode);

        const auto session = autostart->state();
        const auto stored = softphone::core::startupFrom(
            softphone::ui::groupText(host.storedConfiguration(), "startup"));

        if (stored.autostart && session.present && !session.enabled) {
            softphone::core::Logger::instance().log(
                softphone::core::LogLevel::Info, "ui",
                "the session's own settings have switched autostart off; following that");

            auto wanted = settingsController->desired();
            auto startup =
                softphone::core::startupFrom(softphone::ui::groupText(wanted, "startup"));
            startup.autostart = false;
            softphone::ui::putGroup(wanted, "startup", softphone::core::asText(startup),
                                    softphone::core::defaultConfigLayout().application);
            static_cast<void>(settingsController->request(std::move(wanted), host.calls().calls()));
            return;
        }

        settingsSink->applyStartup(stored);
    });

    QObject::connect(
        &accounts, &softphone::ui::AccountsViewModel::accountSubmitted, &app,
        [&host, &accounts, &settings, &settingsController, &settingsSink, &app,
         publishAccounts](const softphone::core::AccountConfig &edited, const QString &password) {
            host.post([&host, &accounts, &settings, &settingsController, &settingsSink, &app,
                       publishAccounts, account = edited,
                       secret = password.toStdString()]() mutable {
                if (!settingsController) {
                    return;
                }

                auto wanted = settingsController->desired();

                const bool minted = account.id.empty();
                if (minted) {
                    account.id = softphone::core::mintAccountId(wanted);
                }

                if (account.uuid.empty()) {
                    account.uuid = softphone::core::makeUuid();
                }

                if (const auto *was = softphone::core::find(wanted, account.id)) {
                    if (const auto move = softphone::plugins::softphone::refilingFor(
                            softphone::core::credentialName(*was), account)) {
                        static_cast<void>(host.refilePassword(*move));
                    }
                }

                auto keyring = softphone::secrets::SecretStatus::Ok;
                if (!secret.empty()) {
                    keyring = host.rememberPassword(account, secret);
                }

                softphone::core::upsert(wanted, account);

                if (softphone::core::dialingAccount(wanted).empty() && account.enabled) {
                    wanted.settings.calls.dialAccount = account.id;
                }

                const auto changes =
                    settingsController->request(std::move(wanted), host.calls().calls());

                const bool applied =
                    std::any_of(changes.begin(), changes.end(), [&account](const auto &change) {
                        return change.area == softphone::core::SettingArea::Account &&
                               change.subject == account.id;
                    });

                if (!secret.empty() && !applied && settingsSink) {
                    settingsSink->applyAccount(account);
                }

                publishAccounts();

                const auto kind = host.keyringKind();

                QMetaObject::invokeMethod(
                    &app,
                    [&accounts, &settings, account, keyring, kind, minted]() {
                        if (minted) {

                            accounts.select(QString::fromStdString(account.id));
                        }

                        accounts.setKeyring(keyringStateFor(keyring, kind));

                        settings.setFirstRun(false);
                    },
                    Qt::QueuedConnection);
            });
        });

    QObject::connect(&accounts, &softphone::ui::AccountsViewModel::accountChanged, &app,
                     [&host, &settingsController,
                      publishAccounts](const softphone::core::AccountConfig &account) {
                         host.post([&host, &settingsController, publishAccounts, account]() {
                             if (!settingsController) {
                                 return;
                             }
                             auto wanted = settingsController->desired();
                             softphone::core::upsert(wanted, account);
                             static_cast<void>(settingsController->request(std::move(wanted),
                                                                           host.calls().calls()));
                             publishAccounts();
                         });
                     });

    QObject::connect(&accounts, &softphone::ui::AccountsViewModel::accountDropped, &app,
                     [&host, &settingsController, publishAccounts](const QString &accountId) {
                         host.post([&host, &settingsController, publishAccounts,
                                    id = accountId.toStdString()]() {
                             if (!settingsController) {
                                 return;
                             }
                             auto wanted = settingsController->desired();
                             softphone::core::remove(wanted, id);
                             static_cast<void>(settingsController->request(std::move(wanted),
                                                                           host.calls().calls()));
                             publishAccounts();
                         });
                     });

    QObject::connect(
        &accounts, &softphone::ui::AccountsViewModel::accountOrderChanged, &app,
        [&host, &settingsController, publishAccounts](const QStringList &accountIds) {
            std::vector<std::string> order;
            order.reserve(static_cast<std::size_t>(accountIds.size()));
            for (const auto &id : accountIds) {
                order.push_back(id.toStdString());
            }

            host.post([&host, &settingsController, publishAccounts, order = std::move(order)]() {
                if (!settingsController) {
                    return;
                }

                auto wanted = settingsController->desired();

                std::vector<softphone::core::AccountConfig> reordered;
                reordered.reserve(wanted.accounts.size());
                for (const auto &id : order) {
                    if (const auto *account = softphone::core::find(wanted, id)) {
                        reordered.push_back(*account);
                    }
                }
                for (const auto &account : wanted.accounts) {
                    if (std::ranges::find(order, account.id) == order.end()) {
                        reordered.push_back(account);
                    }
                }
                wanted.accounts = std::move(reordered);

                static_cast<void>(
                    settingsController->request(std::move(wanted), host.calls().calls()));
                publishAccounts();
            });
        });

    QObject::connect(&accounts, &softphone::ui::AccountsViewModel::dialAccountChosen, &app,
                     [&host, &settingsController, publishAccounts](const QString &accountId) {
                         host.post([&host, &settingsController, publishAccounts,
                                    id = accountId.toStdString()]() {
                             if (!settingsController) {
                                 return;
                             }
                             auto wanted = settingsController->desired();
                             wanted.settings.calls.dialAccount = id;
                             static_cast<void>(settingsController->request(std::move(wanted),
                                                                           host.calls().calls()));
                             publishAccounts();
                         });
                     });

    auto globalHotkeys =
        softphone::ui::makeGlobalHotkeys([&settings](softphone::ui::HotkeyAction action) {

            softphone::core::Logger::instance().log(
                softphone::core::LogLevel::Debug, "ui",
                "hotkey pressed: " + std::string{softphone::core::toString(action)});

            settings.announceHotkey(static_cast<softphone::ui::SettingsViewModel::Action>(action));
        });

    const auto grabHotkeys = [&settings, &globalHotkeys,
                              grabbing = std::make_shared<bool>(false)]() {
        if (*grabbing) {
            return;
        }
        *grabbing = true;

        if (settings.capturingKeys()) {
            static_cast<void>(globalHotkeys->grab({}));
            *grabbing = false;
            return;
        }

        const auto outcomes = globalHotkeys->grab(settings.bindings());
        settings.applyHotkeyGrabs(outcomes, globalHotkeys->supported());

        *grabbing = false;
    };

    QObject::connect(&settings, &softphone::ui::SettingsViewModel::hotkeysChanged, &app,
                     grabHotkeys);

    grabHotkeys();

    softphone::ui::CallViewModel calls{
        host.calls(), host.redial(),
        [&host](std::function<void()> work) { host.post(std::move(work)); }};

    QObject::connect(&calls, &softphone::ui::CallViewModel::callChanged, &updates,
                     [&calls, &updates]() { updates.inACall(calls.hasCall()); });

    calls.setEndedLinger(
        std::chrono::seconds{softphone::core::appearanceFrom(
                                 softphone::ui::groupText(host.storedConfiguration(), "appearance"))
                                 .endedCallSeconds});

    const auto followCall = [&calls, &sound]() {
        sound.follow(calls.talking() ? calls.currentCallId() : QString{});
    };
    QObject::connect(&calls, &softphone::ui::CallViewModel::callChanged, &app, followCall);

    const std::function<void()> refreshContacts = [&storage, &host, &calls, &contacts, &history,
                                                   &recordings, &app, book]() {
        storage.post([&host, &calls, &contacts, &history, &recordings, &app,
                      book](softphone::core::IContactStore &store, softphone::core::ICallLog &) {
            std::vector<softphone::core::Contact> found;
            const auto status = store.contacts(found);

            QMetaObject::invokeMethod(
                &app,
                [&contacts, status]() {
                    contacts.setAvailable(status == softphone::core::StoreStatus::Ok);
                },
                Qt::QueuedConnection);

            if (status != softphone::core::StoreStatus::Ok) {
                return;
            }

            host.post([book, found]() mutable { book->reset(std::move(found)); });
            QMetaObject::invokeMethod(
                &app,
                [&calls, &contacts, &history, &recordings, found]() mutable {
                    calls.setContacts(found);
                    history.setContacts(found);

                    recordings.setContacts(found);
                    contacts.applyContacts(std::move(found));
                },
                Qt::QueuedConnection);
        });
    };
    refreshContacts();

    QObject::connect(&storageModel, &softphone::ui::StorageViewModel::startedAgain, &app,
                     refreshContacts);

    const std::function<void()> refreshHistory = [&storage, &history, &app]() {
        const auto query = history.query();
        const auto forThePhone = history.newestQuery();
        storage.post([&history, &app, query, forThePhone](softphone::core::IContactStore &,
                                                          softphone::core::ICallLog &log) {
            std::vector<softphone::core::CallRecord> found;
            const auto status = log.calls(query, found);

            std::vector<softphone::core::CallRecord> newest;
            static_cast<void>(log.calls(forThePhone, newest));

            QMetaObject::invokeMethod(
                &app,
                [&history, status, found, newest]() mutable {
                    history.setAvailable(status == softphone::core::StoreStatus::Ok);
                    if (status == softphone::core::StoreStatus::Ok) {
                        history.applyCalls(std::move(found));
                        history.applyNewest(std::move(newest));
                    }
                },
                Qt::QueuedConnection);
        });
    };
    refreshHistory();

    QObject::connect(&history, &softphone::ui::HistoryViewModel::queryChanged, &app,
                     refreshHistory);

    QObject::connect(&history, &softphone::ui::HistoryViewModel::pageRequested, &app,
                     [&storage, &history, &app]() {
                         const auto query = history.nextPageQuery();
                         storage.post([&history, &app, query](softphone::core::IContactStore &,
                                                              softphone::core::ICallLog &log) {
                             std::vector<softphone::core::CallRecord> found;
                             const auto status = log.calls(query, found);

                             QMetaObject::invokeMethod(
                                 &app,
                                 [&history, status, found]() mutable {
                                     history.setAvailable(status ==
                                                          softphone::core::StoreStatus::Ok);
                                     if (status == softphone::core::StoreStatus::Ok) {
                                         history.appendCalls(std::move(found));
                                     }
                                 },
                                 Qt::QueuedConnection);
                         });
                     });

    QObject::connect(&history, &softphone::ui::HistoryViewModel::callDropped, &app,
                     [&storage, refreshHistory](const QString &id) {
                         storage.post([wanted = id.toStdString(),
                                       refreshHistory](softphone::core::IContactStore &,
                                                       softphone::core::ICallLog &log) {
                             static_cast<void>(log.removeCall(wanted));
                             refreshHistory();
                         });
                     });

    QObject::connect(&history, &softphone::ui::HistoryViewModel::clearRequested, &app,
                     [&storage, refreshHistory]() {
                         storage.post([refreshHistory](softphone::core::IContactStore &,
                                                       softphone::core::ICallLog &log) {
                             static_cast<void>(log.clearCalls());
                             refreshHistory();
                         });
                     });

    sweepTheCallLog = [&storage, &configurationNow, refreshHistory]() {

        const auto days = configurationNow().settings.calls.historyDays;
        if (days <= 0) {
            return;
        }

        const auto before = std::chrono::system_clock::now() - std::chrono::hours{24} * days;
        storage.post([before, days, refreshHistory](softphone::core::IContactStore &,
                                                    softphone::core::ICallLog &log) {
            auto removed = 0;
            if (log.removeCallsBefore(before, removed) == softphone::core::StoreStatus::Ok &&
                removed > 0) {
                softphone::core::Logger::instance().log(
                    softphone::core::LogLevel::Info, "history",
                    "the call log kept " + std::to_string(days) +
                        " days: " + std::to_string(removed) + " older calls removed");
                refreshHistory();
            }
        });
    };

    sweepTheCallLog();

    const std::function<void()> refreshRecordings = [&storage, &recordings, &app]() {

        auto asked = recordings.query();

        asked.limit = softphone::ui::RecordingsViewModel::pageSize();

        storage.withRecordings(
            [&recordings, &app, query = std::move(asked)](softphone::core::IRecordingStore &store) {
                std::vector<softphone::core::Recording> found;
                const auto status = store.recordings(query, found);

                QMetaObject::invokeMethod(
                    &app,
                    [&recordings, status, found]() mutable {
                        recordings.setRecordings(std::move(found),
                                                 status == softphone::core::StoreStatus::Ok);
                    },
                    Qt::QueuedConnection);
            });
    };
    refreshRecordings();

    const auto noteDeeds = [&app, &progressModel](const QStringList &found) {
        if (found.isEmpty()) {
            return;
        }
        QMetaObject::invokeMethod(
            &app, [&progressModel, found]() { progressModel.noteAll(found); },
            Qt::QueuedConnection);
    };

    const auto askWhatHasBeenDone = [&storage, &progressModel, noteDeeds]() {
        const auto missing = [&progressModel](const char *code) {
            return !progressModel.wasDone(QString::fromUtf8(code));
        };

        if (missing("calls.recorded") || missing("capture.first") ||
            missing("transcription.first") || missing("processing.first")) {
            storage.withRecordings([noteDeeds, missing](softphone::core::IRecordingStore &store) {
                const auto any = [&store](softphone::core::RecordingQuery asking) {
                    asking.limit = 1;
                    std::vector<softphone::core::Recording> rows;
                    return store.recordings(asking, rows) == softphone::core::StoreStatus::Ok &&
                           !rows.empty();
                };

                QStringList found;
                softphone::core::RecordingQuery asking;

                asking.source = softphone::core::RecordingSource::Native;
                if (missing("calls.recorded") && any(asking)) {
                    found << QStringLiteral("calls.recorded");
                }

                asking = {};
                asking.source = softphone::core::RecordingSource::External;
                if (missing("capture.first") && any(asking)) {
                    found << QStringLiteral("capture.first");
                }

                asking = {};
                asking.transcribed = true;
                if (missing("transcription.first") && any(asking)) {
                    found << QStringLiteral("transcription.first");
                }

                asking = {};
                asking.processed = true;
                if (missing("processing.first") && any(asking)) {
                    found << QStringLiteral("processing.first");
                }

                noteDeeds(found);
            });
        }

        if (missing("calls.outgoing") || missing("calls.incoming")) {
            storage.post([noteDeeds, missing](softphone::core::IContactStore &,
                                              softphone::core::ICallLog &log) {
                QStringList found;

                if (missing("calls.outgoing")) {
                    softphone::core::CallLogQuery asking;
                    asking.limit = 1;
                    asking.direction = softphone::core::CallDirection::Outgoing;
                    std::vector<softphone::core::CallRecord> rows;
                    if (log.calls(asking, rows) == softphone::core::StoreStatus::Ok &&
                        !rows.empty()) {
                        found << QStringLiteral("calls.outgoing");
                    }
                }

                if (missing("calls.incoming")) {
                    softphone::core::CallLogQuery asking;
                    asking.limit = 200;
                    asking.direction = softphone::core::CallDirection::Incoming;
                    std::vector<softphone::core::CallRecord> rows;
                    if (log.calls(asking, rows) == softphone::core::StoreStatus::Ok) {
                        for (const auto &row : rows) {
                            if (row.answered) {
                                found << QStringLiteral("calls.incoming");
                                break;
                            }
                        }
                    }
                }

                noteDeeds(found);
            });
        }
    };

    askWhatHasBeenDone();

    QObject::connect(&recordings, &softphone::ui::RecordingsViewModel::rowsChanged, &app,
                     askWhatHasBeenDone);
    QObject::connect(&history, &softphone::ui::HistoryViewModel::callsChanged, &app,
                     askWhatHasBeenDone);

    QObject::connect(&recordings, &softphone::ui::RecordingsViewModel::filterChanged, &app,
                     [&recordings, &progressModel]() {
                         if (!recordings.search().isEmpty()) {
                             progressModel.note(QStringLiteral("library.search"));
                         }
                     });

    QObject::connect(&telemetryModel, &softphone::ui::TelemetryViewModel::feedbackChanged, &app,
                     [&telemetryModel, &progressModel]() {
                         if (telemetryModel.delivered()) {
                             progressModel.note(QStringLiteral("feedback.sent"));
                         }
                     });

    QObject::connect(&prompter, &softphone::ui::PrompterViewModel::runningChanged, &app,
                     [&prompter, &progressModel]() {
                         if (prompter.running()) {
                             progressModel.note(QStringLiteral("prompter.first"));
                         }
                     });

    const std::function<void(const std::string &)> refreshOneRecording =
        [&storage, &recordings, &app](const std::string &id) {
            if (id.empty()) {
                return;
            }
            storage.withRecordings([&recordings, &app,
                                    wanted = id](softphone::core::IRecordingStore &store) {
                softphone::core::Recording row;
                if (store.recording(wanted, row) != softphone::core::StoreStatus::Ok) {
                    return;
                }
                QMetaObject::invokeMethod(
                    &app,
                    [&recordings, row]() mutable { recordings.updateRecording(std::move(row)); },
                    Qt::QueuedConnection);
            });
        };

    const auto refreshFacets = [&storage, &recordings, &app]() {
        storage.withRecordings([&storage, &recordings, &app](softphone::core::IRecordingStore &) {

            const auto offered = [&storage](softphone::core::TaxonomyKind kind) {
                QStringList out;
                std::vector<softphone::core::TaxonomyEntry> known;
                if (softphone::core::succeeded(storage.taxonomy()->entries(kind, false, known))) {
                    for (const auto &entry : known) {
                        if (!entry.code.empty()) {
                            out.push_back(QString::fromStdString(entry.code));
                        }
                    }
                }

                std::vector<std::string> codes;
                static_cast<void>(storage.taxonomy()->usedCodes(kind, codes));
                for (const auto &code : codes) {
                    const auto text = QString::fromStdString(code);
                    if (!text.isEmpty() && !out.contains(text)) {
                        out.push_back(text);
                    }
                }
                return out;
            };
            auto theCategories = offered(softphone::core::TaxonomyKind::Category);
            auto theTags = offered(softphone::core::TaxonomyKind::Tag);
            auto theFlags = offered(softphone::core::TaxonomyKind::RedFlag);

            QMetaObject::invokeMethod(
                &app,
                [&recordings, theCategories, theTags, theFlags]() mutable {
                    recordings.setFacets(std::move(theCategories), std::move(theTags),
                                         std::move(theFlags));
                },
                Qt::QueuedConnection);
        });
    };
    refreshFacets();

    QObject::connect(&recordings, &softphone::ui::RecordingsViewModel::moreWanted, &app,
                     [&storage, &recordings, &app](int offset) {
                         auto asked = recordings.query();
                         asked.limit = softphone::ui::RecordingsViewModel::pageSize();
                         asked.offset = static_cast<std::size_t>(std::max(offset, 0));

                         storage.withRecordings([&recordings, &app, query = std::move(asked)](
                                                    softphone::core::IRecordingStore &store) {
                             std::vector<softphone::core::Recording> found;
                             const auto status = store.recordings(query, found);

                             QMetaObject::invokeMethod(
                                 &app,
                                 [&recordings, status, found]() mutable {
                                     recordings.appendRecordings(
                                         std::move(found),
                                         status == softphone::core::StoreStatus::Ok);
                                 },
                                 Qt::QueuedConnection);
                         });
                     });

    recordings.setActions(
        refreshRecordings,
        [&storage, &host, &recordings, &app](const QString &id, bool play) {

            storage.withRecordings([&host, &recordings, &app, wanted = id.toStdString(), play,
                                    id](softphone::core::IRecordingStore &store) {
                softphone::core::Recording row;
                if (store.recording(wanted, row) != softphone::core::StoreStatus::Ok) {
                    return;
                }

                const auto file = softphone::core::recordingsDirectory() / row.filePath;
                host.playRecording(file, play);

                static_cast<void>(store.noteAccess(wanted, softphone::core::RecordingAccess::Played,
                                                   row.filePath));

                QThreadPool::globalInstance()->start([&recordings, &app, file, id]() {
                    softphone::media::Waveform picture;
                    if (!softphone::media::waveformFor(file, picture)) {
                        return;
                    }

                    std::vector<double> nearSide;
                    std::vector<double> farSide;
                    nearSide.reserve(picture.nearSide.size());
                    farSide.reserve(picture.farSide.size());
                    for (const auto value : picture.nearSide) {
                        nearSide.push_back(value / 255.0);
                    }
                    for (const auto value : picture.farSide) {
                        farSide.push_back(value / 255.0);
                    }

                    softphone::ui::raiseOffTheNoiseFloor(std::span{nearSide});
                    softphone::ui::raiseOffTheNoiseFloor(std::span{farSide});

                    QMetaObject::invokeMethod(
                        &app,
                        [&recordings, id, nearSide, farSide]() mutable {
                            recordings.setWaveform(id, std::move(nearSide), std::move(farSide));
                        },
                        Qt::QueuedConnection);
                });
            });
        },
        [&host]() { host.stopPlaying(); }, [&host]() { host.pausePlaying(true); },
        [&host]() { host.pausePlaying(false); },
        [&host](qint64 positionMs) {
            host.seekPlayback(static_cast<std::uint64_t>(std::max<qint64>(0, positionMs)));
        },
        [&host](double speed) { host.setPlaybackSpeed(speed); },
        [&host](bool nearSide, bool farSide) { host.setPlaybackChannels(nearSide, farSide); },
        [&storage, refreshRecordings](const QString &id) {
            storage.withRecordings([wanted = id.toStdString(),
                                    refreshRecordings](softphone::core::IRecordingStore &store) {
                softphone::core::RecordingLibrary shelf{&store, softphone::core::dataDirectory() /
                                                                    "recordings"};
                static_cast<void>(shelf.remove(wanted));
                refreshRecordings();
            });
        },
        [&storage, refreshRecordings](const QString &id) {
            storage.withRecordings([wanted = id.toStdString(),
                                    refreshRecordings](softphone::core::IRecordingStore &store) {
                softphone::core::Recording row;
                if (store.recording(wanted, row) != softphone::core::StoreStatus::Ok) {
                    return;
                }
                row.pinned = !row.pinned;
                static_cast<void>(store.save(row));
                refreshRecordings();
            });
        },
        [&storage, &app, &progressModel](const QString &id, const QString &target) {

            storage.withRecordings([&app, &progressModel, wanted = id.toStdString(),
                                    where = softphone::core::fromUtf8(target.toStdString())](
                                       softphone::core::IRecordingStore &store) {
                softphone::core::Recording row;
                if (store.recording(wanted, row) != softphone::core::StoreStatus::Ok ||
                    row.filePath.empty()) {
                    return;
                }

                const auto from = softphone::core::recordingsDirectory() / row.filePath;

                std::error_code failed;
                std::filesystem::copy_file(
                    from, where, std::filesystem::copy_options::overwrite_existing, failed);

                if (failed) {
                    softphone::core::Logger::instance().log(
                        softphone::core::LogLevel::Warning, "ui",
                        "the recording was not saved to " + softphone::core::asUtf8(where) + ": " +
                            failed.message());
                    return;
                }

                static_cast<void>(store.noteAccess(wanted,
                                                   softphone::core::RecordingAccess::Exported,
                                                   softphone::core::asUtf8(where)));

                QMetaObject::invokeMethod(
                    &app,
                    [&progressModel]() {
                        progressModel.note(QStringLiteral("library.export.recording"));
                    },
                    Qt::QueuedConnection);
            });
        },
        [&storage, &app, &progressModel, refreshRecordings](const QString &id,
                                                            const QString &name) {
            storage.withRecordings([&app, &progressModel, wanted = id.toStdString(),
                                    called = name.toStdString(),
                                    refreshRecordings](softphone::core::IRecordingStore &store) {
                softphone::core::Recording row;
                if (store.recording(wanted, row) != softphone::core::StoreStatus::Ok) {
                    return;
                }

                const auto captured = row.source == softphone::core::RecordingSource::External;
                if (row.source != softphone::core::RecordingSource::Native) {
                    row.remoteName = called;
                } else {
                    row.title = called;
                }
                static_cast<void>(store.save(row));
                refreshRecordings();

                if (captured) {
                    QMetaObject::invokeMethod(
                        &app,
                        [&progressModel]() { progressModel.note(QStringLiteral("capture.named")); },
                        Qt::QueuedConnection);
                }
            });
        });

    softphone::http::HttplibHttpClient asrTransport;

    softphone::core::JobRunner jobs{*storage.jobs()};
    softphone::core::TranscriptionService pipeline{*storage.library(), *storage.transcripts(),
                                                   *storage.providers(), jobs,
                                                   softphone::core::recordingsDirectory()};

    static softphone::http::IxWebSocketFactory batchSockets;

    const auto asrProviderFor =
        [&asrTransport](const std::string &type) -> std::shared_ptr<softphone::core::IAsrProvider> {
        if (type == "openai") {
            return std::make_shared<softphone::http::OpenAiAsrProvider>(asrTransport);
        }
        if (type == "deepgram") {
            return std::make_shared<softphone::http::DeepgramAsrProvider>(asrTransport);
        }
        if (type == "elevenlabs") {
            return std::make_shared<softphone::http::ElevenLabsAsrProvider>(asrTransport);
        }
        if (type == "speechmatics") {
            return std::make_shared<softphone::http::SpeechmaticsAsrProvider>(asrTransport);
        }
        if (type == "xai") {
            return std::make_shared<softphone::http::XaiAsrProvider>(asrTransport);
        }
        if (type == "yandex") {
            return std::make_shared<softphone::http::YandexAsrProvider>(asrTransport);
        }

        if (type == "assemblyai") {
            return std::make_shared<softphone::http::AssemblyAiAsrProvider>(asrTransport);
        }
        if (type == "soniox") {
            return std::make_shared<softphone::http::SonioxAsrProvider>(asrTransport);
        }

        if (type == "vosk" || type == "whisper-live" || type == "riva") {
            return std::make_shared<softphone::core::StreamedTranscription>(
                type, [type] { return softphone::http::makeStreamingAsr(type, batchSockets); },
                softphone::media::makePcmReader());
        }

        return {};
    };

    const auto llmProviderFor =
        [&asrTransport](const std::string &type) -> std::shared_ptr<softphone::core::ILlmProvider> {
        if (type == "anthropic") {
            return std::make_shared<softphone::http::AnthropicChatProvider>(asrTransport);
        }
        if (type == "openai") {
            return std::make_shared<softphone::http::OpenAiChatProvider>(asrTransport);
        }

        if (type == "xai" || type == "yandex") {
            return std::make_shared<softphone::http::OpenAiChatProvider>(asrTransport, type);
        }
        return {};
    };

    pipeline.setProviderMaker([asrProviderFor](const softphone::core::ProviderConfig &configured) {
        return asrProviderFor(configured.type);
    });

    softphone::core::GenerationService generation{*storage.library(),   *storage.transcripts(),
                                                  *storage.templates(), *storage.artifacts(),
                                                  *storage.providers(), vocabulary,
                                                  *storage.usage(),     jobs};

    generationService = &generation;

    generation.setInterfaceLanguage(
        softphone::ui::installTranslations(
            app, uiRegistry,
            softphone::ui::offeredLanguage(
                uiRegistry,
                QString::fromStdString(
                    softphone::core::appearanceFrom(
                        softphone::ui::groupText(host.storedConfiguration(), "appearance"))
                        .language)))
            .toStdString());

    generation.setProviderMaker(
        [llmProviderFor](const softphone::core::ProviderConfig &configured) {
            return llmProviderFor(configured.type);
        });

    softphone::core::PipelineEngine rules{*storage.library(),   *storage.transcripts(),
                                          vocabulary,           *storage.automations(),
                                          *storage.templates(), *storage.usage()};

    rules.setActions(
        [&pipeline](const std::string &recordingId, const std::string &providerId, int priority,
                    const std::string &automationId) {
            return pipeline.transcribe(recordingId, providerId, {}, priority, automationId);
        },
        [&generation](const std::string &recordingId, const std::string &templateId,
                      const std::string &providerId, int priority,
                      const std::string &automationId) {
            return generation.generate(recordingId, templateId, providerId, priority, {},
                                       automationId);
        },

        [&storage](const std::string &recordingId) {
            auto *queue = storage.jobs();
            if (queue == nullptr) {
                return false;
            }
            for (const auto state :
                 {softphone::core::JobState::Queued, softphone::core::JobState::Running}) {
                softphone::core::JobQuery asking;
                asking.recordingId = recordingId;
                asking.state = state;
                asking.limit = 1;
                std::vector<softphone::core::Job> found;
                if (queue->jobs(asking, found) == softphone::core::StoreStatus::Ok &&
                    !found.empty()) {
                    return true;
                }
            }
            return false;
        });

    const auto refreshSpend = [&rules, &settings, &app]() {
        const auto spent = rules.spend();

        QString line;
        if (spent.month.calls == 0) {
            line = QCoreApplication::translate("PipelineSpend",
                                               "Nothing has been processed this month.");
        } else if (spent.month.costMicros > 0) {

            line = QObject::tr("This month: %1, over %n request(s).", nullptr, spent.month.calls)
                       .arg(QLocale{}.toString(
                           static_cast<double>(spent.month.costMicros) / 1'000'000.0, 'f', 2));
        } else {

            line = QObject::tr("This month: %1 tokens, over %n request(s), at no charge.", nullptr,
                               spent.month.calls)
                       .arg(QLocale{}.toString(static_cast<qlonglong>(spent.month.promptTokens +
                                                                      spent.month.answerTokens)));
        }

        if (spent.paused) {

            line = QCoreApplication::translate(
                       "PipelineSpend",
                       "%1 The limit has been reached, so the rules have stopped until the month "
                       "turns. You can still ask for things yourself.")
                       .arg(line);
        }

        QMetaObject::invokeMethod(
            &app, [&settings, line, paused = spent.paused]() { settings.applySpend(line, paused); },
            Qt::QueuedConnection);
    };

    generation.setSecretLookup([&host](const std::string &providerId) {
        std::string token;
        static_cast<void>(
            host.keyring().fetch(softphone::core::secretKeys::forProvider(providerId), token));
        return token;
    });

    pipeline.setSecretLookup([&host](const std::string &providerId) {
        std::string token;
        static_cast<void>(
            host.keyring().fetch(softphone::core::secretKeys::forProvider(providerId), token));
        return token;
    });

    pipeline.setPreparer(softphone::media::makeAsrPreparer(softphone::core::recordingsDirectory() /
                                                           softphone::core::kScratch));

    pipeline.attach();
    generation.attach();

    struct Readings {
        std::vector<softphone::core::Transcript> found;
        std::string chosen;
    };

    const auto readTranscripts = [&storage](const std::string &wanted, const std::string &show) {
        Readings readings;
        static_cast<void>(storage.transcripts()->transcripts(wanted, readings.found));

        readings.chosen = show;
        if (readings.chosen.empty()) {
            const auto record =
                std::find_if(readings.found.begin(), readings.found.end(),
                             [](const softphone::core::Transcript &one) { return one.isPrimary; });
            if (record != readings.found.end()) {
                readings.chosen = record->id;
            } else if (!readings.found.empty()) {
                readings.chosen = readings.found.front().id;
            }
        }
        for (auto &one : readings.found) {
            if (one.id == readings.chosen) {
                softphone::core::Transcript whole;
                if (storage.transcripts()->transcript(one.id, whole) ==
                    softphone::core::StoreStatus::Ok) {
                    one = std::move(whole);
                }
            }
        }
        return readings;
    };

    const auto refreshTranscripts = [&storage, &transcript, &app, readTranscripts](
                                        const QString &recordingId, const QString &show) {
        if (recordingId.isEmpty()) {
            QMetaObject::invokeMethod(
                &app, [&transcript]() { transcript.clear(); }, Qt::QueuedConnection);
            return;
        }
        storage.withRecordings([&transcript, &app, readTranscripts,
                                wanted = recordingId.toStdString(),
                                show = show.toStdString()](softphone::core::IRecordingStore &) {
            auto readings = readTranscripts(wanted, show);

            QMetaObject::invokeMethod(
                &app,
                [&transcript, found = std::move(readings.found),
                 chosen = QString::fromStdString(readings.chosen)]() mutable {
                    transcript.setTranscripts(std::move(found), chosen);
                },
                Qt::QueuedConnection);
        });
    };

    transcript.setActions(
        [&recordings, &providers, &pipeline, &transcript](const QString &providerId) {
            const auto id = recordings.openId();
            if (id.isEmpty()) {
                return;
            }

            const auto status =
                pipeline.transcribe(id.toStdString(), providerId.toStdString(), {}, 10);
            if (softphone::core::succeeded(status)) {
                transcript.refuse(softphone::ui::TranscriptViewModel::Refusal::None);

                transcript.setWorking(true);
                return;
            }

            using Refusal = softphone::ui::TranscriptViewModel::Refusal;
            const auto why = !recordings.openHasAudio()                        ? Refusal::NoAudio
                             : providers.count() == 0                          ? Refusal::NoProvider
                             : status == softphone::core::StoreStatus::Missing ? Refusal::NoProvider
                                                                               : Refusal::Failed;
            transcript.setWorking(false);
            transcript.refuse(why);
        },
        [&recordings, refreshTranscripts](const QString &transcriptId) {
            refreshTranscripts(recordings.openId(), transcriptId);
        },
        [&storage, &recordings, &transcript, refreshTranscripts]() {
            const auto chosen = transcript.openId();
            const auto recording = recordings.openId();
            storage.withRecordings([&storage, chosen, recording,
                                    refreshTranscripts](softphone::core::IRecordingStore &) {
                static_cast<void>(storage.transcripts()->makePrimary(chosen.toStdString()));
                refreshTranscripts(recording, chosen);
            });
        },
        [&storage, &recordings, &transcript, refreshTranscripts]() {
            const auto chosen = transcript.openId();
            const auto recording = recordings.openId();
            storage.withRecordings([&storage, chosen, recording,
                                    refreshTranscripts](softphone::core::IRecordingStore &) {
                std::vector<std::string> files;
                static_cast<void>(
                    storage.transcripts()->removeTranscript(chosen.toStdString(), files));

                for (const auto &file : files) {
                    std::error_code ignored;
                    std::filesystem::remove(softphone::core::recordingsDirectory() / file, ignored);
                }
                refreshTranscripts(recording, {});
            });
        },
        [](const QString &target, const QString &text) {
            std::ofstream out{softphone::core::fromUtf8(target.toStdString()), std::ios::binary};
            if (!out) {
                softphone::core::Logger::instance().log(softphone::core::LogLevel::Warning, "ui",
                                                        "the transcript was not saved to " +
                                                            target.toStdString());
                return;
            }
            const auto bytes = text.toUtf8();
            out.write(bytes.constData(), bytes.size());
        },
        [](const QString &rawPath, const QString &target) {

            const auto from = softphone::core::recordingsDirectory() /
                              softphone::core::fromUtf8(rawPath.toStdString());
            std::error_code failed;
            std::filesystem::copy_file(from, softphone::core::fromUtf8(target.toStdString()),
                                       std::filesystem::copy_options::overwrite_existing, failed);
            if (failed) {
                softphone::core::Logger::instance().log(softphone::core::LogLevel::Warning, "ui",
                                                        "the provider's answer was not saved to " +
                                                            target.toStdString() + ": " +
                                                            failed.message());
            }
        });

    const auto refreshArtifacts = [&storage, &artifacts, &recordings, &app]() {
        const auto open = recordings.openId();
        if (open.isEmpty()) {
            QMetaObject::invokeMethod(
                &app, [&artifacts]() { artifacts.setArtifacts({}); }, Qt::QueuedConnection);
            return;
        }

        storage.withRecordings([&storage, &artifacts, &app,
                                wanted = open.toStdString()](softphone::core::IRecordingStore &) {
            softphone::core::ArtifactQuery query;
            query.recordingId = wanted;
            std::vector<softphone::core::Artifact> found;
            static_cast<void>(storage.artifacts()->artifacts(query, found));

            QMetaObject::invokeMethod(
                &app,
                [&artifacts, found = std::move(found)]() mutable {
                    artifacts.setArtifacts(std::move(found));
                },
                Qt::QueuedConnection);
        });
    };

    const auto refreshBothReadings = [&storage, &transcript, &artifacts, &app,
                                      readTranscripts](const QString &recordingId) {
        if (recordingId.isEmpty()) {
            QMetaObject::invokeMethod(
                &app,
                [&transcript, &artifacts]() {
                    transcript.clear();
                    artifacts.setArtifacts({});
                },
                Qt::QueuedConnection);
            return;
        }

        storage.withRecordings(
            [&storage, &transcript, &artifacts, &app, readTranscripts,
             wanted = recordingId.toStdString()](softphone::core::IRecordingStore &) {
                auto readings = readTranscripts(wanted, {});

                softphone::core::ArtifactQuery query;
                query.recordingId = wanted;
                std::vector<softphone::core::Artifact> made;
                static_cast<void>(storage.artifacts()->artifacts(query, made));

                QMetaObject::invokeMethod(
                    &app,
                    [&transcript, &artifacts, found = std::move(readings.found),
                     chosen = QString::fromStdString(readings.chosen),
                     made = std::move(made)]() mutable {

                        artifacts.setArtifacts(std::move(made));
                        transcript.setTranscripts(std::move(found), chosen);
                    },
                    Qt::QueuedConnection);
            });
    };

    const auto refreshPrompts = [&storage, &artifacts, &app]() {
        storage.withRecordings([&storage, &artifacts, &app](softphone::core::IRecordingStore &) {
            std::vector<softphone::core::PromptTemplate> found;
            static_cast<void>(storage.templates()->templates(found));

            softphone::core::ProviderConfig model;
            const auto haveModel =
                storage.providers()->defaultProvider(softphone::core::ProviderKind::Llm, model) ==
                softphone::core::StoreStatus::Ok;

            QMetaObject::invokeMethod(
                &app,
                [&artifacts, found = std::move(found), haveModel]() mutable {
                    artifacts.setTemplates(std::move(found), haveModel);
                },
                Qt::QueuedConnection);
        });
    };

    const auto refreshTemplates = [&storage, &templates, &app, &prompter, &tellPrompter,
                                   promptable]() {
        storage.withRecordings([&storage, &templates, &app, &prompter, &tellPrompter,
                                promptable](softphone::core::IRecordingStore &) {
            std::vector<softphone::core::PromptTemplate> found;
            static_cast<void>(storage.templates()->templates(found));

            std::vector<softphone::core::ProviderConfig> theModels;
            static_cast<void>(
                storage.providers()->providers(softphone::core::ProviderKind::Llm, theModels));

            std::vector<softphone::core::ProviderConfig> everyRecogniser;
            static_cast<void>(storage.providers()->providers(softphone::core::ProviderKind::Asr,
                                                             everyRecogniser));
            std::vector<softphone::core::ProviderConfig> streaming;
            for (auto &one : everyRecogniser) {
                if (softphone::http::canStream(one.type)) {
                    streaming.push_back(std::move(one));
                }
            }

            QMetaObject::invokeMethod(
                &app,
                [&templates, &prompter, &tellPrompter, promptable, found = std::move(found),
                 theModels = std::move(theModels), streaming = std::move(streaming)]() mutable {

                    (*promptable)[0] = !streaming.empty();
                    prompter.setTemplates(found);
                    tellPrompter();

                    templates.setModels(std::move(theModels));
                    templates.setRecognisers(std::move(streaming));
                    templates.setTemplates(std::move(found));
                },
                Qt::QueuedConnection);
        });
    };

    const auto refreshAutomations = [&storage, &automations, &app]() {
        storage.withRecordings([&storage, &automations, &app](softphone::core::IRecordingStore &) {
            std::vector<softphone::core::Automation> found;
            static_cast<void>(storage.automations()->automations(found));

            std::vector<softphone::core::PromptTemplate> prompts;
            static_cast<void>(storage.templates()->templates(prompts));

            QMetaObject::invokeMethod(
                &app,
                [&automations, found = std::move(found), prompts = std::move(prompts)]() mutable {
                    automations.setPrompts(std::move(prompts));
                    automations.setAutomations(std::move(found));
                },
                Qt::QueuedConnection);
        });
    };

    templates.setActions(
        [&storage, refreshTemplates, refreshPrompts,
         contentLanguage](softphone::core::PromptTemplate row) {
            storage.withRecordings(
                [&storage, refreshTemplates, refreshPrompts, contentLanguage,
                 row = std::move(row)](softphone::core::IRecordingStore &) mutable {

                    if (row.builtIn && !row.code.empty()) {

                        const auto shipped =
                            softphone::core::shippedTemplate(row.code, contentLanguage);
                        if (shipped.has_value() &&
                            (shipped->body != row.body || shipped->system != row.system)) {
                            row.builtIn = false;
                        }
                    }
                    static_cast<void>(storage.templates()->save(row));
                    refreshTemplates();
                    refreshPrompts();
                });
        },
        [&storage, refreshTemplates, refreshPrompts](const QString &id) {
            storage.withRecordings([&storage, refreshTemplates, refreshPrompts,
                                    wanted = id.toStdString()](softphone::core::IRecordingStore &) {
                static_cast<void>(storage.templates()->removeTemplate(wanted));
                refreshTemplates();
                refreshPrompts();
            });
        },
        [&storage, &templates, &app, &generation](softphone::core::PromptTemplate row) {
            storage.withRecordings([&storage, &templates, &app, &generation, row = std::move(row)](
                                       softphone::core::IRecordingStore &) mutable {

                softphone::core::RecordingQuery query;
                query.transcribed = true;
                query.limit = 1;
                std::vector<softphone::core::Recording> found;
                static_cast<void>(storage.library()->recordings(query, found));

                QString text;
                QString problem;
                QString source;
                if (found.empty()) {

                    problem = QCoreApplication::translate(
                        "TemplatesViewModel",
                        "Nothing has been transcribed yet, so there is nothing to try this "
                        "against.");
                } else {

                    softphone::core::Transcript reading;
                    static_cast<void>(
                        storage.transcripts()->primaryTranscript(found.front().id, reading));

                    softphone::core::PromptContext context;
                    context.transcript = softphone::core::asTurns(reading);
                    context.duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                        found.front().duration);
                    context.party = found.front().remoteName.empty() ? found.front().remoteNumber
                                                                     : found.front().remoteName;
                    context.language = reading.detectedLanguage;
                    context.question = "What was agreed?";

                    if (softphone::core::isRealtime(row)) {

                        const auto said = reading.detectedLanguage.empty()
                                              ? reading.requestedLanguage
                                              : reading.detectedLanguage;
                        std::vector<std::string> before;
                        for (const auto &segment : reading.segments) {
                            if (segment.text.empty()) {
                                continue;
                            }
                            const auto voice = softphone::core::parseVoice(segment.voice);
                            const auto side =
                                voice.has_value() ? voice->first : softphone::core::Voice::Mixed;
                            if (side == softphone::core::Voice::Far ||
                                side == softphone::core::Voice::Mixed) {
                                if (!context.reply.empty()) {
                                    before.push_back(std::string{softphone::core::speakerWord(
                                                         softphone::core::Voice::Far, said)} +
                                                     ": " + context.reply);
                                }
                                context.reply = segment.text;
                                continue;
                            }
                            before.push_back(std::string{softphone::core::speakerWord(side, said)} +
                                             ": " + segment.text);
                        }

                        const auto from = before.size() > softphone::core::kRepliesOfContext
                                              ? before.size() - softphone::core::kRepliesOfContext
                                              : 0u;
                        for (auto at = from; at < before.size(); ++at) {
                            if (!context.conversation.empty()) {
                                context.conversation += "\n\n";
                            }
                            context.conversation += before[at];
                        }
                    }
                    static_cast<void>(storage.taxonomy()->entries(
                        softphone::core::TaxonomyKind::Category, false, context.categories));
                    static_cast<void>(storage.taxonomy()->entries(
                        softphone::core::TaxonomyKind::Tag, false, context.tags));
                    static_cast<void>(storage.taxonomy()->entries(
                        softphone::core::TaxonomyKind::RedFlag, false, context.redFlags));

                    static_cast<void>(storage.taxonomy()->entries(
                        softphone::core::TaxonomyKind::Instruction, true, context.instructions));

                    const auto when = QDateTime::fromSecsSinceEpoch(
                        std::chrono::duration_cast<std::chrono::seconds>(
                            found.front().startedAt.time_since_epoch())
                            .count());
                    source = QCoreApplication::translate(
                                 "TemplatesViewModel",
                                 "Built from the most recent transcribed conversation: %1, %2.")
                                 .arg(context.party.empty() ? QCoreApplication::translate(
                                                                  "TemplatesViewModel", "no number")
                                                            : QString::fromStdString(context.party),
                                      QLocale{}.toString(when, QLocale::ShortFormat));

                    const auto rendered =
                        softphone::core::renderPrompt(row, context, generation.interfaceLanguage());
                    for (const auto &message : rendered.messages) {
                        text +=
                            QString::fromUtf8(softphone::core::toString(message.role).data(),
                                              static_cast<qsizetype>(
                                                  softphone::core::toString(message.role).size()))
                                .toUpper() +
                            QStringLiteral("\n") + QString::fromStdString(message.text) +
                            QStringLiteral("\n\n");
                    }
                }

                QMetaObject::invokeMethod(
                    &app,
                    [&templates, text, problem, source]() {
                        templates.setPreview(text, problem, source);
                    },
                    Qt::QueuedConnection);
            });
        },
        refreshTemplates,
        [&storage, refreshTemplates, refreshPrompts, refreshAutomations]() {
            storage.withRecordings([&storage, refreshTemplates, refreshPrompts,
                                    refreshAutomations](softphone::core::IRecordingStore &) {

                static_cast<void>(storage.templates()->restoreShippedTemplates());
                static_cast<void>(storage.automations()->restoreShippedAutomations());
                refreshTemplates();
                refreshPrompts();
                refreshAutomations();
            });
        },
        [&storage, refreshTemplates,
         refreshPrompts](std::vector<std::pair<std::string, int>> places) {
            storage.withRecordings(
                [&storage, refreshTemplates, refreshPrompts,
                 places = std::move(places)](softphone::core::IRecordingStore &) {
                    static_cast<void>(storage.templates()->orderTemplates(places));
                    refreshTemplates();

                    refreshPrompts();
                });
        });

    artifacts.setActions(
        [&generation, &recordings, &app, refreshArtifacts](const QString &templateId,
                                                           const QString &question) {
            const auto open = recordings.openId();
            if (open.isEmpty()) {
                return;
            }

            static_cast<void>(generation.generate(open.toStdString(), templateId.toStdString(), {},
                                                  1, question.toStdString()));
            QMetaObject::invokeMethod(&app, refreshArtifacts, Qt::QueuedConnection);
        },
        [&storage, refreshArtifacts](const QString &artifactId) {
            storage.withRecordings([&storage, refreshArtifacts, wanted = artifactId.toStdString()](
                                       softphone::core::IRecordingStore &) {
                static_cast<void>(storage.artifacts()->removeArtifact(wanted));
                refreshArtifacts();
            });
        },

        [&progressModel](const QString &target, const QString &text) {
            std::ofstream out{softphone::core::fromUtf8(target.toStdString()), std::ios::binary};
            if (!out) {
                softphone::core::Logger::instance().log(softphone::core::LogLevel::Warning, "ui",
                                                        "the answer was not saved to " +
                                                            target.toStdString());
                return;
            }
            const auto bytes = text.toUtf8();
            out.write(bytes.constData(), bytes.size());

            progressModel.note(QStringLiteral("library.export.summary"));
        },
        refreshPrompts);

    artifacts.setVocabulary([&taxonomy](const QString &kind, const QString &code) {
        return taxonomy.titleOf(kind, code);
    });

    generation.watch([&app, refreshArtifacts, refreshSpend, refreshFacets,
                      refreshOneRecording](const softphone::core::Artifact &made) {
        QMetaObject::invokeMethod(&app, refreshArtifacts, Qt::QueuedConnection);
        refreshSpend();

        if (made.status == softphone::core::ArtifactStatus::Done) {
            refreshOneRecording(made.recordingId);
            refreshFacets();
        }
    });

    const auto sayWhetherItIsBeingTranscribed = [&jobsModel, &recordings, &transcript]() {
        transcript.setWorking(jobsModel.transcribing(recordings.openId()));
    };

    QObject::connect(&recordings, &softphone::ui::RecordingsViewModel::openChanged, &app,
                     [&recordings, &transcript, &artifacts, refreshBothReadings,
                      sayWhetherItIsBeingTranscribed]() {
                         const auto id = recordings.openId();

                         auto stem = recordings.exportName();
                         if (const auto dot = stem.lastIndexOf(QLatin1Char{'.'}); dot > 0) {
                             stem = stem.left(dot);
                         }
                         transcript.setSubject(stem, recordings.openLabel(), {});
                         artifacts.setStem(stem);

                         sayWhetherItIsBeingTranscribed();

                         refreshBothReadings(id);
                     });

    pipeline.watch([&recordings, &app, refreshTranscripts, refreshFacets, refreshOneRecording,
                    sayWhetherItIsBeingTranscribed](const softphone::core::Transcript &written) {
        QMetaObject::invokeMethod(
            &app,
            [&recordings, refreshTranscripts, refreshFacets, refreshOneRecording,
             sayWhetherItIsBeingTranscribed, about = QString::fromStdString(written.recordingId),
             which = QString::fromStdString(written.id)]() {

                sayWhetherItIsBeingTranscribed();
                refreshFacets();

                refreshOneRecording(about.toStdString());

                if (recordings.openId() == about) {

                    refreshTranscripts(about, which);
                }
            },
            Qt::QueuedConnection);
    });

    const auto refreshJobs = [&storage, &jobsModel, &app, sayWhetherItIsBeingTranscribed]() {
        storage.withRecordings([&storage, &jobsModel, &app, sayWhetherItIsBeingTranscribed](
                                   softphone::core::IRecordingStore &store) {
            auto *queue = storage.jobs();
            if (queue == nullptr) {
                return;
            }

            softphone::core::JobQuery asking;
            asking.limit = 100;
            std::vector<softphone::core::Job> found;
            if (!softphone::core::succeeded(queue->jobs(asking, found))) {
                return;
            }

            std::vector<softphone::ui::JobsViewModel::Entry> rows;
            for (const auto &job : found) {

                if (job.state == softphone::core::JobState::Done ||
                    job.state == softphone::core::JobState::Cancelled) {
                    continue;
                }

                softphone::ui::JobsViewModel::Entry entry;
                entry.id = QString::fromStdString(job.id);
                entry.recordingId = QString::fromStdString(job.recordingId);
                entry.state = job.state;
                entry.kind = job.kind;
                entry.attempts = job.attempts;
                entry.error = QString::fromStdString(job.error);
                entry.sinceMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                                    job.updatedAt.time_since_epoch())
                                    .count();
                entry.queuedAtMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                                       job.createdAt.time_since_epoch())
                                       .count();

                softphone::core::Recording row;
                if (store.recording(job.recordingId, row) == softphone::core::StoreStatus::Ok) {
                    entry.party = QString::fromStdString(row.remoteName.empty() ? row.remoteNumber
                                                                                : row.remoteName);
                    entry.startedAtMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                                            row.startedAt.time_since_epoch())
                                            .count();
                }
                rows.push_back(std::move(entry));
            }

            QMetaObject::invokeMethod(
                &app,
                [&jobsModel, sayWhetherItIsBeingTranscribed, rows = std::move(rows)]() mutable {
                    jobsModel.setJobs(std::move(rows));

                    sayWhetherItIsBeingTranscribed();
                },
                Qt::QueuedConnection);
        });
    };

    jobsModel.setActions(
        [&jobs, refreshJobs](const QString &id) {
            static_cast<void>(jobs.cancel(id.toStdString()));
            refreshJobs();
        },
        [&storage, refreshJobs](const QString &id) {
            storage.withRecordings([&storage, refreshJobs,
                                    wanted = id.toStdString()](softphone::core::IRecordingStore &) {
                if (auto *queue = storage.jobs(); queue != nullptr) {
                    static_cast<void>(queue->forgetJob(wanted));
                }
                refreshJobs();
            });
        },
        refreshJobs);

    jobs.watch([refreshJobs, &rules](const softphone::core::Job &job) {
        refreshJobs();
        if (job.state == softphone::core::JobState::Done && !job.recordingId.empty() &&
            !job.automationId.empty()) {
            rules.advance(job.recordingId);
        }
    });

    refreshJobs();

    const auto refreshProviders = [&storage, &providers, &transcript, &automations, &app]() {
        storage.withRecordings([&storage, &providers, &transcript, &automations,
                                &app](softphone::core::IRecordingStore &) {
            std::vector<softphone::core::ProviderConfig> found;
            const auto status =
                storage.providers()->providers(softphone::core::ProviderKind::Asr, found);

            std::vector<std::pair<QString, QString>> names;
            names.reserve(found.size());
            for (const auto &one : found) {
                names.emplace_back(QString::fromStdString(one.id),
                                   QString::fromStdString(one.name));
            }

            QMetaObject::invokeMethod(
                &app,
                [&providers, &transcript, &automations, found = std::move(found),
                 names = std::move(names), status]() mutable {

                    automations.setRecognisers(found);
                    providers.setProviders(std::move(found),
                                           status == softphone::core::StoreStatus::Ok);
                    transcript.setProviderNames(std::move(names));
                },
                Qt::QueuedConnection);
        });
    };

    const auto stillHere = std::make_shared<std::atomic<bool>>(true);

    const auto tokenFor = [&host](const softphone::core::ProviderConfig &row,
                                  const QString &typed) {
        if (!typed.isEmpty()) {
            return typed.toStdString();
        }
        std::string stored;
        static_cast<void>(
            host.keyring().fetch(softphone::core::secretKeys::forProvider(row.id), stored));
        return stored;
    };

    providers.setActions(
        [&storage, &host, refreshProviders](softphone::core::ProviderConfig row,
                                            const QString &secret) {
            storage.withRecordings([&storage, &host, refreshProviders, row = std::move(row),
                                    secret](softphone::core::IRecordingStore &) mutable {
                if (!softphone::core::succeeded(storage.providers()->save(row))) {
                    return;
                }

                if (!secret.isEmpty()) {
                    static_cast<void>(host.keyring().store(
                        softphone::core::secretKeys::forProvider(row.id), secret.toStdString()));
                }
                refreshProviders();
            });
        },
        [&storage, &host, refreshProviders](const QString &id) {
            storage.withRecordings([&storage, &host, refreshProviders,
                                    wanted = id.toStdString()](softphone::core::IRecordingStore &) {
                static_cast<void>(storage.providers()->removeProvider(wanted));

                static_cast<void>(
                    host.keyring().erase(softphone::core::secretKeys::forProvider(wanted)));
                refreshProviders();
            });
        },
        [asrProviderFor, &providers, &app, stillHere, tokenFor](softphone::core::ProviderConfig row,
                                                                const QString &secret) {

            std::thread{[asrProviderFor, &providers, &app, stillHere, tokenFor,
                         row = std::move(row), secret]() {
                softphone::core::AsrEndpoint where;
                where.url = row.endpoint;
                where.model = row.model;
                where.region = row.region;
                where.params = row.params;
                where.timeout = std::chrono::seconds{20};

                if (!stillHere->load()) {
                    return;
                }
                where.token = tokenFor(row, secret);

                softphone::core::AsrProbe answer;
                if (const auto provider = asrProviderFor(row.type)) {
                    answer = provider->probe(where);
                } else {
                    answer.reachable = false;
                    answer.status = softphone::core::AsrStatus::Unsupported;
                    answer.detail = "this build does not know that kind of recogniser";
                }

                softphone::core::Logger::instance().log(
                    softphone::core::LogLevel::Info, "ui",
                    "a recogniser was tested: " + row.type + " at " + row.endpoint +
                        (answer.reachable ? " answered" : " refused") + ", offering " +
                        std::to_string(answer.models.size()) + " model(s)");

                if (!stillHere->load()) {
                    return;
                }
                QMetaObject::invokeMethod(
                    &app, [&providers, answer]() { providers.setTestResult(answer); },
                    Qt::QueuedConnection);
            }}.detach();
        },
        refreshProviders);

    refreshProviders();

    models.setKind(softphone::core::ProviderKind::Llm);

    const auto offerByCountry = [&providers, &models, &about]() {
        providers.setCountry(about.country());
        models.setCountry(about.country());
    };
    QObject::connect(&about, &softphone::ui::About::countryChanged, &about, offerByCountry);
    offerByCountry();

    const auto refreshModels = [&storage, &models, &automations, &artifacts, &app,
                                refreshPrompts]() {
        storage.withRecordings([&storage, &models, &automations, &artifacts,
                                &app](softphone::core::IRecordingStore &) {
            std::vector<softphone::core::ProviderConfig> found;
            const auto status =
                storage.providers()->providers(softphone::core::ProviderKind::Llm, found);

            std::vector<std::pair<QString, QString>> names;
            names.reserve(found.size());
            for (const auto &one : found) {
                names.emplace_back(QString::fromStdString(one.id),
                                   QString::fromStdString(one.name));
            }

            QMetaObject::invokeMethod(
                &app,
                [&models, &automations, &artifacts, found = std::move(found),
                 names = std::move(names), status]() mutable {

                    automations.setModels(found);
                    artifacts.setProviderNames(std::move(names));
                    models.setProviders(std::move(found),
                                        status == softphone::core::StoreStatus::Ok);
                },
                Qt::QueuedConnection);
        });

        refreshPrompts();
    };

    models.setActions(
        [&storage, &host, refreshModels, refreshTemplates](softphone::core::ProviderConfig row,
                                                           const QString &secret) {
            storage.withRecordings([&storage, &host, refreshModels, refreshTemplates,
                                    row = std::move(row),
                                    secret](softphone::core::IRecordingStore &) mutable {
                if (!softphone::core::succeeded(storage.providers()->save(row))) {
                    return;
                }
                if (!secret.isEmpty()) {
                    static_cast<void>(host.keyring().store(
                        softphone::core::secretKeys::forProvider(row.id), secret.toStdString()));
                }
                refreshModels();
                refreshTemplates();
            });
        },
        [&storage, &host, refreshModels, refreshTemplates](const QString &id) {
            storage.withRecordings([&storage, &host, refreshModels, refreshTemplates,
                                    wanted = id.toStdString()](softphone::core::IRecordingStore &) {
                static_cast<void>(storage.providers()->removeProvider(wanted));
                static_cast<void>(
                    host.keyring().erase(softphone::core::secretKeys::forProvider(wanted)));
                refreshModels();
                refreshTemplates();
            });
        },
        [llmProviderFor, &models, &app, stillHere, tokenFor](softphone::core::ProviderConfig row,
                                                             const QString &secret) {

            std::thread{[llmProviderFor, &models, &app, stillHere, tokenFor, row = std::move(row),
                         secret]() {
                softphone::core::LlmEndpoint where;
                where.url = row.endpoint;
                where.model = row.model;
                where.region = row.region;
                where.params = row.params;
                where.timeout = std::chrono::seconds{30};

                if (!stillHere->load()) {
                    return;
                }
                where.token = tokenFor(row, secret);

                softphone::core::LlmProbe answer;
                if (const auto provider = llmProviderFor(row.type)) {
                    answer = provider->probe(where);
                } else {
                    answer.reachable = false;
                    answer.status = softphone::core::LlmStatus::Unsupported;
                    answer.detail = "this build does not know that kind of model";
                }

                if (!stillHere->load()) {
                    return;
                }
                QMetaObject::invokeMethod(
                    &app, [&models, answer]() { models.setTestResult(answer); },
                    Qt::QueuedConnection);
            }}.detach();
        },
        refreshModels);

    refreshModels();

    const auto refreshTaxonomy = [&storage, &taxonomy, &automations, &templates, &app]() {
        storage.withRecordings([&storage, &taxonomy, &automations, &templates,
                                &app](softphone::core::IRecordingStore &) {
            std::vector<softphone::core::TaxonomyEntry> found;
            for (const auto kind :
                 {softphone::core::TaxonomyKind::Category, softphone::core::TaxonomyKind::Tag,
                  softphone::core::TaxonomyKind::RedFlag,
                  softphone::core::TaxonomyKind::Instruction}) {

                static_cast<void>(storage.taxonomy()->entries(kind, true, found));
            }

            const auto used = [&storage](softphone::core::TaxonomyKind kind) {
                std::vector<std::string> codes;
                static_cast<void>(storage.taxonomy()->usedCodes(kind, codes));
                QStringList out;
                out.reserve(static_cast<qsizetype>(codes.size()));
                for (const auto &code : codes) {
                    out.push_back(QString::fromStdString(code));
                }
                return out;
            };
            auto categories = used(softphone::core::TaxonomyKind::Category);
            auto tags = used(softphone::core::TaxonomyKind::Tag);
            auto flags = used(softphone::core::TaxonomyKind::RedFlag);

            const auto of = [&found](softphone::core::TaxonomyKind kind) {
                std::vector<softphone::core::TaxonomyEntry> out;
                for (const auto &entry : found) {
                    if (entry.kind == kind) {
                        out.push_back(entry);
                    }
                }
                return out;
            };
            auto forRules = std::make_tuple(of(softphone::core::TaxonomyKind::Category),
                                            of(softphone::core::TaxonomyKind::Tag),
                                            of(softphone::core::TaxonomyKind::RedFlag));

            auto forPrompts = of(softphone::core::TaxonomyKind::Instruction);

            QMetaObject::invokeMethod(
                &app,
                [&taxonomy, &automations, &templates, found = std::move(found),
                 categories = std::move(categories), tags = std::move(tags),
                 flags = std::move(flags), forRules = std::move(forRules),
                 forPrompts = std::move(forPrompts)]() mutable {
                    templates.setInstructionEntries(std::move(forPrompts));
                    taxonomy.setEntries(std::move(found), std::move(categories), std::move(tags),
                                        std::move(flags));
                    automations.setVocabulary(std::move(std::get<0>(forRules)),
                                              std::move(std::get<1>(forRules)),
                                              std::move(std::get<2>(forRules)));
                },
                Qt::QueuedConnection);
        });
    };

    taxonomy.setActions(
        [&storage, announceDictionaries](softphone::core::TaxonomyEntry row) {
            storage.withRecordings([&storage, announceDictionaries, row = std::move(row)](
                                       softphone::core::IRecordingStore &) mutable {
                static_cast<void>(storage.taxonomy()->save(row));
                announceDictionaries();
            });
        },
        [&storage, announceDictionaries](const QString &id) {
            storage.withRecordings([&storage, announceDictionaries,
                                    wanted = id.toStdString()](softphone::core::IRecordingStore &) {

                bool retiredInstead = false;
                static_cast<void>(storage.taxonomy()->removeEntry(wanted, retiredInstead));
                announceDictionaries();
            });
        },
        refreshTaxonomy,
        [&storage, announceDictionaries, refreshFacets]() {
            storage.withRecordings([&storage, announceDictionaries,
                                    refreshFacets](softphone::core::IRecordingStore &) {
                static_cast<void>(storage.taxonomy()->restoreShippedTaxonomy());
                announceDictionaries();

                refreshFacets();
            });
        });

    refreshTaxonomy();

    const auto vocabularyChanged =
        plugins.bus().subscribe<softphone::plugins::taxonomy::events::DictionariesChanged>(
            softphone::container::inlineExecutor(),
            [refreshTaxonomy](const softphone::plugins::taxonomy::events::DictionariesChanged &) {

                refreshTaxonomy();
            });
    static_cast<void>(vocabularyChanged);

    automations.setActions(
        [&storage, refreshAutomations](softphone::core::Automation row) {
            storage.withRecordings([&storage, refreshAutomations, row = std::move(row)](
                                       softphone::core::IRecordingStore &) mutable {
                static_cast<void>(storage.automations()->save(row));
                refreshAutomations();
            });
        },
        [&storage, refreshAutomations](const QString &id) {
            storage.withRecordings([&storage, refreshAutomations,
                                    wanted = id.toStdString()](softphone::core::IRecordingStore &) {
                static_cast<void>(storage.automations()->removeAutomation(wanted));
                refreshAutomations();
            });
        },
        refreshAutomations);

    const auto captureNow = [&plugins]() {
        return plugins.services().find<softphone::plugins::capture::ICaptureControl>(
            softphone::plugins::capture::kController);
    };

    struct WhyTheCaptureIsHeldOff {
        bool call{false};
    };

    const auto heldOff = std::make_shared<WhyTheCaptureIsHeldOff>();

    const auto wasCapturing = std::make_shared<bool>(false);

    const auto wasCalled = std::make_shared<std::string>();

    const auto refreshCaptureState = [&app, &settings, &updates, captureNow, refreshRecordings,
                                      wasCapturing, wasCalled, &demoCapture, &tellPrompter,
                                      &prompter, &endThePrompter]() {
        auto *const controller = captureNow();
        auto state = controller != nullptr ? controller->state()
                                           : softphone::plugins::capture::ICaptureControl::State{};

        if (demoCapture.running()) {
            state.available = true;
            state.running = true;
            state.what = demoCapture.what().toStdString();
            state.elapsed = demoCapture.elapsed();
        }

        const bool turned = state.running != *wasCapturing;
        *wasCapturing = state.running;

        const bool named = state.what != *wasCalled;
        *wasCalled = state.what;

        QMetaObject::invokeMethod(
            &app,
            [&settings, &updates, state, turned, named, refreshRecordings, &tellPrompter, &prompter,
             &endThePrompter]() {
                settings.applyCaptureState(state.available, state.watching, state.running,
                                           QString::fromStdString(state.what), state.heldOff);

                updates.capturing(state.running);

                tellPrompter();

                if (turned) {

                    if (state.running) {
                        prompter.freshConversation();
                    } else {
                        prompter.setWaiting(false);
                        endThePrompter();
                    }
                    refreshRecordings();
                }

                if (named && !turned) {
                    refreshRecordings();
                }
            },
            Qt::QueuedConnection);
    };

    settingsSink->ownedBy(std::string{softphone::ui::kCaptureGroup}, [&app, &settings, captureNow,
                                                                      refreshCaptureState](
                                                                         const std::string &text) {
        const auto applied = softphone::ui::captureFrom(text);

        if (auto *const controller = captureNow()) {
            controller->setSettings(softphone::ui::asControllerSettings(
                applied, static_cast<std::uint32_t>(QCoreApplication::applicationPid())));
        }
        refreshCaptureState();

        QMetaObject::invokeMethod(
            &app, [&settings, applied]() { settings.applyCapture(applied); }, Qt::QueuedConnection);
    });

    QObject::connect(&demoCapture, &softphone::ui::DemoCapture::changed, &app,
                     [refreshCaptureState]() { refreshCaptureState(); });

    const auto holdTheCaptureOff = [captureNow, heldOff, refreshCaptureState]() {
        auto *const capture = captureNow();
        if (capture == nullptr) {
            return;
        }
        const bool wanted = heldOff->call;
        if (capture->suppressed() == wanted) {
            return;
        }
        capture->suppress(wanted);
        refreshCaptureState();
    };

    host.backend().setSoundDeviceWantedCallback([captureNow, refreshCaptureState](bool wanted) {
        auto *const capture = captureNow();
        if (capture == nullptr) {
            return;
        }
        capture->yieldTheSoundCard(wanted);
        refreshCaptureState();
    });

    settings.setCaptureCallback([&host,
                                 &settingsController](softphone::ui::CaptureSettings chosen) {
        if (!settingsController) {
            return;
        }
        auto wanted = settingsController->desired();
        wanted.owned[std::string{softphone::ui::kCaptureGroup}] = softphone::core::OwnedSettings{
            std::string{softphone::plugins::capture::kOwner}, softphone::ui::asText(chosen)};
        static_cast<void>(settingsController->request(std::move(wanted), host.calls().calls()));
    });

    settings.setCaptureElapsed([captureNow, &demoCapture]() {
        if (demoCapture.running()) {
            return static_cast<int>(demoCapture.elapsed().count() / 1000);
        }
        auto *const controller = captureNow();
        return controller != nullptr ? static_cast<int>(controller->state().elapsed.count() / 1000)
                                     : 0;
    });

    settings.setCaptureDrawingCallback([&captureSound](softphone::core::Visualisation drawing) {
        captureSound.chooseDrawing(drawing);
        captureSound.follow(
            drawing == softphone::core::Visualisation::Off ? QString{} : QStringLiteral("capture"));
    });

    settings.setCaptureLevelSource([captureNow]() -> std::pair<unsigned, unsigned> {
        auto *const controller = captureNow();
        if (controller == nullptr) {
            return {};
        }
        const auto levels = controller->takeLevels();
        return {levels.near, levels.far};
    });

    settings.setCaptureNotACall(
        [captureNow, refreshCaptureState, refreshRecordings](const QString &recordingId) {
            auto *const controller = captureNow();
            if (controller == nullptr) {
                return false;
            }
            const auto done = controller->notACall(recordingId.toStdString());
            refreshCaptureState();

            if (done) {
                refreshRecordings();
            }
            return done;
        });

    settings.setCaptureRename([captureNow, refreshCaptureState](const QString &what) {
        auto *const controller = captureNow();
        if (controller == nullptr) {
            return false;
        }
        const auto done = controller->rename(what.toStdString());
        refreshCaptureState();
        return done;
    });

    settings.setCaptureCommand([captureNow, refreshCaptureState](bool start, QString what,
                                                                 QString process) {
        auto *const controller = captureNow();
        if (controller == nullptr) {
            return false;
        }
        bool ok = false;
        if (start) {
            ok = controller->start(what.trimmed().toStdString(), process.trimmed().toStdString()) ==
                 softphone::capture::CaptureError::None;
        } else {
            controller->stop();
            ok = true;
        }
        refreshCaptureState();
        return ok;
    });

    wireTheCapture = [&settings, &app, &rules, &plugins, &telemetry, &configurationNow, captureNow,
                      refreshCaptureState, refreshRecordings, holdTheCaptureOff]() {
        const auto &owned = configurationNow().owned;
        const auto held = owned.find(std::string{softphone::ui::kCaptureGroup});
        const auto stored =
            softphone::ui::captureFrom(held == owned.end() ? std::string{} : held->second.value);
        if (auto *const controller = captureNow()) {
            controller->setSettings(softphone::ui::asControllerSettings(
                stored, static_cast<std::uint32_t>(QCoreApplication::applicationPid())));
            controller->setListener(refreshCaptureState);

            controller->setWritten([&rules, &app, &plugins, &telemetry,
                                    refreshRecordings](const softphone::core::Recording &written) {

                telemetry.countCapture();

                rules.advance(written.id);
                plugins.bus().publish(softphone::ui::events::RecordingFinished{written.id},
                                      "application");

                QMetaObject::invokeMethod(&app, refreshRecordings, Qt::QueuedConnection);
            });

            controller->setMeetingPhrase([] {
                return QCoreApplication::translate("capture", "Meeting in %1").toStdString();
            });

            controller->setFound([&settings, &app](std::string what, std::string process) {
                QMetaObject::invokeMethod(
                    &app,
                    [&settings, what = QString::fromStdString(what),
                     process = QString::fromStdString(process)]() {
                        settings.sayFound(what, process);
                    },
                    Qt::QueuedConnection);
            });
            controller->setLost([&settings, &app]() {
                QMetaObject::invokeMethod(
                    &app, [&settings]() { settings.sayLost(); }, Qt::QueuedConnection);
            });

            controller->setExcluded(
                [&settings,
                 &app](std::vector<softphone::plugins::capture::ApplicationPolicy> policies) {
                    QMetaObject::invokeMethod(
                        &app,
                        [&settings, policies = std::move(policies)]() mutable {
                            settings.applyCapturePolicies(std::move(policies));
                        },
                        Qt::QueuedConnection);
                });
        }
        settings.applyCapture(stored);
        refreshCaptureState();

        holdTheCaptureOff();
    };

    wireTheCapture();

    static_cast<void>(softphone::ui::platform::watchForLeaving(
        [captureNow, refreshCaptureState](softphone::ui::platform::Away what) {
            auto *const watching = captureNow();
            if (watching == nullptr) {
                return;
            }

            const auto answer = softphone::ui::answerTo(what);
            watching->watchWith(answer.patience);
            if (answer.closeTheFile) {
                softphone::core::Logger::instance().log(
                    softphone::core::LogLevel::Info, "capture",
                    "this session is going away; closing any capture while it can still "
                    "be closed honestly");
                watching->stop();
                refreshCaptureState();
            }
        }));

    settingsSink->ownedBy(std::string{softphone::ui::kPipelineGroup},
                          [&settings, &app, &rules, refreshSpend](const std::string &text) {
                              const auto applied = softphone::ui::pipelineFrom(text);
                              softphone::core::PipelineEngine::Settings wanted;
                              wanted.automatic = applied.automatic;
                              wanted.monthlyCeilingMicros = applied.monthlyCeilingMicros;
                              wanted.monthlyCeilingTokens = applied.monthlyCeilingTokens;
                              rules.setSettings(wanted);
                              refreshSpend();

                              QMetaObject::invokeMethod(
                                  &app, [&settings, applied]() { settings.applyPipeline(applied); },
                                  Qt::QueuedConnection);
                          });

    softphone::http::IxWebSocketFactory sockets;

    softphone::http::Nghttp2GrpcTransport grpc;

    const auto streamingAsrFor =
        [&sockets,
         &grpc](const std::string &type) -> std::unique_ptr<softphone::core::IStreamingAsr> {
        if (type == "yandex") {
            return softphone::http::makeYandexStreaming(grpc);
        }
        return softphone::http::makeStreamingAsr(type, sockets);
    };

    std::unique_ptr<softphone::plugins::realtime::Session> prompting;
    std::string promptingId;

    auto answered = std::make_shared<std::vector<softphone::core::PromptedAnswer>>();

    const auto rehearsal =
        std::make_shared<softphone::core::RehearsalFeed>(softphone::core::kRehearsalRate);

    const auto rehearsing = std::make_shared<std::atomic<bool>>(false);

    const auto rehearsalFile = std::make_shared<std::filesystem::path>();

    auto promptingCall = std::make_shared<std::string>();

    auto promptingRecording = std::make_shared<std::string>();

    struct PromptedWith {
        softphone::core::PromptTemplate prompt;
        std::string recogniserId;
        std::string recogniserType;
        std::string recogniserModel;
    };

    auto promptingWith = std::make_shared<PromptedWith>();

    struct PromptedSoFar {

        std::vector<softphone::core::AsrTurn> replies;

        std::string transcriptId;
        std::string artifactId;

        std::size_t offset{0};
    };

    auto prompted = std::make_shared<PromptedSoFar>();

    auto conversing = std::make_shared<bool>(false);

    const auto fileThePrompter = [&storage, &prompting, promptingCall, promptingRecording,
                                  promptingWith, answered, prompted, refreshRecordings] {
        if (!prompting) {
            return;
        }

        prompting->stop();

        auto replies = prompted->replies;
        const auto thisRun = prompting->replies();
        replies.insert(replies.end(), thisRun.begin(), thisRun.end());

        const auto said = *answered;
        const auto callId = *promptingCall;
        const auto recordingId = *promptingRecording;
        const auto recogniser = *promptingWith;
        const auto prompt = promptingWith->prompt;

        if ((callId.empty() && recordingId.empty()) || (replies.empty() && said.empty())) {
            return;
        }

        if (prompted->transcriptId.empty()) {
            prompted->transcriptId = softphone::core::makeUuid();
        }
        if (prompted->artifactId.empty()) {
            prompted->artifactId = softphone::core::makeUuid();
        }

        prompted->replies = replies;

        storage.withRecordings([&storage, refreshRecordings, replies = std::move(replies), said,
                                callId, recordingId, recogniser, prompt,
                                readingId = prompted->transcriptId,
                                artifactId = prompted->artifactId](
                                   softphone::core::IRecordingStore &library) {

            std::vector<softphone::core::Recording> found;
            if (!recordingId.empty()) {
                softphone::core::Recording one;
                if (softphone::core::succeeded(library.recording(recordingId, one))) {
                    found.push_back(std::move(one));
                }
            } else {
                softphone::core::RecordingQuery query;
                query.callId = callId;
                query.limit = 1;
                static_cast<void>(library.recordings(query, found));
            }
            if (found.empty()) {
                softphone::core::Logger::instance().log(
                    softphone::core::LogLevel::Info, "realtime",
                    "nothing was recorded on this conversation, so the prompter's "
                    "transcript and answers have nowhere to be filed");
                return;
            }
            const auto &recording = found.front();

            auto reading = softphone::core::asRealtimeTranscript(
                recording.id, replies, recogniser.recogniserId, recogniser.recogniserType,
                recogniser.recogniserModel, prompt.language);

            reading.id = readingId;

            if (!reading.segments.empty() &&
                storage.transcripts()->save(reading) != softphone::core::StoreStatus::Ok) {
                softphone::core::Logger::instance().log(
                    softphone::core::LogLevel::Warning, "realtime",
                    "the prompter's transcript could not be filed");
                return;
            }

            if (auto suggestions =
                    softphone::core::asRealtimeSuggestions(recording.id, reading.id, said, prompt);
                suggestions.has_value()) {

                suggestions->id = artifactId;
                if (storage.artifacts()->save(*suggestions) != softphone::core::StoreStatus::Ok) {
                    softphone::core::Logger::instance().log(
                        softphone::core::LogLevel::Warning, "realtime",
                        "the prompter's suggestions could not be filed");
                }
            }

            softphone::core::Logger::instance().log(
                softphone::core::LogLevel::Info, "realtime",
                "filed a live transcript of " + std::to_string(reading.segments.size()) +
                    " replies and " + std::to_string(said.size()) + " answers against " +
                    recording.id);

            refreshRecordings();
        });
    };

    const auto stopPrompting = [&host, &prompting, &promptingId, fileThePrompter, rehearsing] {
        if (!prompting) {
            return;
        }

        if (rehearsing->load(std::memory_order_acquire)) {
            host.pausePlaying(true);
        }

        fileThePrompter();

        prompting.reset();
        if (!promptingId.empty()) {
            host.stopListening(promptingId);
            promptingId.clear();
        }
    };

    endThePrompter = [&host, &stopPrompting] { host.post([&stopPrompting] { stopPrompting(); }); };

    tellPrompter = [&prompter, &settings, &host, &uiRegistry, promptable, captureNow] {

        auto *const capturing = captureNow();
        const bool aCapture = capturing != nullptr && capturing->state().running;

        prompter.setReady(settings.prompterOffered(), !host.calls().calls().empty() || aCapture,
                          (*promptable)[0]);

        {
            const auto legs = host.calls().calls();
            QString who;
            if (!legs.empty()) {
                const auto &first = legs.front();
                who = QString::fromStdString(first.remoteDisplayName.empty()
                                                 ? softphone::core::sipUserPart(first.remoteUri)
                                                 : first.remoteDisplayName);
            } else if (capturing != nullptr) {
                who = QString::fromStdString(capturing->state().what);
            }
            prompter.setSubject(std::move(who));
        }

        uiRegistry.offerWindow(QStringLiteral("prompter"),
                               prompter.offered() || prompter.running());
    };

    QObject::connect(&settings, &softphone::ui::SettingsViewModel::realtimeChanged, &app,
                     [&tellPrompter] { tellPrompter(); });

    prompter.setDispatch([&storage](std::function<void()> work) {

        storage.withRecordings(
            [work = std::move(work)](softphone::core::IRecordingStore &) { work(); });
    });

    const std::function<void()> refreshRehearsalChoices = [&storage, &prompter, &app]() {
        softphone::core::RecordingQuery asked;
        asked.limit = 100;

        storage.withRecordings([&prompter, &app](softphone::core::IRecordingStore &store) {
            softphone::core::RecordingQuery query;
            query.limit = 100;
            std::vector<softphone::core::Recording> found;
            static_cast<void>(store.recordings(query, found));

            QVariantList rows;
            rows.reserve(static_cast<qsizetype>(found.size()));
            for (const auto &one : found) {
                if (one.filePath.empty() || one.duration.count() == 0) {

                    continue;
                }
                QVariantMap entry;
                entry.insert(QStringLiteral("id"), QString::fromStdString(one.id));

                const auto when = QDateTime::fromMSecsSinceEpoch(
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        one.startedAt.time_since_epoch())
                        .count());
                const auto who = one.remoteName.empty()
                                     ? softphone::ui::PrompterViewModel::tr("Unknown")
                                     : QString::fromStdString(one.remoteName);
                const auto minutes = one.duration.count() / 60000;
                const auto seconds = (one.duration.count() / 1000) % 60;
                entry.insert(QStringLiteral("label"),

                             softphone::ui::PrompterViewModel::tr("%1 · %2 · %3:%4")
                                 .arg(who)
                                 .arg(QLocale{}.toString(when, QLocale::ShortFormat))
                                 .arg(minutes)
                                 .arg(seconds, 2, 10, QLatin1Char('0')));

                entry.insert(QStringLiteral("incoming"),
                             one.direction == softphone::core::CallDirection::Incoming);
                entry.insert(QStringLiteral("external"),
                             one.source == softphone::core::RecordingSource::External);
                entry.insert(QStringLiteral("imported"),
                             one.source == softphone::core::RecordingSource::Imported);
                rows.push_back(entry);
            }

            QMetaObject::invokeMethod(
                &app,
                [&prompter, rows]() mutable { prompter.setRehearsalChoices(std::move(rows)); },
                Qt::QueuedConnection);
        });
    };

    prompter.setRehearsalActions(
        [&storage, &app, &prompter, &host, rehearsal, rehearsalFile,
         rehearsing](const QString &recordingId, const QString &fileUrl) {
            const auto wantedRow = recordingId.toStdString();
            const auto wantedFile =
                fileUrl.isEmpty()
                    ? std::filesystem::path{}
                    : softphone::core::fromUtf8(QUrl{fileUrl}.toLocalFile().toStdString());

            storage.withRecordings([&app, &prompter, &host, rehearsal, rehearsalFile, rehearsing,
                                    wantedRow,
                                    wantedFile](softphone::core::IRecordingStore &store) {

                const auto refuse = [&app, &prompter](const char *code) {
                    QMetaObject::invokeMethod(
                        &app,
                        [&prompter, code] { prompter.rehearsalRefused(QString::fromLatin1(code)); },
                        Qt::QueuedConnection);
                };

                std::filesystem::path file = wantedFile;
                QString name;
                auto sides = softphone::core::RehearsalFeed::Sides::FarOnly;

                if (!wantedRow.empty()) {
                    softphone::core::Recording row;
                    if (!softphone::core::succeeded(store.recording(wantedRow, row)) ||
                        row.filePath.empty()) {
                        refuse("rehearsal-gone");
                        return;
                    }
                    file = softphone::core::dataDirectory() / "recordings" / row.filePath;
                    name = QString::fromStdString(row.remoteName.empty() ? row.id : row.remoteName);
                    if (row.channelLayout == "near-far") {
                        sides = softphone::core::RehearsalFeed::Sides::NearAndFar;
                    }
                } else {
                    name = QString::fromStdString(softphone::core::asUtf8(file.stem()));
                }

                const auto facts = softphone::media::inspectSoundFile(file);
                if (!facts.readable || facts.duration.count() == 0) {
                    refuse("rehearsal-unreadable");
                    return;
                }

                if (softphone::media::needsConvertingToMp3(file)) {
                    std::error_code checking;
                    const auto size = std::filesystem::file_size(file, checking);
                    const auto when = std::filesystem::last_write_time(file, checking);
                    const auto stamp = std::hash<std::string>{}(softphone::core::asUtf8(file)) ^
                                       static_cast<std::size_t>(size) ^
                                       static_cast<std::size_t>(when.time_since_epoch().count());

                    const auto den = softphone::core::cacheDirectory() / "rehearsals";
                    std::filesystem::create_directories(den, checking);
                    const auto converted = den / (std::to_string(stamp) + ".mp3");

                    if (!std::filesystem::exists(converted, checking) &&
                        !softphone::media::convertToMp3(file, converted)) {
                        refuse("rehearsal-unreadable");
                        return;
                    }
                    file = converted;
                }

                std::vector<std::int16_t> samples;
                unsigned channels = 0;
                unsigned rate = 0;

                softphone::media::Mp3Info asMp3;
                softphone::media::WavInfo asWav;
                if (softphone::media::succeeded(
                        softphone::media::decodeMp3(file, samples, asMp3)) &&
                    asMp3.frames > 0) {
                    channels = asMp3.channels;
                    rate = asMp3.sampleRate;
                } else if (softphone::media::succeeded(
                               softphone::media::decodeWav(file, samples, asWav)) &&
                           asWav.frames > 0) {
                    channels = asWav.channels;
                    rate = asWav.sampleRate;
                } else {
                    refuse("rehearsal-unreadable");
                    return;
                }

                if (channels < 2) {
                    sides = softphone::core::RehearsalFeed::Sides::FarOnly;
                }

                rehearsal->load(samples, channels, rate, sides);
                if (!rehearsal->ready()) {
                    refuse("rehearsal-unreadable");
                    return;
                }

                softphone::media::Waveform picture;
                static_cast<void>(softphone::media::waveformFor(file, picture));
                const auto lane = [](const std::vector<std::uint8_t> &from) {
                    std::vector<float> drawn;
                    drawn.reserve(from.size());
                    for (const auto one : from) {
                        drawn.push_back(static_cast<float>(one) / 255.0F);
                    }
                    return drawn;
                };
                auto nearDrawn = lane(picture.nearSide);
                auto farDrawn = lane(picture.farSide);

                softphone::ui::raiseOffTheNoiseFloor(std::span{nearDrawn});
                softphone::ui::raiseOffTheNoiseFloor(std::span{farDrawn});

                const auto stereo = sides == softphone::core::RehearsalFeed::Sides::NearAndFar &&
                                    !farDrawn.empty() && farDrawn != nearDrawn;
                if (!stereo) {
                    farDrawn.clear();
                }

                const auto note = QString::fromLatin1(
                    sides == softphone::core::RehearsalFeed::Sides::NearAndFar ? "two-sides"
                                                                               : "one-room");

                const auto lengthMs = static_cast<qint64>(rehearsal->length().count());

                *rehearsalFile = file;
                rehearsing->store(true, std::memory_order_release);

                host.playRecording(file, false);

                QMetaObject::invokeMethod(
                    &app,
                    [&prompter, name, note, lengthMs, nearDrawn, farDrawn, stereo]() mutable {
                        prompter.rehearsalChosen(std::move(name), std::move(note), lengthMs,
                                                 std::move(nearDrawn), std::move(farDrawn), stereo);
                    },
                    Qt::QueuedConnection);
            });
        },
        [&host, rehearsal, rehearsalFile, rehearsing] {

            rehearsing->store(false, std::memory_order_release);
            rehearsalFile->clear();
            rehearsal->rewind();
            host.stopPlaying();
        },
        [&host](bool wanted) { host.pausePlaying(!wanted); },
        [&host](qint64 positionMs) {
            host.seekPlayback(static_cast<std::uint64_t>(std::max<qint64>(0, positionMs)));
        },
        refreshRehearsalChoices);

    prompter.setActions(
        [&host, &storage, &app, &prompter, &prompting, &promptingId, &stopPrompting, llmProviderFor,
         streamingAsrFor, answered, prompted, promptingCall, promptingRecording, promptingWith,
         &languageNow, captureNow, consentWanted, rehearsal, rehearsing,
         rehearsalFile](const QString &templateId) {

            const auto didNotStart = [&app, &prompter](const std::string &why) {
                softphone::core::Logger::instance().log(softphone::core::LogLevel::Warning,
                                                        "realtime",
                                                        "the prompter did not start: " + why);
                QMetaObject::invokeMethod(
                    &app, [&prompter] { prompter.setRunning(false); }, Qt::QueuedConnection);
            };

            std::vector<softphone::core::PromptTemplate> shapes;
            static_cast<void>(storage.templates()->templates(shapes));

            const auto wanted = templateId.toStdString();
            const auto found = std::find_if(
                shapes.begin(), shapes.end(),
                [&wanted](const softphone::core::PromptTemplate &one) { return one.id == wanted; });
            if (found == shapes.end()) {
                didNotStart("there is no prompt with the chosen name");
                return;
            }
            const auto prompt = *found;

            std::vector<softphone::core::ProviderConfig> recognisers;
            static_cast<void>(
                storage.providers()->providers(softphone::core::ProviderKind::Asr, recognisers));

            const auto streamable = [](const softphone::core::ProviderConfig &one) {
                return one.enabled && softphone::http::canStream(one.type);
            };

            const auto chosen = softphone::core::recogniserForPrompter(
                recognisers, prompt.asrProviderId, streamable);
            if (chosen.row == nullptr) {

                const auto why = chosen.why == softphone::core::RecogniserChoice::NamedButUnusable
                                     ? std::string{"recogniser-unavailable"}
                                     : std::string{"no-recogniser"};
                {

                    const softphone::core::LogAttribute fields[] = {{"why", why},
                                                                    {"prompt", prompt.id}};
                    softphone::core::Logger::instance().log(softphone::core::LogLevel::Warning,
                                                            "realtime",
                                                            "the prompter did not start", fields);
                }
                QMetaObject::invokeMethod(
                    &app,
                    [&prompter, why] {
                        prompter.heardTrouble(QString::fromStdString(why));
                        prompter.setRunning(false);
                    },
                    Qt::QueuedConnection);
                return;
            }
            const auto recogniser = chosen.row;

            std::vector<softphone::core::ProviderConfig> everyModel;
            static_cast<void>(
                storage.providers()->providers(softphone::core::ProviderKind::Llm, everyModel));
            auto model = std::find_if(everyModel.begin(), everyModel.end(),
                                      [&prompt](const softphone::core::ProviderConfig &one) {
                                          return one.enabled && one.id == prompt.providerId;
                                      });
            if (model == everyModel.end()) {
                model = std::find_if(everyModel.begin(), everyModel.end(),
                                     [](const softphone::core::ProviderConfig &one) {
                                         return one.enabled && one.isDefault;
                                     });
            }
            if (model == everyModel.end()) {
                model = std::find_if(
                    everyModel.begin(), everyModel.end(),
                    [](const softphone::core::ProviderConfig &one) { return one.enabled; });
            }

            const auto chosenRecogniser = *recogniser;
            const std::optional<softphone::core::ProviderConfig> chosenModel =
                model == everyModel.end() ? std::nullopt : std::make_optional(*model);

            host.post([&host, &app, &prompter, &prompting, &promptingId, &stopPrompting,
                       llmProviderFor, streamingAsrFor, answered, prompted, promptingCall,
                       promptingRecording, promptingWith, &languageNow, prompt, chosenRecogniser,
                       chosenModel, captureNow, consentWanted, didNotStart, rehearsal, rehearsing,
                       rehearsalFile] {

                stopPrompting();

                const auto callIds = [&host] {
                    std::vector<std::string> ids;
                    for (const auto &call : host.calls().calls()) {
                        ids.push_back(call.id);
                    }
                    return ids;
                }();

                auto *const capturing = captureNow();
                const auto aCapture = capturing != nullptr
                                          ? capturing->state()
                                          : softphone::plugins::capture::ICaptureControl::State{};
                const bool onACapture = callIds.empty() && aCapture.running;

                const bool onARehearsal = rehearsing->load(std::memory_order_acquire);

                if (onARehearsal && !rehearsal->ready()) {
                    didNotStart("there is nothing loaded to rehearse against");
                    return;
                }

                if (!onARehearsal && callIds.empty() && !onACapture) {
                    didNotStart("there is no conversation to listen to");
                    return;
                }

                const auto legsNow = host.calls().calls();
                const auto anybodyAnswered = std::any_of(
                    legsNow.begin(), legsNow.end(), [](const softphone::core::CallInfo &call) {
                        return call.state == softphone::core::CallState::Active;
                    });
                if (!onARehearsal && !onACapture && !anybodyAnswered) {
                    softphone::core::Logger::instance().log(
                        softphone::core::LogLevel::Info, "realtime",
                        "the prompter is waiting for the call to be answered");
                    QMetaObject::invokeMethod(
                        &app, [&prompter] { prompter.setWaiting(true); }, Qt::QueuedConnection);
                    return;
                }

                const auto durableCall = [&host, &callIds, onACapture] {
                    if (onACapture) {
                        return std::string{};
                    }
                    for (const auto &call : host.calls().calls()) {
                        if (call.id == callIds.front()) {
                            return call.uuid;
                        }
                    }
                    return std::string{};
                }();

                const auto durableRecording = onACapture ? aCapture.recordingId : std::string{};

                const auto states = host.recording().states();
                const auto announced =
                    onARehearsal || onACapture ||
                    std::any_of(states.begin(), states.end(),
                                [](const softphone::core::RecordingController::State &state) {
                                    return state.keeping;
                                });

                const auto posture = onARehearsal || onACapture
                                         ? softphone::core::ConsentPosture::Silent
                                     : states.empty() ? *consentWanted
                                                      : states.front().consent;

                if (const auto refused = softphone::core::promptingAllowed(posture, announced);
                    refused != softphone::core::PromptingRefused::Nothing) {
                    const auto why = std::string{softphone::core::toString(refused)};
                    {
                        const auto where = states.empty() ? std::string{"nothing is recording"}
                                                          : std::string{"the recorder's own"};
                        const softphone::core::LogAttribute fields[] = {
                            {"why", why},
                            {"posture", toString(posture)},
                            {"from", where},
                            {"announced", announced ? "yes" : "no"}};
                        softphone::core::Logger::instance().log(
                            softphone::core::LogLevel::Warning, "realtime",
                            "the prompter did not start", fields);
                    }
                    QMetaObject::invokeMethod(
                        &app,
                        [&prompter, why] {
                            prompter.heardTrouble(QString::fromStdString(why));
                            prompter.setRunning(false);
                        },
                        Qt::QueuedConnection);
                    return;
                }

                promptingId =
                    onACapture ? std::string{} : "realtime:" + softphone::core::makeUuid();

                if (*promptingCall != durableCall || *promptingRecording != durableRecording) {
                    *prompted = PromptedSoFar{};
                    answered->clear();
                }
                *promptingCall = durableCall;
                *promptingRecording = durableRecording;

                *promptingWith = PromptedWith{prompt, chosenRecogniser.id, chosenRecogniser.type,
                                              chosenRecogniser.realtimeModel};

                prompted->offset = prompted->replies.size();

                auto *const feed = onARehearsal ? nullptr
                                   : onACapture ? capturing->listenTo()
                                                : host.listenTo(promptingId, callIds);
                if (!onARehearsal && feed == nullptr) {
                    didNotStart("there is nothing to listen to on this conversation");
                    promptingId.clear();
                    return;
                }

                auto adapter = streamingAsrFor(chosenRecogniser.type);
                if (!adapter) {
                    didNotStart("this build cannot open a live session of type " +
                                chosenRecogniser.type);
                    host.stopListening(promptingId);
                    promptingId.clear();
                    return;
                }
                const auto can = adapter->capabilities({});

                softphone::plugins::realtime::Session::Opening opening;
                opening.prompt = prompt;

                opening.recogniser.url = [&chosenRecogniser] {
                    if (!chosenRecogniser.realtimeEndpoint.empty()) {
                        return chosenRecogniser.realtimeEndpoint;
                    }
                    const auto published =
                        std::string{softphone::core::liveAddressFor(chosenRecogniser.type)};
                    if (!published.empty()) {
                        return published;
                    }
                    return softphone::core::runsOnThisMachine(chosenRecogniser.type)
                               ? chosenRecogniser.endpoint
                               : std::string{};
                }();

                opening.recogniser.model = chosenRecogniser.realtimeModel;
                opening.recogniser.region = chosenRecogniser.region;
                opening.recogniser.params = chosenRecogniser.params;
                if (chosenRecogniser.hasSecret) {
                    static_cast<void>(host.keyring().fetch(
                        softphone::core::secretKeys::forProvider(chosenRecogniser.id),
                        opening.recogniser.token));
                }

                if (onARehearsal) {
                    rehearsal->rewind();
                    opening.farRing = rehearsal->farListening();
                    opening.nearRing = prompt.realtimeNear ? rehearsal->nearListening() : nullptr;
                    opening.sourceRate = rehearsal->sampleRate();
                } else {
                    opening.farRing = feed->farListening();
                    opening.nearRing = prompt.realtimeNear ? feed->nearListening() : nullptr;
                    opening.sourceRate = feed->sampleRate();
                }

                opening.turns.policy = prompt.turnPolicy;
                opening.turns.providerMarksTurns = can.marksTurns;

                opening.turns.providerTakesOurBoundary = can.takesOurTurnMark;
                opening.turns.asksAModel = softphone::core::wantsAModel(prompt);

                {
                    const auto knobs = softphone::core::knobsFor(softphone::core::ProviderKind::Asr,
                                                                 chosenRecogniser.type);
                    const auto millis = [&knobs, &chosenRecogniser](
                                            const char *name, std::chrono::milliseconds otherwise) {
                        const auto said =
                            softphone::core::knobValue(chosenRecogniser.params, knobs, name);
                        if (said.empty()) {
                            return std::chrono::milliseconds::zero();
                        }
                        try {
                            return std::chrono::milliseconds{std::max(0, std::stoi(said))};
                        } catch (const std::exception &) {

                            return otherwise;
                        }
                    };
                    opening.turns.providerIdle =
                        millis("turn_idle_ms", softphone::core::kProviderIdle);
                    opening.turns.join = millis("turn_join_ms", softphone::core::kJoinWithin);
                }

                opening.turns.speakerLanguage = softphone::ui::contentLanguage(languageNow);

                opening.language = chosenRecogniser.language;
                opening.into = prompt.languagePolicy == softphone::core::LanguagePolicy::Fixed
                                   ? prompt.language
                                   : std::string{};

                opening.interface = softphone::ui::contentLanguage(languageNow);

                opening.since =
                    onACapture ? aCapture.elapsed : host.recording().elapsed(callIds.front());

                opening.purse.recogniserMicrosPerMinute = chosenRecogniser.realtimePriceMicros;

                softphone::plugins::realtime::Session::Ask ask;
                if (chosenModel.has_value()) {
                    auto provider = llmProviderFor(chosenModel->type);
                    if (provider) {
                        softphone::core::LlmEndpoint where;
                        where.url = chosenModel->endpoint;
                        where.model = chosenModel->model;
                        where.region = chosenModel->region;
                        where.params = chosenModel->params;
                        if (chosenModel->hasSecret) {
                            static_cast<void>(host.keyring().fetch(
                                softphone::core::secretKeys::forProvider(chosenModel->id),
                                where.token));
                        }

                        where.timeout = std::chrono::seconds{20};

                        ask = [provider, where, inPrice = chosenModel->inPricePerMillion,
                               outPrice = chosenModel->outPricePerMillion](
                                  const softphone::core::RenderedPrompt &rendered,
                                  const softphone::core::LlmCancel &cancelled) {
                            softphone::core::LlmOptions options;
                            const auto answer =
                                provider->ask(where, rendered.messages, options, cancelled);

                            if (softphone::core::Logger::instance().level() >=
                                softphone::core::LogLevel::Trace) {
                                softphone::core::Logger::instance().log(
                                    softphone::core::LogLevel::Trace, "realtime",
                                    "model → " + answer.request);
                                softphone::core::Logger::instance().log(
                                    softphone::core::LogLevel::Trace, "realtime",
                                    "model ← " + answer.raw);
                            }

                            softphone::plugins::realtime::Session::Answered came;
                            came.worked = answer.status == softphone::core::LlmStatus::Ok;
                            came.text = came.worked ? answer.text : answer.detail;
                            came.costMicros =
                                softphone::core::costMicros(answer.usage, inPrice, outPrice);
                            return came;
                        };
                    }
                }

                softphone::plugins::realtime::Session::Listener listener;
                listener.listening = [&app, &prompter] {
                    QMetaObject::invokeMethod(
                        &app, [&prompter] { prompter.setRunning(true); }, Qt::QueuedConnection);
                };
                listener.partial = [&app, &prompter](softphone::core::Voice voice,
                                                     std::string text) {
                    QMetaObject::invokeMethod(
                        &app,
                        [&prompter, near = voice == softphone::core::Voice::Near,
                         text = std::move(text)]() mutable {
                            prompter.heardPartial(near, QString::fromStdString(text));
                        },
                        Qt::QueuedConnection);
                };
                listener.turn = [&app, &prompter](softphone::core::AsrTurn said) {
                    QMetaObject::invokeMethod(
                        &app,
                        [&prompter, said = std::move(said)] {
                            prompter.heardTurn(said.voice == softphone::core::Voice::Near,
                                               QString::fromStdString(said.text), said.start);
                        },
                        Qt::QueuedConnection);
                };
                listener.answered = [&app, &prompter, answered, where = prompted->offset](
                                        std::string answer, std::size_t from, std::size_t to,
                                        std::chrono::milliseconds at) {

                    answered->push_back({answer, from + where, to + where, at});
                    QMetaObject::invokeMethod(
                        &app,
                        [&prompter, answer = std::move(answer), covers = to - from + 1,
                         at]() mutable {
                            prompter.heardAnswer(QString::fromStdString(answer),
                                                 static_cast<int>(covers), at);
                        },
                        Qt::QueuedConnection);
                };
                listener.gap = [&app, &prompter](std::chrono::milliseconds,
                                                 std::chrono::milliseconds lost) {
                    QMetaObject::invokeMethod(
                        &app, [&prompter, lost] { prompter.heardGap(lost); }, Qt::QueuedConnection);
                };
                listener.spent = [&app, &prompter](std::int64_t recogniserMicros,
                                                   std::int64_t modelMicros, bool estimated) {
                    QMetaObject::invokeMethod(
                        &app,
                        [&prompter, recogniserMicros, modelMicros, estimated] {
                            prompter.heardSpent(recogniserMicros, modelMicros, estimated);
                        },
                        Qt::QueuedConnection);
                };
                listener.trouble = [&app, &prompter](std::string what, std::string said) {
                    QMetaObject::invokeMethod(
                        &app,
                        [&prompter, what = std::move(what), said = std::move(said)]() mutable {
                            prompter.heardTrouble(QString::fromStdString(what),
                                                  QString::fromStdString(said));
                        },
                        Qt::QueuedConnection);
                };
                listener.finished = [&app, &prompter](softphone::core::AsrResult) {
                    QMetaObject::invokeMethod(
                        &app, [&prompter] { prompter.setRunning(false); }, Qt::QueuedConnection);
                };

                prompting = std::make_unique<softphone::plugins::realtime::Session>(
                    std::move(opening), std::move(adapter), std::move(ask), std::move(listener));

                if (onARehearsal) {
                    host.pausePlaying(false);
                }
            });
        },
        [&host, &app, &prompter, &stopPrompting] {

            QMetaObject::invokeMethod(
                &app, [&prompter] { prompter.setWaiting(false); }, Qt::QueuedConnection);
            host.post([&stopPrompting] { stopPrompting(); });
        },
        [&host, &prompting] {
            host.post([&prompting] {
                if (prompting) {
                    prompting->askNow();
                }
            });
        },
        [&host, &prompting, answered, prompted] {
            host.post([&prompting, answered, prompted] {
                if (prompting) {
                    prompting->forget();
                }

                *prompted = PromptedSoFar{};
                answered->clear();
            });
        },
        [](const QString &where, const QString &text) {

            const auto chosen = QUrl{where}.isLocalFile() ? QUrl{where}.toLocalFile() : where;
            const auto file = softphone::core::fromUtf8(chosen.toStdString());

            std::ofstream out{file, std::ios::binary | std::ios::trunc};
            if (!out) {
                softphone::core::Logger::instance().log(
                    softphone::core::LogLevel::Warning, "realtime",
                    "the prompter could not be written to " + softphone::core::asUtf8(file));
                return;
            }
            const auto bytes = text.toUtf8();
            out.write(bytes.constData(), bytes.size());
        });

    settingsSink->ownedBy(std::string{softphone::ui::kRealtimeGroup},
                          [&settings, &app, &tellPrompter](const std::string &text) {
                              const auto applied = softphone::ui::realtimeFrom(text);
                              QMetaObject::invokeMethod(
                                  &app,
                                  [&settings, &tellPrompter, applied]() {
                                      settings.applyRealtime(applied);
                                      tellPrompter();
                                  },
                                  Qt::QueuedConnection);
                          });

    settings.setRealtimeCallback([&host,
                                  &settingsController](softphone::ui::RealtimeSettings chosen) {
        if (!settingsController) {
            return;
        }
        auto wanted = settingsController->desired();

        wanted.owned[std::string{softphone::ui::kRealtimeGroup}] = softphone::core::OwnedSettings{
            std::string{softphone::plugins::realtime::kOwner}, softphone::ui::asText(chosen)};
        static_cast<void>(settingsController->request(std::move(wanted), host.calls().calls()));
    });

    settings.setPipelineCallback([&host,
                                  &settingsController](softphone::ui::PipelineSettings chosen) {
        if (!settingsController) {
            return;
        }
        auto wanted = settingsController->desired();

        wanted.owned[std::string{softphone::ui::kPipelineGroup}] = softphone::core::OwnedSettings{
            softphone::core::defaultConfigLayout().processing, softphone::ui::asText(chosen)};
        static_cast<void>(settingsController->request(std::move(wanted), host.calls().calls()));
    });

    {
        const auto &owned = host.storedConfiguration().owned;
        const auto held = owned.find(std::string{softphone::ui::kPipelineGroup});
        const auto stored =
            softphone::ui::pipelineFrom(held == owned.end() ? std::string{} : held->second.value);
        softphone::core::PipelineEngine::Settings wanted;
        wanted.automatic = stored.automatic;
        wanted.monthlyCeilingMicros = stored.monthlyCeilingMicros;
        wanted.monthlyCeilingTokens = stored.monthlyCeilingTokens;
        rules.setSettings(wanted);
        settings.applyPipeline(stored);
        refreshSpend();
    }

    {
        const auto &owned = host.storedConfiguration().owned;
        const auto held = owned.find(std::string{softphone::ui::kRealtimeGroup});
        const auto stored =
            softphone::ui::realtimeFrom(held == owned.end() ? std::string{} : held->second.value);
        settings.applyRealtime(stored);
    }

    recordings.setImporter([&storage, &rules, &plugins, &app, &recordings,
                            refreshRecordings](const QList<QUrl> &files) {
        std::vector<std::filesystem::path> chosen;
        chosen.reserve(static_cast<std::size_t>(files.size()));
        for (const auto &one : files) {

            const auto local = one.isLocalFile() ? one.toLocalFile() : one.toString();
            if (!local.isEmpty()) {
                chosen.push_back(softphone::core::fromUtf8(local.toStdString()));
            }
        }
        if (chosen.empty()) {
            return;
        }

        storage.withRecordings([chosen = std::move(chosen), &rules, &plugins, &app, &recordings,
                                refreshRecordings](softphone::core::IRecordingStore &store) {
            softphone::core::RecordingLibrary shelf{&store, softphone::core::dataDirectory() /
                                                                "recordings"};

            shelf.setProber(softphone::media::inspectSoundFile);
            shelf.setConverter(softphone::media::convertToMp3);

            int brought = 0;
            int refused = 0;
            for (const auto &file : chosen) {
                const auto arrived = shelf.bring(file);
                if (!arrived.arrived()) {
                    ++refused;
                    continue;
                }
                ++brought;

                rules.advance(arrived.recording.id);
                plugins.bus().publish(
                    softphone::ui::events::RecordingFinished{arrived.recording.id}, "application");
            }

            refreshRecordings();

            QMetaObject::invokeMethod(
                &app,
                [&recordings, brought, refused]() {
                    recordings.sayWhatWasImported(brought, refused);
                },
                Qt::QueuedConnection);
        });
    });

    host.setRecordingWrittenListener([&rules,
                                      &plugins](const softphone::core::Recording &finished) {
        rules.advance(finished.id);

        plugins.bus().publish(softphone::ui::events::RecordingFinished{finished.id}, "application");
    });

    settingsSink->ownedBy(std::string{softphone::ui::kTranscriptionGroup},
                          [&settings, &app, &pipeline](const std::string &text) {
                              const auto applied = softphone::ui::transcriptionFrom(text);
                              softphone::core::TranscriptionService::Settings wanted;

                              wanted.automatic = false;
                              wanted.language = applied.language;
                              pipeline.setSettings(wanted);

                              QMetaObject::invokeMethod(
                                  &app,
                                  [&settings, applied]() { settings.applyTranscription(applied); },
                                  Qt::QueuedConnection);
                          });

    settings.setTranscriptionCallback(
        [&host, &settingsController](softphone::ui::TranscriptionSettings chosen) {
            if (!settingsController) {
                return;
            }
            auto wanted = settingsController->desired();
            wanted.owned[std::string{softphone::ui::kTranscriptionGroup}] =
                softphone::core::OwnedSettings{softphone::core::defaultConfigLayout().transcription,
                                               softphone::ui::asText(chosen)};
            static_cast<void>(settingsController->request(std::move(wanted), host.calls().calls()));
        });

    {
        softphone::core::TranscriptionService::Settings wanted;
        wanted.automatic = false;
        const auto &owned = host.storedConfiguration().owned;
        const auto held = owned.find(std::string{softphone::ui::kTranscriptionGroup});
        wanted.language = softphone::ui::transcriptionFrom(held == owned.end() ? std::string{}
                                                                               : held->second.value)
                              .language;
        pipeline.setSettings(wanted);

        if (!wanted.language.empty()) {
            if (auto *const rows = storage.providers(); rows != nullptr) {
                std::vector<softphone::core::ProviderConfig> recognisers;
                if (rows->providers(softphone::core::ProviderKind::Asr, recognisers) ==
                    softphone::core::StoreStatus::Ok) {
                    int moved = 0;
                    for (auto &row : recognisers) {
                        if (!row.language.empty()) {
                            continue;
                        }
                        row.language = wanted.language;
                        if (rows->save(row) == softphone::core::StoreStatus::Ok) {
                            ++moved;
                        }
                    }
                    const auto count = std::to_string(moved);
                    const softphone::core::LogAttribute fields[] = {{"language", wanted.language},
                                                                    {"rows", count}};
                    softphone::core::Logger::instance().log(
                        softphone::core::LogLevel::Info, "asr",
                        "the language from the transcription page is each recogniser's own now",
                        fields);
                }
            }
            QMetaObject::invokeMethod(
                &app, [&settings] { settings.setTranscriptionLanguage(QString{}); },
                Qt::QueuedConnection);
        }
    }

    host.post([&host, &storage]() {
        host.library().setStillWanted([&storage](const std::string &recordingId) {
            auto *queue = storage.jobs();
            if (queue == nullptr) {
                return false;
            }
            softphone::core::JobQuery asking;
            asking.recordingId = recordingId;
            asking.limit = 32;
            std::vector<softphone::core::Job> found;
            if (!softphone::core::succeeded(queue->jobs(asking, found))) {

                return true;
            }
            return std::any_of(found.begin(), found.end(), [](const softphone::core::Job &job) {
                return !softphone::core::finished(job.state);
            });
        });
    });

    jobs.start();

    host.setPlaybackListener([&recordings, &app, &prompter, rehearsal, rehearsalFile](
                                 softphone::plugins::softphone::EngineHost::Playback where) {
        QMetaObject::invokeMethod(
            &app,
            [&recordings, &prompter, rehearsal, rehearsalFile, where]() {
                const bool oursToRehearse = !rehearsalFile->empty() && where.file == *rehearsalFile;

                if (oursToRehearse) {
                    rehearsal->upTo(
                        std::chrono::milliseconds{static_cast<std::int64_t>(where.positionMs)});
                    prompter.rehearsalAt(static_cast<qint64>(where.positionMs), where.playing);

                    if (where.ended && prompter.running()) {
                        prompter.stop();
                    }
                    return;
                }

                recordings.setPlayback(where.playing, static_cast<qint64>(where.positionMs),
                                       static_cast<qint64>(where.durationMs), where.ended);
            },
            Qt::QueuedConnection);
    });

    const auto dialFrom = [&host](auto *asker) {
        return [&host, asker](const QString &number, const QString &accountId) {
            const auto typed = number.toStdString();
            const auto id = accountId.toStdString();

            const auto refuse = [asker](softphone::ui::Dialling::Refusal reason) {
                QMetaObject::invokeMethod(
                    asker, [asker, reason]() { asker->refuseDial(reason); }, Qt::QueuedConnection);
            };

            host.post([&host, refuse, typed, id]() {

                const auto *account = host.accountById(id);
                if (account == nullptr) {
                    softphone::core::Logger::instance().log(
                        softphone::core::LogLevel::Warning, "ui",
                        "dial: the account " + id + " is no longer in the configuration");
                    refuse(softphone::ui::Dialling::Refusal::Vanished);
                    return;
                }

                if (host.calls()
                        .place(id, softphone::core::dialTarget(typed, *account), typed)
                        .empty()) {
                    softphone::core::Logger::instance().log(
                        softphone::core::LogLevel::Warning, "ui",
                        "dial: refused, this phone is already carrying as many calls as it "
                        "will");
                    refuse(softphone::ui::Dialling::Refusal::TooManyCalls);
                }
            });
        };
    };

    QObject::connect(&history, &softphone::ui::HistoryViewModel::dialRequested, &app,
                     dialFrom(&history));

    QObject::connect(
        &contacts, &softphone::ui::ContactsViewModel::contactSubmitted, &app,
        [&storage, refreshContacts](const softphone::core::Contact &edited) {
            storage.post([contact = edited, refreshContacts](softphone::core::IContactStore &store,
                                                             softphone::core::ICallLog &) mutable {

                static_cast<void>(store.save(contact));
                refreshContacts();
            });
        });

    QObject::connect(&contacts, &softphone::ui::ContactsViewModel::contactDropped, &app,
                     [&storage, refreshContacts](const QString &id) {
                         storage.post([wanted = id.toStdString(),
                                       refreshContacts](softphone::core::IContactStore &store,
                                                        softphone::core::ICallLog &) {
                             static_cast<void>(store.removeContact(wanted));
                             refreshContacts();
                         });
                     });

    QObject::connect(&contacts, &softphone::ui::ContactsViewModel::dialRequested, &app,
                     dialFrom(&contacts));

    QObject::connect(&buttons, &softphone::ui::ButtonsViewModel::dialRequested, &app,
                     dialFrom(&buttons));

    if (const auto *dials =
            softphone::core::find(host.storedConfiguration(),
                                  softphone::core::dialingAccount(host.storedConfiguration()));
        dials != nullptr) {
        calls.setAccount(*dials);
    }

    callModel = &calls;
    calls.setLines(host.storedConfiguration().accounts);

    host.setCallListener([&calls, &host, &settingsController, &settings, &historian, &notifier,
                          &app, &telemetry, refreshHistory, heldOff, holdTheCaptureOff,
                          refreshCaptureState, &tellPrompter, &prompter, conversing, answered,
                          prompted, &stopPrompting](const softphone::core::CallInfo &call) {

        historian.observe(call);

        if (call.state == softphone::core::CallState::Ended && call.everConnected()) {
            telemetry.countCall();
        }

        {
            const auto ours = host.calls().calls();
            heldOff->call =
                std::any_of(ours.begin(), ours.end(), [](const softphone::core::CallInfo &one) {
                    return one.state != softphone::core::CallState::Ended;
                });
            holdTheCaptureOff();
        }

        notifier.observe(call);

        if (call.state == softphone::core::CallState::Ended) {
            QMetaObject::invokeMethod(&app, refreshHistory, Qt::QueuedConnection);
        }

        QMetaObject::invokeMethod(
            &calls, [&calls, call]() { calls.apply(call); }, Qt::QueuedConnection);

        QMetaObject::invokeMethod(&app, tellPrompter, Qt::QueuedConnection);

        {
            const bool any = !host.calls().calls().empty();
            const bool fresh = any && !*conversing;
            *conversing = any;
            if (fresh) {

                *prompted = PromptedSoFar{};
                answered->clear();
                QMetaObject::invokeMethod(
                    &app, [&prompter] { prompter.freshConversation(); }, Qt::QueuedConnection);
            }
        }

        if (host.calls().calls().empty()) {
            host.post([&stopPrompting] { stopPrompting(); });
        }

        {

            const auto onNow = host.calls().calls();
            const auto somebodyPickedUp =
                std::any_of(onNow.begin(), onNow.end(), [](const softphone::core::CallInfo &one) {
                    return one.state == softphone::core::CallState::Active;
                });

            const auto nothingLeft = onNow.empty();
            QMetaObject::invokeMethod(
                &app,
                [&prompter, somebodyPickedUp, nothingLeft] {
                    if (!prompter.waiting()) {
                        return;
                    }
                    if (nothingLeft) {
                        prompter.setWaiting(false);
                        return;
                    }
                    if (somebodyPickedUp) {
                        prompter.setWaiting(false);
                        prompter.start();
                    }
                },
                Qt::QueuedConnection);
        }

        if (settingsController) {
            settingsController->callsChanged(host.calls().calls());
        }

        const auto allowed = softphone::core::testSoundAllowed(host.calls().calls());
        QMetaObject::invokeMethod(
            &app, [&settings, allowed]() { settings.setCanTest(allowed); }, Qt::QueuedConnection);
    });

    const auto followDialAccount = [&accounts, &calls]() {
        for (const auto &entry : accounts.accounts()) {
            const auto row = entry.toMap();
            if (row.value(QStringLiteral("id")).toString() != accounts.dialAccount()) {
                continue;
            }

            calls.setAccount(accounts.accountRecord(accounts.dialAccount()));
            return;
        }
    };
    QObject::connect(&accounts, &softphone::ui::AccountsViewModel::accountsChanged, &app,
                     followDialAccount);

    QObject::connect(
        &accounts, &softphone::ui::AccountsViewModel::accountsChanged, &app, [&accounts, &host]() {
            std::map<std::string, bool> registered;
            for (const auto &entry : accounts.accounts()) {
                const auto id = entry.toMap().value(QStringLiteral("id")).toString();
                registered.emplace(id.toStdString(),
                                   accounts.stateOf(id) ==
                                       softphone::ui::AccountsViewModel::State::Registered);
            }

            host.post([&host, registered]() {
                for (const auto &[accountId, up] : registered) {
                    host.lamps().setAccountRegistered(accountId, up);
                }
            });
        });

    auto toneWish = std::make_shared<std::atomic<std::uint64_t>>(0);

    QObject::connect(&calls, &softphone::ui::CallViewModel::toneWanted, &app,
                     [&host, &settings, toneWish](softphone::ui::CallViewModel::Tone tone) {
                         const auto wanted = softphone::ui::toCoreTone(tone);

                         const auto file = softphone::ui::toneFile(wanted, settings.ringtone());
                         const auto loudness = softphone::core::toneLevel(wanted);
                         const auto loop = softphone::ui::toneLoops(wanted);

                         const auto output = softphone::core::toneOutput(wanted);

                         const auto mine = toneWish->fetch_add(1) + 1;

                         if (file.empty()) {
                             host.post([&host, toneWish, mine]() {
                                 if (toneWish->load() != mine) {
                                     return;
                                 }
                                 host.stopTone();
                             });
                             return;
                         }

                         host.post([&host, file, loop, output, loudness, toneWish, mine]() {

                             if (toneWish->load() != mine) {
                                 return;
                             }
                             host.startTone(file, loop, output, loudness);
                         });
                     });

    auto wasRecording = std::make_shared<std::set<std::string>>();

    host.recording().setListener(
        [&calls, &app, refreshRecordings,
         wasRecording](const std::vector<softphone::core::RecordingController::State> &states) {
            QMetaObject::invokeMethod(
                &app,
                [&calls, states, refreshRecordings, wasRecording]() {
                    std::set<std::string> now;
                    for (const auto &state : states) {
                        now.insert(state.recordingId);
                    }

                    const bool finished =
                        std::any_of(wasRecording->begin(), wasRecording->end(),
                                    [&](const std::string &id) { return now.count(id) == 0; });

                    const bool began = std::any_of(now.begin(), now.end(), [&](const auto &id) {
                        return wasRecording->count(id) == 0;
                    });

                    *wasRecording = std::move(now);
                    calls.setRecordingStates(states);

                    if (finished || began) {
                        refreshRecordings();
                    }
                },
                Qt::QueuedConnection);
        });

    host.setRecordingAskListener([&calls, &app](std::string callId) {
        QMetaObject::invokeMethod(
            &app, [&calls, callId]() { calls.askAboutRecording(QString::fromStdString(callId)); },
            Qt::QueuedConnection);
    });

    host.setPlaybackFailureListener([&recordings, &app](std::filesystem::path) {
        QMetaObject::invokeMethod(
            &app, [&recordings]() { recordings.reportUnreadable(recordings.openId()); },
            Qt::QueuedConnection);
    });

    host.setRecordingRefusalListener([&calls, &app](std::string callId,
                                                    softphone::core::EncoderStatus why) {
        QMetaObject::invokeMethod(
            &app,
            [&calls, callId, why]() { calls.refuseRecording(QString::fromStdString(callId), why); },
            Qt::QueuedConnection);
    });

    calls.setRecordingActions(
        [&host](const QString &callId) {

            const auto id = callId.toStdString();
            host.post([&host, id]() { host.startRecording(id); });
        },
        [&host](const QString &callId, bool paused) {
            const auto id = callId.toStdString();
            host.post([&host, id, paused]() { host.recording().setPaused(id, paused); });
        });

    host.setRedialListener([&calls, &app](const softphone::core::RedialController::State &state) {
        QMetaObject::invokeMethod(
            &app, [&calls, state]() { calls.applyRedial(state); }, Qt::QueuedConnection);
    });

    QObject::connect(&calls, &softphone::ui::CallViewModel::redialGotThrough, &app,
                     [&host, &settings]() {
                         const auto file = softphone::ui::toneFile(
                             softphone::core::CallTone::AutoAnswered, settings.ringtone());
                         if (file.empty()) {
                             return;
                         }

                         host.post([&host, file]() {
                             host.startTone(file, false, softphone::core::ToneOutput::Ringer,
                                            softphone::core::ToneLevel::Notifications);
                         });
                     });

    QObject::connect(
        &calls, &softphone::ui::CallViewModel::dtmfPressed, &app, [&host](const QString &digit) {
            const auto file =
                softphone::ui::dtmfFile(digit.isEmpty() ? '\0' : digit.at(0).toLatin1());
            if (file.empty()) {
                return;
            }

            host.post([&host, file]() {
                host.startTone(file, false, softphone::core::ToneOutput::Speakers,
                               softphone::core::ToneLevel::Notifications);
            });
        });

    auto apiDependencies = softphone::ui::ApiDependencies{
        [&storage](softphone::ui::StorageWork work) -> std::optional<std::string> {

            auto answer = std::make_shared<std::promise<std::string>>();
            auto waiting = answer->get_future();
            storage.post([answer, work](softphone::core::IContactStore &stored,
                                        softphone::core::ICallLog &log) {
                answer->set_value(work(stored, log));
            });

            if (waiting.wait_for(std::chrono::seconds{2}) != std::future_status::ready) {
                return std::nullopt;
            }
            return waiting.get();
        },
        [&host](softphone::ui::EngineWork work) -> std::optional<std::string> {
            auto answer = std::make_shared<std::promise<std::string>>();
            auto waiting = answer->get_future();
            host.post([answer, work]() { answer->set_value(work()); });

            if (waiting.wait_for(std::chrono::seconds{2}) != std::future_status::ready) {
                return std::nullopt;
            }
            return waiting.get();
        },
        [&storage](softphone::ui::TaxonomyWork work) -> std::optional<std::string> {

            auto answer = std::make_shared<std::promise<std::string>>();
            auto waiting = answer->get_future();
            storage.withRecordings([&storage, answer, work](softphone::core::IRecordingStore &) {
                answer->set_value(work(*storage.taxonomy()));
            });

            if (waiting.wait_for(std::chrono::seconds{2}) != std::future_status::ready) {
                return std::nullopt;
            }
            return waiting.get();
        },
        [&host]() -> softphone::core::CallController & { return host.calls(); },
        [&host]() -> const softphone::core::StoredConfiguration & {
            return host.storedConfiguration();
        },
        [&host](const std::string &accountId) {
            return std::string{
                softphone::core::toString(host.engine().registrationState(accountId))};
        },
        [&settingsController]() -> softphone::core::SettingsController * {
            return settingsController ? &*settingsController : nullptr;
        },
        [&app, refreshContacts]() {
            QMetaObject::invokeMethod(&app, refreshContacts, Qt::QueuedConnection);
        },
        [&app, refreshHistory]() {
            QMetaObject::invokeMethod(&app, refreshHistory, Qt::QueuedConnection);
        }};

    registerTheApi = [&plugins, apiDependencies]() {
        if (auto *const routes = plugins.services().find<softphone::core::ApiRouter>(
                softphone::plugins::integration::kRouteRegistry);
            routes != nullptr) {
            softphone::ui::registerApiHandlers(*routes, apiDependencies);
        }
    };

    registerTheApi();

    tellThePages = [&plugins, &languageNow]() {

        if (auto *const pages = plugins.services().find<softphone::plugins::webui::IPageLanguage>(
                softphone::plugins::webui::kPageLanguage);
            pages != nullptr) {
            pages->speak(languageNow.toStdString());
        }
    };

    armTheModules = [&plugins, &pipeline, &generation, &host, &tellThePages, &wireTheCapture]() {
        pipeline.arm(plugins.services().find<softphone::core::ITranscriptStore>(
                         softphone::plugins::transcription::kTranscriptStore) != nullptr);
        generation.arm(plugins.services().find<softphone::core::IArtifactStore>(
                           softphone::plugins::processing::kArtifactStore) != nullptr);

        const auto *keys = plugins.plugins().find("blf");
        host.armLamps(keys != nullptr && keys->plugin != nullptr &&
                      keys->state == softphone::container::LoadState::Ready);

        tellThePages();

        if (wireTheCapture) {
            wireTheCapture();
        }
    };

    armTheModules();

    softphone::core::metrics::registerEverything();

    softphone::ui::DiagnosticsViewModel diagnostics{
        [&host](softphone::core::ISipTraceSink *sink) { host.setTraceSink(sink); },
        [&host, &app](std::function<void(std::vector<softphone::core::CallMedia>)> report) {
            host.pollMedia(
                [&app, report = std::move(report)](std::vector<softphone::core::CallMedia> media) {

                    QMetaObject::invokeMethod(
                        &app,
                        [report, media = std::move(media)]() mutable { report(std::move(media)); },
                        Qt::QueuedConnection);
                });
        }};

    softphone::ui::TrayViewModel trayModel{accounts, calls, history};

    settings.applyAppearance(softphone::core::appearanceFrom(
        softphone::ui::groupText(host.storedConfiguration(), "appearance")));
    settings.applyStartup(softphone::core::startupFrom(
        softphone::ui::groupText(host.storedConfiguration(), "startup")));
    settings.applyHotkeys(softphone::core::hotkeysFrom(
        softphone::ui::groupText(host.storedConfiguration(), "hotkeys")));
    settings.applyDiagnostics(softphone::core::diagnosticsFrom(
        softphone::ui::groupText(host.storedConfiguration(), "diagnostics")));
    settings.applyWebhooks(softphone::core::webhooksFrom(
        softphone::ui::groupText(host.storedConfiguration(), "webhooks")));
    settings.applyRecording(softphone::core::recordingFrom(
        softphone::ui::groupText(host.storedConfiguration(), "recording")));
    {
        const auto &owned = host.storedConfiguration().owned;
        const auto held = owned.find(std::string{softphone::ui::kTranscriptionGroup});
        settings.applyTranscription(softphone::ui::transcriptionFrom(
            held == owned.end() ? std::string{} : held->second.value));
    }
    settings.applyApi(
        softphone::core::apiFrom(softphone::ui::groupText(host.storedConfiguration(), "api")));
    buttons.applyButtons(softphone::core::buttonsFrom(
                             softphone::ui::groupText(host.storedConfiguration(), "buttons"))
                             .buttons);
    settings.applyCallSettings(host.storedConfiguration().settings.calls.redialIntervalSeconds,
                               host.storedConfiguration().settings.calls.redialGiveUpMinutes,
                               host.storedConfiguration().settings.calls.historyDays,
                               host.storedConfiguration().settings.calls.callWaiting);

    softphone::ui::Screenshots shots{softphone::ui::screenshotsWanted(arguments),
                                     softphone::ui::screenshotsInto(arguments),
                                     softphone::ui::screenshotsCapturing(arguments),
                                     softphone::ui::screenshotsContent(arguments)};

    softphone::ui::DemoCalls demo{
        host.calls(),
        softphone::core::enabledAccounts(host.storedConfiguration()).empty()
            ? std::string{}
            : softphone::core::enabledAccounts(host.storedConfiguration()).front().id,
        [&host](std::function<void()> work) { host.post(std::move(work)); }};

    engine->rootContext()->setContextProperty(QStringLiteral("startsHidden"), startsHidden);
    engine->rootContext()->setContextProperty(QStringLiteral("shots"), &shots);
    engine->rootContext()->setContextProperty(QStringLiteral("demo"), &demo);
    engine->rootContext()->setContextProperty(QStringLiteral("demoCapture"), &demoCapture);

    softphone::ui::DemoPrompter demoPrompter{prompter};
    engine->rootContext()->setContextProperty(QStringLiteral("demoPrompter"), &demoPrompter);

    engine->rootContext()->setContextProperty(QStringLiteral("instance"), &instance);
    engine->rootContext()->setContextProperty(QStringLiteral("trayModel"), &trayModel);
    engine->rootContext()->setContextProperty(QStringLiteral("accounts"), &accounts);

    softphone::ui::WindowMemory windows{softphone::ui::shapeKey(runningLayout)};
    engine->rootContext()->setContextProperty(QStringLiteral("windows"), &windows);

    softphone::ui::PhoneShape shape{runningLayout};
    engine->rootContext()->setContextProperty(QStringLiteral("shape"), &shape);

    softphone::ui::Desktop desktop;
    engine->rootContext()->setContextProperty(QStringLiteral("desktop"), &desktop);
    engine->rootContext()->setContextProperty(QStringLiteral("calls"), &calls);
    engine->rootContext()->setContextProperty(QStringLiteral("sound"), &sound);
    engine->rootContext()->setContextProperty(QStringLiteral("captureSound"), &captureSound);
    engine->rootContext()->setContextProperty(QStringLiteral("contacts"), &contacts);
    engine->rootContext()->setContextProperty(QStringLiteral("history"), &history);
    engine->rootContext()->setContextProperty(QStringLiteral("recordings"), &recordings);
    engine->rootContext()->setContextProperty(QStringLiteral("transcript"), &transcript);
    engine->rootContext()->setContextProperty(QStringLiteral("providers"), &providers);
    engine->rootContext()->setContextProperty(QStringLiteral("taxonomy"), &taxonomy);
    engine->rootContext()->setContextProperty(QStringLiteral("models"), &models);
    engine->rootContext()->setContextProperty(QStringLiteral("artifacts"), &artifacts);
    engine->rootContext()->setContextProperty(QStringLiteral("templates"), &templates);
    engine->rootContext()->setContextProperty(QStringLiteral("automations"), &automations);
    engine->rootContext()->setContextProperty(QStringLiteral("jobs"), &jobsModel);
    engine->rootContext()->setContextProperty(QStringLiteral("buttons"), &buttons);
    engine->rootContext()->setContextProperty(QStringLiteral("storage"), &storageModel);
    engine->rootContext()->setContextProperty(QStringLiteral("settings"), &settings);
    engine->rootContext()->setContextProperty(QStringLiteral("prompter"), &prompter);
    engine->rootContext()->setContextProperty(QStringLiteral("about"), &about);
    engine->rootContext()->setContextProperty(QStringLiteral("updates"), &updates);
    engine->rootContext()->setContextProperty(QStringLiteral("telemetry"), &telemetryModel);
    engine->rootContext()->setContextProperty(QStringLiteral("diagnostics"), &diagnostics);
    engine->rootContext()->setContextProperty(QStringLiteral("uiRegistry"), &uiRegistry);
    engine->rootContext()->setContextProperty(QStringLiteral("plugins"), &pluginsModel);

    engine->rootContext()->setContextProperty(QStringLiteral("deeds"), &progressModel);

    qmlRegisterType<softphone::ui::SignalDrawing>("Softphone", 1, 0, "SignalDrawing");

    std::optional<QQmlComponent> apology;
    QObject::connect(
        &*engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        [&app, &engine, &apology]() {
            const auto said = qtComplaints().isEmpty()
                                  ? QStringLiteral("The window could not be built.")
                                  : qtComplaints().join(QStringLiteral("\n"));

            softphone::core::Logger::instance().setSink(
                std::make_shared<softphone::core::FileLogSink>(softphone::core::defaultLogFile()));
            softphone::core::Logger::instance().log(softphone::core::LogLevel::Error, "ui",
                                                    "the window could not be built: " +
                                                        said.toStdString());

            const auto where =
                QString::fromStdString(softphone::core::asUtf8(softphone::core::defaultLogFile()));

            apology.emplace(&*engine);
            apology->setData(R"qml(
                import QtQuick
                import QtQuick.Controls

                ApplicationWindow {
                    id: apology

                    // **Every word comes in from C++ and none of it is `qsTr`
                    // here.** `lupdate` reads files, and this QML is a string
                    // literal in a C++ source — so a `qsTr` in it would be a
                    // call that can never find a translation, which is worse
                    // than English text that admits it. The four strings are
                    // translated where the tool can see them, a few lines
                    // below.
                    property string heading: ""
                    property string said: ""
                    property string hint: ""
                    property string closeText: ""

                    width: 620
                    height: 320
                    visible: true
                    title: "AI Softphone"

                    Column {
                        anchors.fill: parent
                        anchors.margins: 20
                        spacing: 12

                        Label {
                            text: apology.heading
                            font.pixelSize: 18
                            font.bold: true
                        }

                        Label {
                            width: parent.width
                            wrapMode: Text.WordWrap
                            text: apology.said
                        }

                        Label {
                            width: parent.width
                            wrapMode: Text.WordWrap
                            opacity: 0.7
                            text: apology.hint
                        }

                        Button {
                            text: apology.closeText
                            onClicked: Qt.quit()
                        }
                    }
                }
            )qml",
                             QUrl{QStringLiteral("qrc:/softphone/apology.qml")});

            if (apology->isError()) {

                QCoreApplication::exit(EXIT_FAILURE);
                return;
            }

            auto *window = apology->create();
            if (window == nullptr) {
                QCoreApplication::exit(EXIT_FAILURE);
                return;
            }

            window->setParent(&app);
            window->setProperty(
                "heading", QCoreApplication::translate("Apology", "The phone could not start."));
            window->setProperty("said", said);
            window->setProperty(
                "hint",
                QCoreApplication::translate("Apology", "The whole of it is in %1").arg(where));
            window->setProperty("closeText", QCoreApplication::translate("Apology", "Close"));
        },
        Qt::QueuedConnection);

    engine->rootContext()->setContextProperty(QStringLiteral("startup"), &startupState);

    mark("the wiring");

    engine->loadFromModule("Softphone", "Splash");
    mark("the splash");

    QQuickWindow *splash = nullptr;
    if (const auto &shown = engine->rootObjects(); !shown.isEmpty()) {
        splash = qobject_cast<QQuickWindow *>(shown.constLast());
    }
    if (splash != nullptr) {
        engine->setIncubationController(splash->incubationController());
    }

    const QPointer<QQuickWindow> splashToTakeAway{splash};

    const auto dismissTheSplash = [&splash, &mark]() {
        if (splash == nullptr) {
            return;
        }

        mark("the splash goes");
        std::exchange(splash, nullptr)->close();
    };

    TakesTheSplashAway takesTheSplashAway{splash, dismissTheSplash};
    app.installEventFilter(&takesTheSplashAway);

    std::unique_ptr<QObject> phoneWindow;

    QObject *phoneUp = nullptr;

    LeavesWhenAsked leavesWhenAsked{[&phoneUp]() {
        if (phoneUp != nullptr) {
            phoneUp->setProperty("quitting", true);
        }
    }};
    app.installEventFilter(&leavesWhenAsked);

    AnswersALink answersALink{
        [&instance](const QString &target) { instance.broughtALink(target); }};
    app.installEventFilter(&answersALink);

    softphone::ui::WheelAccelerator wheelAccelerator;
    app.installEventFilter(&wheelAccelerator);

    QTimer hurry;
    const auto windowIsUp = [&](QObject *built) {
        mark("the window built");

        phoneUp = built;

        if (auto *window = qobject_cast<QQuickWindow *>(built); window != nullptr) {

            QObject::connect(
                window, &QQuickWindow::frameSwapped, &app, [&mark]() { mark("the first frame"); },
                Qt::SingleShotConnection);
        }

        hurry.stop();

        dismissTheSplash();

        if (!linkToDial.empty()) {
            instance.broughtALink(QString::fromStdString(linkToDial));
        }
    };

    class PhoneIncubator final : public QQmlIncubator {
    public:
        explicit PhoneIncubator(std::function<void(QObject *)> ready)
            : QQmlIncubator(Asynchronous), ready_{std::move(ready)}
        {}

    protected:
        void statusChanged(Status status) override
        {
            if (status == Ready && ready_) {
                std::exchange(ready_, nullptr)(object());
            }
        }

    private:
        std::function<void(QObject *)> ready_;
    };

    QQmlComponent phone{&*engine, QStringLiteral("Softphone"), QStringLiteral("Main")};
    PhoneIncubator incubating{windowIsUp};

    if (phone.isError()) {

        for (const auto &complaint : phone.errors()) {
            softphone::core::Logger::instance().log(softphone::core::LogLevel::Error, "ui",
                                                    complaint.toString().toStdString());
        }
        return EXIT_FAILURE;
    }

    hurry.setInterval(0);
    QObject::connect(&hurry, &QTimer::timeout, &app, [&engine]() {
        if (auto *controller = engine->incubationController(); controller != nullptr) {
            controller->incubateFor(10);
        }
    });
    hurry.start();

    phone.create(incubating);

    marksReachTheLog();

    if (const auto previous = softphone::core::lastRunCrashed(); previous.has_value()) {
        auto said = std::string{"the previous run ended unexpectedly"};
        if (!previous->report.empty()) {
            said += "; what it left is in " + softphone::core::asUtf8(previous->report);
        } else {
            said += " and left nothing behind, which is what a power cut or a "
                    "killed process looks like";
        }
        if (!previous->dump.empty()) {
            said += ", with a crash dump beside it";
        }
        softphone::core::Logger::instance().log(softphone::core::LogLevel::Warning, "ui",
                                                std::move(said));
    }

    telemetryService.start();

    const int result = QGuiApplication::exec();

    telemetryService.stop();

    phoneWindow.reset(incubating.object());

    stillHere->store(false);

    softphone::core::markCleanExit();

    if (starter.joinable()) {
        starter.join();
    }

    uiRegistry.stopAnnouncing();

    phoneWindow.reset();

    delete splashToTakeAway.data();

    jobs.stop();

    host.stop();

    QThreadPool::globalInstance()->clear();

    plugins.leave();

    if (pendingRestart.valid()) {
        instance.release();

        if (!QProcess::startDetached(pendingRestart.program, pendingRestart.arguments)) {

            softphone::core::Logger::instance().log(softphone::core::LogLevel::Error, "ui",
                                                    "the new version is installed but would "
                                                    "not start; it will be there at the next "
                                                    "start");
        }
    }

    return result;
}
