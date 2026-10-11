// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/account_config.hpp>
#include <softphone/core/call_controller.hpp>
#include <softphone/core/call_historian.hpp>
#include <softphone/core/codecs.hpp>
#include <softphone/core/config_store.hpp>
#include <softphone/core/contacts.hpp>
#include <softphone/core/credentials.hpp>
#include <softphone/core/engine.hpp>
#include <softphone/core/event_loop.hpp>
#include <softphone/core/level_meter.hpp>
#include <softphone/core/logging.hpp>
#include <softphone/core/paths.hpp>
#include <softphone/core/recording_controller.hpp>
#include <softphone/core/registration.hpp>
#include <softphone/core/settings_controller.hpp>
#include <softphone/core/settings_groups.hpp>
#include <softphone/core/uuid.hpp>
#include <softphone/core/version.hpp>
#include <softphone/media/mp3.hpp>
#include <softphone/plugins/monolith/plugin.hpp>
#include <softphone/secrets/credential_store.hpp>
#include <softphone/sip/pjsip_backend.hpp>
#include <softphone/storage/sqlite_store.hpp>

#include "collections.hpp"
#include "keyboard.hpp"
#include "store_commands.hpp"

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace {

using namespace std::chrono_literals;
namespace core = softphone::core;
namespace container = softphone::container;
namespace monolith = softphone::plugins::monolith;

[[nodiscard]] std::string deviceName(const core::AudioDevice &device)
{
    if (device.systemDefault()) {
        return "(system default)";
    }
    return device.name + " [" + device.driver + "]";
}

[[nodiscard]] std::string offeredCodecs(const std::vector<core::CodecChoice> &codecs)
{
    if (codecs.empty()) {
        return "(the stack's own order)";
    }

    std::string offered;
    int silent = 0;
    for (const auto &choice : codecs) {
        if (!choice.enabled) {
            ++silent;
            continue;
        }
        offered += offered.empty() ? "" : ", ";
        offered += core::describeCodec(choice.id).name;
    }

    if (offered.empty()) {
        offered = "nothing";
    }
    if (silent > 0) {
        offered += " (" + std::to_string(silent) + " off)";
    }
    return offered;
}

[[nodiscard]] std::string selectionText(const core::DeviceSelection &selection)
{
    switch (selection.use) {
    case core::DeviceUse::SystemDefault:
        return "(system default)";
    case core::DeviceUse::Chosen:
        return selection.wanted.name + " [" + selection.wanted.driver + "]";
    case core::DeviceUse::Missing:
        return "(system default; waiting for " + selection.wanted.name + ")";
    }
    return "?";
}

void printDevices(const std::vector<core::AudioDeviceInfo> &devices)
{

    const auto offered = core::devicesWorthOffering(devices);
    std::cout << offered.size() << " sound device(s), " << devices.size() - offered.size()
              << " hidden as plumbing or as the sound server:\n";
    for (const auto &device : offered) {

        auto words = device.describedAs(core::DeviceRole::Capture);
        if (const auto &speaking = device.describedAs(core::DeviceRole::Playback);
            !speaking.empty() && speaking != words) {
            words = words.empty() ? speaking : words + " / " + speaking;
        }

        std::cout << "  " << (words.empty() ? device.name : words) << "  (" << device.name << " ["
                  << device.driver << "])"
                  << (device.canCapture() ? " in=" + std::to_string(device.inputs) : std::string{})
                  << (device.canPlay() ? " out=" + std::to_string(device.outputs) : std::string{})
                  << '\n';
    }
    std::cout << std::flush;
}

[[nodiscard]] std::string groupText(const softphone::core::StoredConfiguration &configuration,
                                    const char *name)
{
    const auto held = configuration.owned.find(name);
    return held == configuration.owned.end() ? std::string{} : held->second.value;
}

class CliSettingsSink final : public core::ISettingsSink {
public:
    CliSettingsSink(core::Engine &engine, core::CallController &calls,
                    core::IAudioDeviceBackend &audio)
        : engine_{engine}, calls_{calls}, audio_{audio}
    {

        audio_.setDeviceLostCallback([this](core::DeviceRole role) {
            std::cout << "the " << toString(role) << " device stopped working" << std::endl;
            applyAudio(chosen_);
        });
    }

    void applyAudio(const core::AudioSettings &audio) override
    {
        chosen_ = audio;

        const auto result = audio_.applyAudio(audio);

        calls_.audioDeviceChanged();

        audio_.applyVolumes(audio.volumes);

        std::cout << "  microphone " << selectionText(result.microphone) << '\n'
                  << "  speakers   " << selectionText(result.speakers) << '\n'
                  << "  ringer     " << selectionText(result.ringer)
                  << (result.ringerOnItsOwnDevice ? " [own device]" : " [with the speakers]")
                  << std::endl;
    }

    void applyRingtone(const core::RingtoneChoice &ringtone) override
    {

        chosen_.ringtone = ringtone;
        std::cout << "  ringtone   " << (ringtone.ownFile() ? ringtone.file : ringtone.bundled)
                  << std::endl;
    }

    void applyVolumes(const core::Volumes &volumes) override
    {
        chosen_.volumes = volumes;
        audio_.applyVolumes(volumes);
        std::cout << "  volumes    microphone " << volumes.microphone << "%, speakers "
                  << volumes.speakers << "%, ring " << volumes.ring << "%, other "
                  << volumes.notifications << '%' << std::endl;
    }

    void applyCallSettings(const core::CallSettings &settings) override
    {

        std::cout << "  dials on "
                  << (settings.dialAccount.empty() ? "(the first account)" : settings.dialAccount)
                  << std::endl;
    }

    void applyAccount(const core::AccountConfig &account) override
    {
        if (const auto problem = core::validate(account); problem != core::ConfigProblem::None) {
            std::cout << "  account rejected: " << toString(problem) << std::endl;
            return;
        }

        calls_.setAccountPolicy(account.id, core::callPolicyFor(account));

        if (!account.enabled) {

            static_cast<void>(engine_.stopRegistration(account.id));
            std::cout << "  account " << account.id << " switched off" << std::endl;
            return;
        }

        if (const auto problem = engine_.addAccount(account);
            problem != core::ConfigProblem::None) {
            std::cout << "  account rejected: " << toString(problem) << std::endl;
            return;
        }
        static_cast<void>(engine_.startRegistration(account.id));
        std::cout << "  account " << account.id << " re-registering, dtmf "
                  << toString(account.dtmfTransport) << std::endl;
    }

    void removeAccount(const softphone::core::AccountConfig &account) override
    {
        static_cast<void>(engine_.stopRegistration(account.id));
        static_cast<void>(engine_.removeAccount(account.id));
        calls_.forgetAccountPolicy(account.id);
        std::cout << "  account " << account.id << " forgotten" << std::endl;
    }

    void applyOwned(const std::string &group, const std::string &value) override
    {
        std::cout << "  " << group << " settings " << (value.empty() ? "cleared" : "changed")
                  << std::endl;
    }

private:
    core::Engine &engine_;
    core::CallController &calls_;
    core::IAudioDeviceBackend &audio_;

    core::AudioSettings chosen_;
};

enum ExitCode : int {
    kSuccess = 0,
    kUsage = 1,
    kAuthenticationRejected = 2,
    kForbidden = 3,
    kAccountNotFound = 4,
    kRegistrarUnreachable = 5,
    kDomainNotResolved = 6,
    kServerUnavailable = 7,
    kTimedOut = 8,
    kUnknownFailure = 9,
    kStartupFailure = 10,

    kPasswordUnavailable = 11,

    kCertificateNotTrusted = 12,

    kCallBusy = 20,
    kCallDeclined = 21,
    kCallNoAnswer = 22,
    kCallNumberNotFound = 23,
    kCallNotAllowed = 24,
    kCallMediaMismatch = 25,
    kCallFailed = 26,
    kCallUnavailable = 27,
    kCallCancelled = 28,

    kCallNoAudioDevice = 29,
};

[[nodiscard]] int exitCodeFor(core::CallEndReason reason)
{
    switch (reason) {
    case core::CallEndReason::None:
    case core::CallEndReason::LocalHangup:
    case core::CallEndReason::RemoteHangup:
    case core::CallEndReason::Transferred:
    case core::CallEndReason::Replaced:
    case core::CallEndReason::PickedUpElsewhere:

        return kSuccess;
    case core::CallEndReason::Busy:
        return kCallBusy;
    case core::CallEndReason::Declined:
        return kCallDeclined;
    case core::CallEndReason::DoNotDisturb:
    case core::CallEndReason::TooManyCalls:
    case core::CallEndReason::CallWaitingOff:

        return kCallDeclined;
    case core::CallEndReason::NoAnswer:
        return kCallNoAnswer;
    case core::CallEndReason::Unavailable:
        return kCallUnavailable;
    case core::CallEndReason::Cancelled:
        return kCallCancelled;
    case core::CallEndReason::NumberNotFound:
        return kCallNumberNotFound;
    case core::CallEndReason::NotAllowed:
        return kCallNotAllowed;
    case core::CallEndReason::MediaMismatch:
        return kCallMediaMismatch;
    case core::CallEndReason::NoAudioDevice:
        return kCallNoAudioDevice;
    case core::CallEndReason::ServerError:
    case core::CallEndReason::Unreachable:
    case core::CallEndReason::Failed:
        break;
    }
    return kCallFailed;
}

[[nodiscard]] int exitCodeFor(core::RegistrationErrorCategory category)
{
    switch (category) {
    case core::RegistrationErrorCategory::None:
        return kSuccess;
    case core::RegistrationErrorCategory::AuthenticationRejected:
        return kAuthenticationRejected;
    case core::RegistrationErrorCategory::Forbidden:
        return kForbidden;
    case core::RegistrationErrorCategory::AccountNotFound:
        return kAccountNotFound;
    case core::RegistrationErrorCategory::RegistrarUnreachable:
        return kRegistrarUnreachable;
    case core::RegistrationErrorCategory::DomainNotResolved:
        return kDomainNotResolved;
    case core::RegistrationErrorCategory::ServerUnavailable:
        return kServerUnavailable;
    case core::RegistrationErrorCategory::InvalidConfiguration:
        return kUsage;
    case core::RegistrationErrorCategory::PasswordUnavailable:
        return kPasswordUnavailable;
    case core::RegistrationErrorCategory::CertificateNotTrusted:
        return kCertificateNotTrusted;
    case core::RegistrationErrorCategory::Unknown:
        break;
    }
    return kUnknownFailure;
}

struct Arguments {
    core::AccountConfig account;
    std::string password;
    std::string accountId{"cli"};
    std::chrono::seconds timeout{30};
    int logLevel{0};
    bool watch{false};
    bool save{false};
    bool forget{false};

    bool allAccounts{false};

    std::string callTarget;

    std::string redialId;

    std::string transferTarget;
    bool autoAnswer{false};
    bool noAudio{false};
    bool listAudio{false};

    std::string recordingsIn;
};

void printUsage()
{
    std::cout << "Usage: ai-softphone-cli --user <name> --domain <host> [options]\n\n"
                 "Registers one SIP account, reports every state change, and exits.\n\n"
                 "Required:\n"
                 "  --user <name>          SIP user part\n"
                 "  --domain <host>        SIP domain\n\n"
                 "Optional:\n"
                 "  --account <id>         Stored account to use or write (default \"cli\")\n"
                 "  --all-accounts         Also bring up every other enabled stored account\n"
                 "  --save                 Store the account, and the password in the keyring\n"
                 "  --forget               Remove the stored account and password, then exit\n"
                 "  --auth-user <name>     Authentication name, when it differs from --user\n"
                 "  --registrar <host>     Registrar host, when it differs from --domain\n"
                 "  --port <number>        Registrar port (default 5060)\n"
                 "  --transport <udp|tcp|tls>  Default udp\n"
                 "  --proxy <uri>          Outbound proxy URI\n"
                 "  --expiry <seconds>     Requested registration lifetime (default 300)\n"
                 "  --timeout <seconds>    How long to wait for an outcome (default 30)\n"
                 "  --log-level <0-4>      pjsip log verbosity (default 0)\n"
                 "  --transfer-to <target> Where the t key hands the call to\n"
                 "  --recordings-in <dir>  The library the r key records into\n"
                 "  --redial <id>          Make the call in that history row again —\n"
                 "                         `history list` prints the identifiers, and\n"
                 "                         an outgoing row redials what was dialled\n"
                 "  --watch                Stay registered until interrupted\n"
                 "  --help                 This text\n"
                 "  --version              Print the version and exit\n\n"
                 "During a call, when run from a terminal: 0-9, * and # are sent\n"
                 "to the far end as DTMF, h holds and resumes, m mutes, q hangs up,\n"
                 "c joins every call into one conference and s splits it again,\n"
                 "t hands the call to --transfer-to and waits to hear whether it\n"
                 "worked — the call stays until the far end says somebody answered,\n"
                 "r starts and stops recording the conversation and prints where\n"
                 "the file went and how long it turned out to be.\n"
                 "Piped or redirected, there are no keys and nothing waits for one.\n\n"
                 "Call or no call: S prints the settings and anything waiting to\n"
                 "be applied, D lists the sound devices, M moves the microphone\n"
                 "to the next one, R moves the ringer, L turns the level meter on\n"
                 "and off, C moves the next audio format to the head of the list,\n"
                 "X switches the DTMF mechanism, E changes the account's\n"
                 "registration expiry. M, R, C, X and E report when the change can\n"
                 "take effect — at once, at the next call, or when the calls it\n"
                 "would disturb are over. L holds a sound device open for as long\n"
                 "as it is on, which is why it is a key rather than the default.\n\n"
                 "The password comes from SOFTPHONE_SIP_PASSWORD, or from the OS\n"
                 "keyring when that is unset. It is deliberately not an argument:\n"
                 "anything on a command line is visible to every process on the\n"
                 "machine through ps.\n\n"
                 "Given no --user and --domain, the stored account named by --account\n"
                 "is used, so a saved account needs only:\n"
                 "  ai-softphone-cli --account work\n\n"
                 "Exit codes: 0 registered, 1 usage, 2 credentials rejected,\n"
                 "3 forbidden, 4 unknown account, 5 registrar unreachable,\n"
                 "6 domain not resolved, 7 server unavailable, 8 timed out,\n"
                 "9 unknown failure, 10 SIP stack failed to start,\n"
                 "11 the keychain would not give the password up,\n"
                 "12 the server's TLS certificate was refused.\n"
                 "With --call: 0 the call happened, 20 busy, 21 declined,\n"
                 "22 no answer, 23 no such number, 24 not allowed,\n"
                 "27 reached but unavailable (declined, do not disturb, away),\n"
                 "28 cancelled before it was answered — a missed call, incoming.\n"
                 "25 no common audio format, 26 the call failed.\n";
}

[[nodiscard]] bool parseNumber(std::string_view text, unsigned long &out)
{
    const auto *first = text.data();
    const auto *last = text.data() + text.size();
    const auto result = std::from_chars(first, last, out);
    return result.ec == std::errc{} && result.ptr == last;
}

[[nodiscard]] std::optional<Arguments> parseArguments(const std::vector<std::string_view> &args)
{
    Arguments parsed;
    parsed.account.id = "cli";

    for (std::size_t i = 0; i < args.size(); ++i) {
        const auto name = args[i];

        const auto value = [&]() -> std::string_view {
            return i + 1 < args.size() ? args[i + 1] : std::string_view{};
        };
        const auto requireValue = [&](std::string &target) {
            target = std::string{value()};
            ++i;
            return !target.empty();
        };

        if (name == "--help" || name == "-h") {
            printUsage();
            return std::nullopt;
        }

        if (name == "--version") {
            std::cout << "ai-softphone-cli " << core::versionString() << '\n';
            return std::nullopt;
        }
        if (name == "--recordings-in") {
            if (!requireValue(parsed.recordingsIn)) {
                std::cerr << "--recordings-in needs a directory\n";
                return std::nullopt;
            }
            continue;
        }
        if (name == "--watch") {
            parsed.watch = true;
            continue;
        }
        if (name == "--save") {
            parsed.save = true;
            continue;
        }
        if (name == "--forget") {
            parsed.forget = true;
            continue;
        }
        if (name == "--answer") {
            parsed.autoAnswer = true;
            continue;
        }
        if (name == "--no-audio") {
            parsed.noAudio = true;
            continue;
        }
        if (name == "--list-audio") {
            parsed.listAudio = true;
            continue;
        }

        bool ok = true;
        if (name == "--call") {
            ok = requireValue(parsed.callTarget);
        } else if (name == "--redial") {
            ok = requireValue(parsed.redialId);
        } else if (name == "--transfer-to") {
            ok = requireValue(parsed.transferTarget);
        } else if (name == "--all-accounts") {
            parsed.allAccounts = true;
        } else if (name == "--account") {
            ok = requireValue(parsed.accountId);
        } else if (name == "--user") {
            ok = requireValue(parsed.account.username);
        } else if (name == "--auth-user") {
            ok = requireValue(parsed.account.authUser);
        } else if (name == "--domain") {
            ok = requireValue(parsed.account.domain);
        } else if (name == "--registrar") {
            ok = requireValue(parsed.account.registrar);
        } else if (name == "--proxy") {
            ok = requireValue(parsed.account.outboundProxy);
        } else if (name == "--transport") {
            std::string transport;
            ok = requireValue(transport);
            const auto known = core::parseTransport(transport);
            if (ok && known) {
                parsed.account.transport = *known;
            } else if (ok) {
                std::cerr << "Unknown transport: " << transport << " (expected udp, tcp or tls)\n";
                return std::nullopt;
            }
        } else if (name == "--port" || name == "--expiry" || name == "--timeout" ||
                   name == "--log-level") {
            std::string raw;
            ok = requireValue(raw);
            unsigned long number = 0;
            if (!ok || !parseNumber(raw, number)) {
                std::cerr << "Expected a number after " << name << "\n";
                return std::nullopt;
            }
            if (name == "--port") {
                if (number == 0 || number > 65535) {
                    std::cerr << "Port out of range: " << number << "\n";
                    return std::nullopt;
                }
                parsed.account.port = static_cast<std::uint16_t>(number);
            } else if (name == "--expiry") {
                parsed.account.expiry = std::chrono::seconds{static_cast<long>(number)};
            } else if (name == "--timeout") {
                parsed.timeout = std::chrono::seconds{static_cast<long>(number)};
            } else {
                parsed.logLevel = static_cast<int>(number);
            }
        } else {
            std::cerr << "Unknown option: " << name << "\n\n";
            printUsage();
            return std::nullopt;
        }

        if (!ok) {
            std::cerr << "Missing value after " << name << "\n";
            return std::nullopt;
        }
    }

    if (const auto *password = std::getenv("SOFTPHONE_SIP_PASSWORD"); password != nullptr) {
        parsed.password = password;
    }

    parsed.account.id = parsed.accountId;

    return parsed;
}

class Reporter final : public core::EngineListener {
public:
    void onRegistrationStateChanged(const std::string &accountId, core::RegistrationState state,
                                    const core::RegistrationError &error) override
    {
        std::cout << "[" << accountId << "] " << toString(state);

        if (error.category != core::RegistrationErrorCategory::None) {
            std::cout << " (" << toString(error.category);
            if (error.sipStatus != 0) {
                std::cout << ", SIP " << error.sipStatus;
            }
            if (!error.detail.empty()) {
                std::cout << ": " << error.detail;
            }
            std::cout << ")";
        }

        std::cout << std::endl;

        lastState = state;
        lastCategory = error.category;
    }

    core::RegistrationState lastState{core::RegistrationState::Offline};
    core::RegistrationErrorCategory lastCategory{core::RegistrationErrorCategory::None};
};

void reportCall(const core::CallInfo &call)
{
    std::cout << "[" << call.id << "] " << toString(call.state) << " " << toString(call.direction)
              << " " << call.remoteLabel();

    if (const auto number = call.remoteDetail(); !number.empty()) {
        std::cout << " <" << number << ">";
    }

    if (call.transfer.state != core::TransferState::None) {
        std::cout << " [transfer " << toString(call.transfer.state);
        if (!call.transfer.target.empty()) {
            std::cout << " to " << call.transfer.target;
        }
        if (call.transfer.sipStatus != 0) {
            std::cout << ", SIP " << call.transfer.sipStatus;
        }
        if (!call.transfer.referredBy.empty()) {
            std::cout << ", asked by " << call.transfer.referredBy;
        }
        std::cout << "]";
    }

    if (call.state == core::CallState::Ended) {
        std::cout << " (" << toString(call.end.reason);
        if (call.end.sipStatus != 0) {
            std::cout << ", SIP " << call.end.sipStatus;
        }
        if (!call.end.detail.empty()) {
            std::cout << ": " << call.end.detail;
        }
        std::cout << ")";
    } else {

        std::vector<std::string> notes;
        if (call.heldByUs) {
            notes.emplace_back("on hold");
        }
        if (call.heldByThem) {
            notes.emplace_back("held by them");
        }
        if (call.muted) {
            notes.emplace_back("muted");
        }

        if (!notes.empty()) {
            std::cout << " (";
            for (std::size_t i = 0; i < notes.size(); ++i) {
                std::cout << (i == 0 ? "" : ", ") << notes[i];
            }
            std::cout << ")";
        }
    }

    std::cout << std::endl;
}

}

int main(int argc, char *argv[])
{
    const std::vector<std::string_view> args{argv + 1, argv + argc};

    if (const auto command = softphone::cli::parseStoreCommand(args); command.wanted()) {
        softphone::storage::SqliteStore store;
        if (const auto status = store.open(softphone::storage::defaultDatabaseFile());
            status != core::StoreStatus::Ok) {
            std::cerr << "ai-softphone-cli: cannot open "
                      << softphone::storage::defaultDatabaseFile() << ": " << toString(status)
                      << " (" << store.lastError() << ")\n";
            return 2;
        }
        softphone::cli::declarePluginCollections(store);

        auto asked = command;
        core::StoredConfiguration configuration;
        container::Settings settingsFile{core::defaultSettingsFile(), core::restrictToOwner};
        settingsFile.adoptOlderDocumentsAs(std::string{core::kLegacySettingsSection});
        static_cast<void>(settingsFile.load());
        if (core::ConfigStore{monolith::settingsDocument(settingsFile)}.load(configuration) ==
            core::ConfigStoreStatus::Ok) {
            asked.retention = core::recordingFrom(groupText(configuration, "recording")).retention;
        }

        const auto recording = core::recordingFrom(groupText(configuration, "recording"));
        const auto library = recording.library.empty() ? core::recordingsDirectory()
                                                       : core::fromUtf8(recording.library);

        return softphone::cli::runStoreCommand(asked, store, store, store, library, std::cout,
                                               std::cerr);
    }

    auto arguments = parseArguments(args);
    if (!arguments.has_value()) {
        const auto asked = !args.empty() && (args.front() == "--help" || args.front() == "-h" ||
                                             args.front() == "--version");
        return args.empty() || asked ? kSuccess : kUsage;
    }

    container::Settings settingsFile{core::defaultSettingsFile(), core::restrictToOwner};
    settingsFile.adoptOlderDocumentsAs(std::string{core::kLegacySettingsSection});
    static_cast<void>(settingsFile.load());

    const core::ConfigStore configStore{monolith::settingsDocument(settingsFile)};

    const auto keyring = softphone::secrets::makeCredentialStore().store;

    core::StoredConfiguration stored;
    const auto loadStatus = configStore.load(stored);
    if (loadStatus != core::ConfigStoreStatus::Ok &&
        loadStatus != core::ConfigStoreStatus::NotFound) {
        std::cerr << "Cannot read " << configStore.filePath() << ": " << toString(loadStatus)
                  << "\n";
        return kUsage;
    }

    if (arguments->forget) {

        const auto *going = core::find(stored, arguments->accountId);
        const auto name = going != nullptr ? core::credentialName(*going) : std::string{};

        const bool removed = core::remove(stored, arguments->accountId);
        if (removed) {
            if (const auto status = configStore.save(stored);
                status != core::ConfigStoreStatus::Ok) {
                std::cerr << "Could not update " << configStore.filePath() << ": "
                          << toString(status) << "\n";
                return kUsage;
            }
        }

        const auto erased =
            name.empty() ? softphone::secrets::SecretStatus::NotFound : keyring->erase(name);
        std::cout << "account " << arguments->accountId << ": "
                  << (removed ? "configuration removed" : "no stored configuration") << ", keyring "
                  << toString(erased) << std::endl;
        return kSuccess;
    }

    if (!arguments->redialId.empty()) {
        softphone::storage::SqliteStore log;
        if (const auto status = log.open(softphone::storage::defaultDatabaseFile());
            status != core::StoreStatus::Ok) {
            std::cerr << "Cannot read the call history: " << core::toString(status) << "\n";
            return kUsage;
        }
        softphone::cli::declarePluginCollections(log);

        const auto &wanted = arguments->redialId;

        std::vector<core::CallRecord> rows;
        core::CallLogQuery query;
        query.limit = 1000;
        if (log.calls(query, rows) != core::StoreStatus::Ok) {
            std::cerr << "Cannot read the call history.\n";
            return kUsage;
        }

        const auto at =
            std::ranges::find_if(rows, [wanted](const auto &row) { return row.id == wanted; });
        if (at == rows.end()) {
            std::cerr << "No call " << wanted << " in the history.\n";
            return kUsage;
        }

        arguments->callTarget = core::redialTarget(*at);

        if (!at->accountAddress.empty()) {
            for (const auto &account : stored.accounts) {
                if (core::accountAddress(account) == at->accountAddress && account.enabled) {
                    arguments->accountId = account.id;
                    arguments->account = account;
                    break;
                }
            }
        }
    }

    if (arguments->account.username.empty() || arguments->account.domain.empty()) {
        const auto *found = core::find(stored, arguments->accountId);
        if (found == nullptr) {
            std::cerr << "No account \"" << arguments->accountId << "\" is stored, and --user or "
                      << "--domain is missing.\n\n";
            printUsage();
            return kUsage;
        }
        arguments->account = *found;
    }

    if (const auto problem = validate(arguments->account); problem != core::ConfigProblem::None) {
        std::cerr << "Invalid account configuration: " << toString(problem) << "\n\n";
        printUsage();
        return kUsage;
    }

    if (arguments->save) {
        if (arguments->password.empty()) {
            std::cerr << "--save needs the password in SOFTPHONE_SIP_PASSWORD\n";
            return kUsage;
        }

        if (arguments->account.uuid.empty()) {
            if (const auto *known = core::find(stored, arguments->account.id);
                known != nullptr && !known->uuid.empty()) {
                arguments->account.uuid = known->uuid;
            } else {
                arguments->account.uuid = core::makeUuid();
            }
        }

        core::upsert(stored, arguments->account);
        if (const auto status = configStore.save(stored); status != core::ConfigStoreStatus::Ok) {
            std::cerr << "Could not write " << configStore.filePath() << ": " << toString(status)
                      << "\n";
            return kUsage;
        }

        if (const auto status =
                keyring->store(core::credentialName(arguments->account), arguments->password);
            status != softphone::secrets::SecretStatus::Ok) {
            std::cerr << "Saved the account, but the keyring refused the password: "
                      << toString(status) << "\n";
            return kUsage;
        }

        std::cout << "saved account " << arguments->account.id << " to " << configStore.filePath()
                  << ", password in the keyring" << std::endl;
    }

    const auto level = arguments->logLevel >= 4  ? core::LogLevel::Trace
                       : arguments->logLevel > 0 ? core::LogLevel::Debug
                                                 : core::LogLevel::Info;
    core::Logger::instance().setLevel(level);
    core::Logger::instance().setSink(std::make_shared<core::FileLogSink>(core::defaultLogFile()));

    core::EventLoop loop;

    core::InMemoryCredentialProvider overrideCredentials;
    if (!arguments->password.empty()) {
        overrideCredentials.set(arguments->account.id, arguments->password);

        for (const auto &other : core::enabledAccounts(stored)) {
            overrideCredentials.set(other.id, arguments->password);
        }
    }
    const core::ICredentialProvider &credentials =
        arguments->password.empty()
            ? static_cast<const core::ICredentialProvider &>(*keyring)
            : static_cast<const core::ICredentialProvider &>(overrideCredentials);

    softphone::sip::PjsipOptions options;
    options.logLevel = arguments->logLevel;
    softphone::sip::PjsipBackend backend{loop, credentials, options};

    try {
        backend.start();
    } catch (const std::exception &error) {
        std::cerr << "Could not start the SIP stack: " << error.what() << "\n";
        return kStartupFailure;
    }

    std::cout << "SIP stack listening on port " << backend.boundPort() << std::endl;

    if (arguments->listAudio) {
        printDevices(backend.devices());
        return kSuccess;
    }

    core::Engine engine{backend, loop};

    engine.setSelfRefresh(false);

    Reporter reporter;
    engine.setListener(&reporter);

    if (const auto problem = engine.addAccount(arguments->account);
        problem != core::ConfigProblem::None) {
        std::cerr << "Rejected account: " << toString(problem) << "\n";
        return kUsage;
    }

    std::vector<std::string> watched{arguments->account.id};
    if (arguments->allAccounts) {
        for (const auto &other : core::enabledAccounts(stored)) {
            if (other.id == arguments->account.id) {
                continue;
            }
            if (const auto problem = engine.addAccount(other);
                problem != core::ConfigProblem::None) {
                std::cout << "account " << other.id << " rejected: " << toString(problem)
                          << std::endl;
                continue;
            }
            watched.push_back(other.id);
        }
        std::cout << "watching " << watched.size() << " account(s)" << std::endl;
    }

    bool callPlaced = false;
    bool callEnded = false;
    bool gaveUpRinging = false;
    auto callOutcome = core::CallEndReason::None;

    core::CallPolicy policy;

    std::optional<core::SettingsController> settings;

    softphone::storage::SqliteStore history;
    std::optional<core::CallHistorian> historian;
    if (const auto status = history.open(softphone::storage::defaultDatabaseFile());
        status == core::StoreStatus::Ok) {
        softphone::cli::declarePluginCollections(history);
        historian.emplace(history, loop);
        historian->setAddressLookup([&arguments](const std::string &accountId) {
            core::CallHistorian::AccountIdentity line;
            if (accountId == arguments->account.id) {
                line.uuid = arguments->account.uuid;
                line.address = core::accountAddress(arguments->account);
            }
            return line;
        });

        std::vector<core::Contact> people;
        if (history.contacts(people) == core::StoreStatus::Ok) {
            historian->setNameLookup(
                [book = core::ContactBook{std::move(people)}](const std::string &number) {
                    return book.nameFor(number);
                });
        }
    } else {
        std::cerr << "History unavailable: " << core::toString(status) << " ("
                  << history.lastError() << ")\n";
    }

    core::RecordingController recording{
        backend,
        [](const std::filesystem::path &file, const core::EncoderSettings &format) {
            auto encoder = std::make_unique<softphone::media::Mp3Encoder>();
            if (!core::succeeded(encoder->open(file, format))) {
                return std::unique_ptr<core::IAudioEncoder>{};
            }
            return std::unique_ptr<core::IAudioEncoder>{std::move(encoder)};
        },
        arguments->recordingsIn.empty() ? core::recordingsDirectory()
                                        : std::filesystem::path{arguments->recordingsIn},
        history.isOpen() ? &history : nullptr, &backend};

    core::CallController calls{backend, loop, policy, [&](const core::CallInfo &call) {
                                   if (historian) {
                                       historian->observe(call);
                                   }

                                   recording.callsChanged(calls.calls());

                                   reportCall(call);

                                   if (call.state == core::CallState::Ended) {
                                       callOutcome = call.end.reason;
                                       callEnded = true;

                                       loop.post([&]() {
                                           if (settings) {
                                               settings->callsChanged(calls.calls());
                                           }
                                       });
                                   } else if (call.state == core::CallState::RingingIn &&
                                              arguments->autoAnswer) {

                                       const auto id = call.id;
                                       loop.post([&calls, id]() { calls.answer(id); });
                                   }
                               }};
    backend.setCallObserver(&calls);

    CliSettingsSink settingsSink{engine, calls, backend};

    core::StoredConfiguration running = stored;
    core::upsert(running, arguments->account);

    for (const auto &account : running.accounts) {
        calls.setAccountPolicy(account.id, core::callPolicyFor(account));
    }

    {
        const auto recordingNow = core::recordingFrom(groupText(running, "recording"));
        std::map<std::string, core::RecordingController::AccountRecording> policies;
        for (const auto &account : running.accounts) {
            core::RecordingController::AccountRecording line;

            switch (core::recordingPolicyFor(recordingNow, account.id)) {
            case core::RecordingPolicy::Always:
                line.when = core::RecordingController::WhenToRecord::Always;
                break;
            case core::RecordingPolicy::Ask:

                line.when = core::RecordingController::WhenToRecord::Never;
                break;
            case core::RecordingPolicy::Never:
            case core::RecordingPolicy::OnTheseAccounts:
                line.when = core::RecordingController::WhenToRecord::Never;
                break;
            }

            line.consent.posture = core::consentFor(recordingNow, account);
            line.consent.announcement = recordingNow.announcement;
            line.consent.interval = std::chrono::seconds{recordingNow.toneIntervalSeconds};
            line.consent.keepInTheRecording = recordingNow.keepNotices;

            policies.emplace(account.id, std::move(line));
        }
        recording.setAccountPolicies(std::move(policies));
    }

    settings.emplace(settingsSink, running);

    bool timedOut = false;
    bool settled = false;

    auto settledState = core::RegistrationState::Offline;
    auto settledCategory = core::RegistrationErrorCategory::None;

    const auto finish = [&]() {
        if (settled) {
            return;
        }
        settled = true;

        loop.post([&]() {
            engine.stopAll();
            static_cast<void>(loop.scheduleAfter(2s, [&loop]() { loop.stop(); }));
        });
    };

    static_cast<void>(loop.scheduleAfter(arguments->timeout, [&]() {
        if (settled) {
            return;
        }

        if (const auto call = calls.current(); call.has_value()) {
            if (call->state == core::CallState::Active) {

                std::cout << "talked for " << arguments->timeout.count() << "s; hanging up"
                          << std::endl;
                calls.hangUp(call->id);
                return;
            }

            std::cout << "no answer after " << arguments->timeout.count() << "s; giving up"
                      << std::endl;
            gaveUpRinging = true;
            calls.hangUp(call->id);
            return;
        }

        timedOut = true;
        finish();
    }));

    const auto watchdog = [&]() {
        if (settled) {
            return;
        }

        const auto state = engine.registrationState(arguments->account.id);

        if (watched.size() > 1 && !callPlaced) {
            bool allSettled = true;
            auto worstState = core::RegistrationState::Registered;
            auto worstCategory = core::RegistrationErrorCategory::None;

            for (const auto &id : watched) {
                const auto each = engine.registrationState(id);
                const bool done =
                    each == core::RegistrationState::Registered ||
                    (each == core::RegistrationState::Failed && !engine.retryPending(id));
                allSettled = allSettled && done;

                if (each == core::RegistrationState::Failed &&
                    worstCategory == core::RegistrationErrorCategory::None) {
                    worstState = each;
                    worstCategory = engine.lastError(id).category;
                }
            }

            if (allSettled && !arguments->watch && arguments->callTarget.empty()) {
                settledState = worstState;
                settledCategory = worstCategory;
                finish();
                return;
            }
            if (!allSettled) {
                return;
            }
        }

        if (state == core::RegistrationState::Registered && !arguments->callTarget.empty() &&
            !callPlaced) {
            callPlaced = true;
            static_cast<void>(calls.place(
                arguments->account.id, core::dialTarget(arguments->callTarget, arguments->account),
                arguments->callTarget));
            return;
        }

        if (callPlaced) {
            if (callEnded && !arguments->watch) {
                settledState = core::RegistrationState::Registered;
                finish();
            }
            return;
        }

        if (state == core::RegistrationState::Registered && !arguments->watch &&
            arguments->callTarget.empty()) {
            settledState = state;
            finish();
        } else if (state == core::RegistrationState::Failed &&
                   !engine.retryPending(arguments->account.id)) {
            settledState = state;
            settledCategory = engine.lastError(arguments->account.id).category;
            finish();
        }
    };

    for (const auto &id : watched) {
        engine.startRegistration(id);
    }

    const auto keyboard = softphone::cli::makeKeyboard();
    auto keysAnnounced = false;
    auto metering = false;
    auto nextLevelReport = std::chrono::steady_clock::now();

    const auto reportChanges = [](const std::vector<core::SettingChange> &changes) {
        for (const auto &change : changes) {
            std::cout << "  " << toString(change.area) << ": " << toString(change.schedule);
            for (const auto &blocking : change.blockedBy) {
                std::cout << ' ' << blocking;
            }
            std::cout << '\n';
        }
        std::cout << std::flush;
    };

    const auto startRecording = [&](const core::CallInfo &call) {

        if (recording.recording(call.id)) {
            return;
        }

        const auto configured = core::recordingFrom(groupText(settings->effective(), "recording"));
        recording.setKeepStems(configured.keepStems);

        core::RecordingController::ConsentPlan consent;
        consent.posture = arguments->account.id == call.accountId
                              ? core::consentFor(configured, arguments->account)
                              : configured.consent;
        consent.announcement = configured.announcement;
        consent.interval = std::chrono::seconds{configured.toneIntervalSeconds};
        consent.keepInTheRecording = configured.keepNotices;
        recording.setConsent(consent);

        if (const auto status = recording.start(call.id, calls.calls()); !core::succeeded(status)) {
            std::cout << "  cannot record into " << recording.root().string() << ": "
                      << toString(status) << std::endl;
            return;
        }

        std::cout << "  recording " << recording.subject(call.id).size() << " leg(s) to "
                  << recording.file(call.id).string() << ", consent " << toString(consent.posture)
                  << ", " << recording.count() << " in progress" << std::endl;
    };

    const auto stopRecording = [&](const std::string &callId) {
        if (!recording.recording(callId)) {
            return;
        }

        const auto file = recording.file(callId);
        const auto finished = recording.stop(callId);
        const auto &outcome = finished.outcome;

        std::cout << "  recorded " << outcome.duration.count() << " ms, " << outcome.bytes
                  << " bytes";
        if (outcome.hasGaps()) {
            std::cout << ", with gaps (" << outcome.gapSamples << " samples missing)";
        }
        if (!core::succeeded(outcome.status)) {
            std::cout << ", and it ended early: " << toString(outcome.status);
        }
        std::cout << "\n  " << file.string() << std::endl;

        for (const auto &stem : outcome.stems) {
            std::cout << "  stem " << stem.callId << ": " << stem.duration.count() << " ms from +"
                      << stem.offset.count() << " ms, " << stem.bytes << " bytes" << std::endl;
        }
    };

    const auto settingsKey = [&](char key) {
        switch (key) {
        case 'S': {
            const auto &current = settings->effective().settings;
            const auto appearance =
                core::appearanceFrom(groupText(settings->effective(), "appearance"));
            std::cout << "settings:\n"
                      << "  microphone " << deviceName(current.audio.microphone) << '\n'
                      << "  speakers   " << deviceName(current.audio.speakers) << '\n'
                      << "  ringer     " << deviceName(current.audio.ringer) << '\n'
                      << "  dials on   "
                      << (core::dialingAccount(settings->effective()).empty()
                              ? "(nothing enabled)"
                              : core::dialingAccount(settings->effective()))
                      << '\n'
                      << "  codecs     " << offeredCodecs(current.calls.codecs)
                      << '\n'

                      << "  language   "
                      << (appearance.language.empty() ? "(system)" : appearance.language) << '\n'
                      << "  theme      " << core::toString(appearance.theme) << '\n';
            for (const auto &account : settings->effective().accounts) {
                std::cout << "  account    " << account.id << " " << account.sipUri() << " expiry "
                          << account.expiry.count() << "s\n";
            }
            if (settings->pending().empty()) {
                std::cout << "  nothing waiting\n";
            } else {
                std::cout << "  waiting:\n";
                reportChanges(settings->pending());
            }
            std::cout << std::flush;
            return true;
        }
        case 'D': {
            printDevices(backend.devices());
            return true;
        }
        case 'C': {

            const auto available = backend.codecs();
            if (available.empty()) {
                std::cout << "no codecs" << std::endl;
                return true;
            }

            auto wanted = settings->desired();
            const auto planned = core::planCodecs(wanted.settings.calls.codecs, available);
            const auto &head = planned.empty() ? available.front().id : planned.front().id;

            const auto at = std::ranges::find(available, head, &core::CodecInfo::id);
            const auto next =
                at == available.end() || at + 1 == available.end() ? available.begin() : at + 1;

            std::vector<core::CodecChoice> reordered{core::CodecChoice{next->id, true}};
            wanted.settings.calls.codecs = core::planCodecs(reordered, available);

            std::cout << "codecs -> " << offeredCodecs(wanted.settings.calls.codecs) << std::endl;
            reportChanges(settings->request(wanted, calls.calls()));
            return true;
        }
        case 'M': {

            std::vector<core::AudioDevice> ring{core::AudioDevice{}};
            for (const auto &device : core::devicesWorthOffering(backend.devices())) {
                if (device.canCapture()) {
                    ring.push_back(core::AudioDevice{device.driver, device.name});
                }
            }

            auto wanted = settings->desired();
            const auto &current = wanted.settings.audio.microphone;
            const auto at = std::find(ring.begin(), ring.end(), current);
            const auto next = at == ring.end() || at + 1 == ring.end() ? ring.begin() : at + 1;

            wanted.settings.audio.microphone = *next;
            std::cout << "microphone -> " << deviceName(*next) << std::endl;
            reportChanges(settings->request(wanted, calls.calls()));
            return true;
        }
        case 'R': {

            std::vector<core::AudioDevice> ring{core::AudioDevice{}};
            for (const auto &device : core::devicesWorthOffering(backend.devices())) {
                if (device.canPlay()) {
                    ring.push_back(core::AudioDevice{device.driver, device.name});
                }
            }

            auto wanted = settings->desired();
            const auto &current = wanted.settings.audio.ringer;
            const auto at = std::find(ring.begin(), ring.end(), current);
            const auto next = at == ring.end() || at + 1 == ring.end() ? ring.begin() : at + 1;

            wanted.settings.audio.ringer = *next;
            std::cout << "ringer -> " << deviceName(*next) << std::endl;
            reportChanges(settings->request(wanted, calls.calls()));
            return true;
        }
        case 'L': {

            metering = !metering;
            if (metering) {
                backend.startMetering();
            } else {
                backend.stopMetering();
            }
            std::cout << "level meter " << (metering ? "on" : "off") << std::endl;
            return true;
        }
        case 'X': {

            auto wanted = settings->desired();
            const auto id = core::dialingAccount(wanted);
            const auto *account = core::find(wanted, id);
            if (account == nullptr) {
                std::cout << "no account to change" << std::endl;
                return true;
            }
            auto changed = *account;
            changed.dtmfTransport = changed.dtmfTransport == core::DtmfTransport::Rfc2833
                                        ? core::DtmfTransport::SipInfo
                                        : core::DtmfTransport::Rfc2833;
            core::upsert(wanted, changed);
            reportChanges(settings->request(wanted, calls.calls()));
            return true;
        }
        case 'E': {

            auto wanted = settings->desired();
            for (auto &account : wanted.accounts) {
                if (account.id == arguments->account.id) {
                    account.expiry = account.expiry == 300s ? 600s : 300s;
                }
            }
            reportChanges(settings->request(wanted, calls.calls()));
            return true;
        }
        default:
            return false;
        }
    };

    while (!loop.stopped()) {
        loop.runPending();
        watchdog();

        if (const auto key = keyboard->pressed()) {
            const bool wasSettings = settingsKey(*key);
            if (const auto call = calls.current();
                !wasSettings && call.has_value() && call->state == core::CallState::Active) {
                switch (*key) {
                case 'h':
                    if (call->heldByUs) {
                        calls.activate(call->id);
                    } else {
                        calls.hold(call->id);
                    }
                    break;
                case 'm':
                    calls.setMuted(call->id, !call->muted);
                    break;
                case 'q':
                    calls.hangUp(call->id);
                    break;
                case 'c':

                    {
                        std::vector<std::string> everyone;
                        for (const auto &other : calls.conferenceCandidates()) {
                            everyone.push_back(other.id);
                        }
                        calls.merge(everyone);
                    }
                    break;
                case 's':
                    calls.split(call->id);
                    break;
                case 'r':
                    if (recording.recording(call->id)) {
                        stopRecording(call->id);
                    } else {
                        startRecording(*call);
                    }
                    break;
                case 't':

                    if (arguments->transferTarget.empty()) {
                        std::cout << "  nothing to transfer to; pass --transfer-to" << std::endl;
                        break;
                    }
                    calls.transferBlind(
                        call->id, core::dialTarget(arguments->transferTarget, arguments->account));
                    break;
                default:

                    if (core::isDtmfDigit(*key)) {
                        calls.sendDtmf(call->id, *key);
                        if (call->canSendDtmf()) {
                            std::cout << "  sent " << *key << std::endl;
                        } else {
                            std::cout << "  " << *key << " not sent: the call is on hold"
                                      << std::endl;
                        }
                    }
                    break;
                }
            }
        }

        if (const auto call = calls.current();
            (metering || (call.has_value() && call->state == core::CallState::Active)) &&
            std::chrono::steady_clock::now() >= nextLevelReport) {
            nextLevelReport = std::chrono::steady_clock::now() + 1s;

            if (keyboard->available() && !keysAnnounced) {
                keysAnnounced = true;
                std::cout << "  keys: 0-9 * # dial, h hold/resume, m mute, r record, q hang up, "
                             "S settings, D devices, M microphone, R ringer, L level, C codecs, "
                             "X dtmf, E expiry"
                          << std::endl;
            }

            if (const auto levels = backend.audioLevels()) {
                std::cout << "  mic " << levels->capture << '/' << core::kLoudestLevel
                          << ", speaker " << levels->playback << '/' << core::kLoudestLevel
                          << std::endl;
            }
        }
        if (!loop.stopped()) {
            loop.runFor(50ms);
        }
    }

    for (auto open = recording.states(); !open.empty(); open = recording.states()) {
        stopRecording(open.front().calls.front());
    }

    if (timedOut) {
        std::cerr << "Timed out after " << arguments->timeout.count() << "s without an outcome\n";
        return kTimedOut;
    }

    if (gaveUpRinging) {
        return kCallNoAnswer;
    }

    if (callPlaced || callEnded) {
        return exitCodeFor(callOutcome);
    }

    if (settledState == core::RegistrationState::Registered) {
        return kSuccess;
    }

    return exitCodeFor(settledCategory);
}
