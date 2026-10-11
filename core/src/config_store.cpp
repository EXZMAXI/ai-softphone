// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/config_store.hpp>

#include <softphone/core/paths.hpp>
#include <softphone/core/webhook_events.hpp>

#include <algorithm>
#include <array>

#include <nlohmann/json.hpp>

#include <fstream>
#include <system_error>

namespace softphone::core {
namespace {

constexpr auto kFileName = "settings.json";

[[nodiscard]] nlohmann::json toJson(const AccountConfig &account)
{

    return nlohmann::json{
        {"id", account.id},
        {"uuid", account.uuid},
        {"displayName", account.displayName},
        {"username", account.username},
        {"authUser", account.authUser},
        {"domain", account.domain},
        {"registrar", account.registrar},
        {"outboundProxy", account.outboundProxy},
        {"port", account.port},
        {"transport", toString(account.transport)},
        {"verifyCertificate", account.verifyCertificate},
        {"mediaEncryption", toString(account.mediaEncryption)},
        {"expirySeconds", account.expiry.count()},
        {"enabled", account.enabled},
        {"dtmfTransport", toString(account.dtmfTransport)},
        {"consent", account.consent},
        {"autoAnswer", account.autoAnswer},
        {"autoAnswerAfterSeconds", account.autoAnswerAfter.count()},
    };
}

[[nodiscard]] AccountConfig accountFrom(const nlohmann::json &json, DtmfTransport fallbackDtmf)
{
    AccountConfig account;
    account.id = json.value("id", std::string{});

    account.uuid = json.value("uuid", std::string{});
    account.displayName = json.value("displayName", std::string{});
    account.username = json.value("username", std::string{});
    account.authUser = json.value("authUser", std::string{});
    account.domain = json.value("domain", std::string{});
    account.registrar = json.value("registrar", std::string{});
    account.outboundProxy = json.value("outboundProxy", std::string{});
    account.port = json.value("port", std::uint16_t{5060});

    if (const auto transport = parseTransport(json.value("transport", std::string{"udp"}))) {
        account.transport = *transport;
    }

    account.verifyCertificate = json.value("verifyCertificate", true);
    if (const auto encryption =
            parseMediaEncryption(json.value("mediaEncryption", std::string{}))) {
        account.mediaEncryption = *encryption;
    }

    account.expiry = std::chrono::seconds{json.value("expirySeconds", std::int64_t{300})};
    account.enabled = json.value("enabled", true);

    account.consent = json.value("consent", std::string{});
    account.dtmfTransport = fallbackDtmf;
    if (const auto dtmf = parseDtmfTransport(json.value("dtmfTransport", std::string{}))) {
        account.dtmfTransport = *dtmf;
    }

    account.autoAnswer = json.value("autoAnswer", false);
    account.autoAnswerAfter =
        std::chrono::seconds{json.value("autoAnswerAfterSeconds", std::int64_t{0})};
    if (account.autoAnswerAfter < std::chrono::seconds{0} ||
        account.autoAnswerAfter > kMaxAutoAnswerSetting) {

        account.autoAnswerAfter = std::chrono::seconds{0};
    }
    return account;
}

[[nodiscard]] nlohmann::json toJson(const AudioDevice &device)
{
    return nlohmann::json{{"driver", device.driver}, {"name", device.name}};
}

[[nodiscard]] AudioDevice deviceFrom(const nlohmann::json &json)
{
    if (!json.is_object()) {
        return AudioDevice{};
    }
    return AudioDevice{json.value("driver", std::string{}), json.value("name", std::string{})};
}

[[nodiscard]] RingtoneChoice ringtoneFrom(const nlohmann::json &json)
{
    if (!json.is_object()) {
        return {};
    }

    RingtoneChoice choice;
    choice.file = json.value("file", std::string{});
    choice.bundled = json.value("bundled", std::string{});

    if (choice.bundled.empty()) {
        if (choice.file.empty()) {
            return {};
        }
        return choice;
    }

    if (bundledRingtoneFile(choice.bundled).empty()) {
        choice.bundled = std::string{defaultRingtone()};
    }
    choice.file.clear();
    return choice;
}

[[nodiscard]] Volumes volumesFrom(const nlohmann::json &json)
{
    Volumes volumes;
    if (!json.is_object()) {
        return volumes;
    }

    const auto level = [&json](const char *name, int fallback) {
        const auto found = json.find(name);
        if (found == json.end() || !found->is_number_integer()) {
            return fallback;
        }
        return std::clamp(found->get<int>(), 0, 100);
    };

    volumes.microphone = level("microphone", volumes.microphone);
    volumes.speakers = level("speakers", volumes.speakers);
    volumes.ring = level("ring", volumes.ring);
    volumes.notifications = level("notifications", volumes.notifications);
    return volumes;
}

[[nodiscard]] nlohmann::json writeAudio(const Settings &settings)
{
    return nlohmann::json{
        {"microphone", toJson(settings.audio.microphone)},
        {"speakers", toJson(settings.audio.speakers)},
        {"ringer", toJson(settings.audio.ringer)},
        {"ringtone",
         {
             {"bundled", settings.audio.ringtone.bundled},
             {"file", settings.audio.ringtone.file},
         }},
        {"volumes",
         {
             {"microphone", settings.audio.volumes.microphone},
             {"speakers", settings.audio.volumes.speakers},
             {"ring", settings.audio.volumes.ring},
             {"notifications", settings.audio.volumes.notifications},
         }},
    };
}

[[nodiscard]] nlohmann::json writeCalls(const Settings &settings)
{
    nlohmann::json codecs = nlohmann::json::array();
    for (const auto &codec : settings.calls.codecs) {
        codecs.push_back(nlohmann::json{{"id", codec.id}, {"enabled", codec.enabled}});
    }

    return nlohmann::json{
        {"codecs", codecs},
        {"dialAccount", settings.calls.dialAccount},
        {"callWaiting", std::string{toString(settings.calls.callWaiting)}},
        {"redialIntervalSeconds", settings.calls.redialIntervalSeconds},
        {"historyDays", settings.calls.historyDays},
        {"redialGiveUpMinutes", settings.calls.redialGiveUpMinutes},
    };
}

void readAudio(const nlohmann::json &group, Settings &settings)
{
    if (!group.is_object()) {
        return;
    }

    settings.audio.microphone = deviceFrom(group.value("microphone", nlohmann::json::object()));
    settings.audio.speakers = deviceFrom(group.value("speakers", nlohmann::json::object()));
    settings.audio.ringer = deviceFrom(group.value("ringer", nlohmann::json::object()));
    settings.audio.ringtone = ringtoneFrom(group.value("ringtone", nlohmann::json::object()));
    settings.audio.volumes = volumesFrom(group.value("volumes", nlohmann::json::object()));
}

void readCalls(const nlohmann::json &group, Settings &settings)
{
    if (!group.is_object()) {
        return;
    }

    if (const auto codecs = group.find("codecs"); codecs != group.end() && codecs->is_array()) {
        for (const auto &entry : *codecs) {
            if (!entry.is_object()) {
                continue;
            }
            const auto id = entry.value("id", std::string{});
            if (!id.empty()) {
                settings.calls.codecs.push_back(CodecChoice{id, entry.value("enabled", true)});
            }
        }
    }

    settings.calls.dialAccount = group.value("dialAccount", std::string{});

    if (const auto waiting = parseCallWaiting(group.value("callWaiting", std::string{"allowed"}))) {
        settings.calls.callWaiting = *waiting;
    }

    settings.calls.historyDays = std::clamp(group.value("historyDays", 0), 0, kMaxHistoryDays);
    settings.calls.redialIntervalSeconds =
        std::clamp(group.value("redialIntervalSeconds", 15), kMinRedialIntervalSeconds,
                   kMaxRedialIntervalSeconds);
    settings.calls.redialGiveUpMinutes = std::clamp(
        group.value("redialGiveUpMinutes", 30), kMinRedialGiveUpMinutes, kMaxRedialGiveUpMinutes);

}

struct GroupOnDisk {
    std::string_view name;
    std::string ConfigLayout::*home;
    nlohmann::json (*write)(const Settings &settings);
    void (*read)(const nlohmann::json &group, Settings &settings);
};

constexpr std::array kGroups = {
    GroupOnDisk{"audio", &ConfigLayout::telephone, &writeAudio, &readAudio},
    GroupOnDisk{"calls", &ConfigLayout::telephone, &writeCalls, &readCalls},
};

struct DepartedGroup {
    std::string_view name;
    std::string ConfigLayout::*home;
};

const std::array kDepartedGroups = {
    DepartedGroup{"buttons", &ConfigLayout::buttons},
    DepartedGroup{"appearance", &ConfigLayout::application},
    DepartedGroup{"startup", &ConfigLayout::application},
    DepartedGroup{"hotkeys", &ConfigLayout::application},
    DepartedGroup{"updates", &ConfigLayout::application},
    DepartedGroup{"diagnostics", &ConfigLayout::diagnostics},
    DepartedGroup{"webhooks", &ConfigLayout::integration},
    DepartedGroup{"api", &ConfigLayout::integration},
    DepartedGroup{"recording", &ConfigLayout::recording},
    DepartedGroup{"transcription", &ConfigLayout::transcription},
    DepartedGroup{"pipeline", &ConfigLayout::processing},
};

[[nodiscard]] std::map<std::string, OwnedSettings> ownedFrom(const nlohmann::json &settings,
                                                             const ConfigLayout &layout)
{
    std::map<std::string, OwnedSettings> owned;
    if (!settings.is_object()) {
        return owned;
    }

    for (const auto &[name, content] : settings.items()) {

        const auto known = std::ranges::any_of(
            kGroups, [&wanted = name](const auto &row) { return row.name == wanted; });
        if (known) {
            continue;
        }

        const auto departed = std::ranges::find(kDepartedGroups, name, &DepartedGroup::name);
        auto owner = departed == kDepartedGroups.end() ? std::string{} : layout.*(departed->home);
        owned[name] = OwnedSettings{std::move(owner), content.dump()};
    }
    return owned;
}

void writeOwnedInto(nlohmann::json &settings, const StoredConfiguration &configuration)
{
    for (const auto &[name, group] : configuration.owned) {
        auto content = nlohmann::json::parse(group.value, nullptr, false);
        settings[name] = content.is_discarded() ? nlohmann::json{group.value} : std::move(content);
    }
}

[[nodiscard]] nlohmann::json toJson(const Settings &settings)
{
    auto out = nlohmann::json::object();
    for (const auto &group : kGroups) {
        out[std::string{group.name}] = group.write(settings);
    }
    return out;
}

[[nodiscard]] Settings settingsFrom(const nlohmann::json &json)
{
    Settings settings;
    if (!json.is_object()) {
        return settings;
    }

    for (const auto &group : kGroups) {
        if (const auto found = json.find(group.name); found != json.end()) {
            group.read(*found, settings);
        }
    }

    return settings;
}

[[nodiscard]] ConfigStoreStatus readDocument(const nlohmann::json &json, const ConfigLayout &layout,
                                             StoredConfiguration &out)
{
    if (json.is_discarded() || !json.is_object()) {
        return ConfigStoreStatus::Malformed;
    }

    const auto schemaVersion = json.value("schemaVersion", 0);
    if (schemaVersion <= 0) {
        return ConfigStoreStatus::Malformed;
    }
    if (schemaVersion > kCurrentSchemaVersion) {
        return ConfigStoreStatus::UnsupportedSchema;
    }

    const auto accounts = json.find("accounts");
    if (accounts != json.end() && !accounts->is_array()) {
        return ConfigStoreStatus::Malformed;
    }

    StoredConfiguration loaded;
    loaded.schemaVersion = schemaVersion;
    loaded.settings = settingsFrom(json.value("settings", nlohmann::json::object()));
    loaded.owned = ownedFrom(json.value("settings", nlohmann::json::object()), layout);
    loaded.nextAccountNumber = std::max(1, json.value("nextAccountNumber", 1));

    auto inherited = DtmfTransport::Rfc2833;
    if (const auto settings = json.find("settings");
        settings != json.end() && settings->is_object()) {
        if (const auto calls = settings->find("calls");
            calls != settings->end() && calls->is_object()) {
            if (const auto dtmf =
                    parseDtmfTransport(calls->value("dtmfTransport", std::string{}))) {
                inherited = *dtmf;
            }
        }
    }

    if (accounts != json.end()) {
        for (const auto &entry : *accounts) {
            if (!entry.is_object()) {
                return ConfigStoreStatus::Malformed;
            }
            loaded.accounts.push_back(accountFrom(entry, inherited));
        }
    }

    out = std::move(loaded);
    return ConfigStoreStatus::Ok;
}

[[nodiscard]] const std::string &homeOf(std::string_view group, const ConfigLayout &layout)
{
    for (const auto &known : kGroups) {
        if (known.name == group) {
            const auto &section = layout.*(known.home);
            if (!section.empty()) {
                return section;
            }
        }
    }

    for (const auto &departed : kDepartedGroups) {
        if (departed.name == group) {
            const auto &section = layout.*(departed.home);
            if (!section.empty()) {
                return section;
            }
        }
    }
    return layout.application;
}

[[nodiscard]] const std::string &sectionOf(const std::string &group,
                                           const StoredConfiguration &configuration,
                                           const ConfigLayout &layout)
{
    if (const auto owned = configuration.owned.find(group);
        owned != configuration.owned.end() && !owned->second.owner.empty()) {
        return owned->second.owner;
    }
    return homeOf(group, layout);
}

[[nodiscard]] std::vector<std::string> sectionsOf(const ConfigLayout &layout)
{
    std::vector<std::string> names;
    for (const auto *name :
         {&layout.application, &layout.telephone, &layout.buttons, &layout.recording,
          &layout.transcription, &layout.processing, &layout.integration, &layout.diagnostics}) {
        if (!name->empty() && std::ranges::find(names, *name) == names.end()) {
            names.push_back(*name);
        }
    }
    return names;
}

[[nodiscard]] nlohmann::json writeDocument(const StoredConfiguration &configuration)
{
    nlohmann::json json;
    json["schemaVersion"] = kCurrentSchemaVersion;
    json["accounts"] = nlohmann::json::array();
    for (const auto &account : configuration.accounts) {
        json["accounts"].push_back(toJson(account));
    }
    auto settings = toJson(configuration.settings);
    writeOwnedInto(settings, configuration);
    json["settings"] = std::move(settings);
    json["nextAccountNumber"] = configuration.nextAccountNumber;
    return json;
}

}

std::filesystem::path defaultConfigDirectory()
{

    return configDirectory();
}

std::string_view toString(ConfigStoreStatus status) noexcept
{
    switch (status) {
    case ConfigStoreStatus::Ok:
        return "ok";
    case ConfigStoreStatus::NotFound:
        return "not-found";
    case ConfigStoreStatus::Unreadable:
        return "unreadable";
    case ConfigStoreStatus::Malformed:
        return "malformed";
    case ConfigStoreStatus::UnsupportedSchema:
        return "unsupported-schema";
    case ConfigStoreStatus::NotWritable:
        return "not-writable";
    }
    return "unknown";
}

ConfigStore::ConfigStore(std::filesystem::path directory) : directory_{std::move(directory)} {}

std::filesystem::path defaultSettingsFile()
{
    return defaultConfigDirectory() / kFileName;
}

const ConfigLayout &defaultConfigLayout()
{

    static const ConfigLayout layout{
        .application = "application",
        .telephone = "softphone",
        .buttons = "blf",
        .recording = "recording",
        .transcription = "transcription",
        .processing = "processing",
        .integration = "integration",
        .diagnostics = "diagnostics",
    };
    return layout;
}

std::filesystem::path ConfigStore::filePath() const
{
    return document_.bound() ? document_.shownAs : directory_ / kFileName;
}

ConfigStore::ConfigStore(ConfigDocument document, ConfigLayout layout)
    : document_{std::move(document)}, layout_{std::move(layout)}
{}

ConfigStoreStatus ConfigStore::read(std::string_view text, const ConfigLayout &layout,
                                    StoredConfiguration &out)
{
    if (text.empty()) {
        return ConfigStoreStatus::NotFound;
    }

    const auto json = nlohmann::json::parse(text, nullptr, false);
    return readDocument(json, layout, out);
}

std::string ConfigStore::write(const StoredConfiguration &configuration)
{
    return writeDocument(configuration).dump(2);
}

std::vector<std::string> ConfigStore::groupsWithNoHome()
{
    std::vector<std::string> strays;

    const auto groups = toJson(Settings{});
    for (const auto &[group, content] : groups.items()) {
        const auto known = std::ranges::any_of(
            kGroups, [&wanted = group](const auto &row) { return row.name == wanted; });
        if (!known) {
            strays.push_back(group);
        }
    }
    return strays;
}

std::vector<std::pair<std::string, std::string>>
ConfigStore::writeSections(const StoredConfiguration &configuration, const ConfigLayout &layout)
{

    const auto whole = writeDocument(configuration);

    std::vector<std::pair<std::string, nlohmann::json>> built;
    const auto sectionFor = [&built](const std::string &name) -> nlohmann::json & {
        const auto found =
            std::ranges::find(built, name, &std::pair<std::string, nlohmann::json>::first);
        if (found != built.end()) {
            return found->second;
        }
        auto document = nlohmann::json::object();
        document["schemaVersion"] = kCurrentSchemaVersion;
        return built.emplace_back(name, std::move(document)).second;
    };

    for (const auto &name : sectionsOf(layout)) {
        static_cast<void>(sectionFor(name));
    }

    auto &telephone = sectionFor(layout.telephone);
    telephone["accounts"] = whole.value("accounts", nlohmann::json::array());
    telephone["nextAccountNumber"] = whole.value("nextAccountNumber", 1);

    const auto groups = whole.value("settings", nlohmann::json::object());
    for (const auto &[group, content] : groups.items()) {
        sectionFor(sectionOf(group, configuration, layout))["settings"][group] = content;
    }

    std::vector<std::pair<std::string, std::string>> rendered;
    rendered.reserve(built.size());
    for (const auto &[name, document] : built) {
        rendered.emplace_back(name, document.dump(2));
    }
    return rendered;
}

ConfigStoreStatus
ConfigStore::readSections(const std::function<std::string(std::string_view)> &section,
                          const ConfigLayout &layout, StoredConfiguration &out,
                          const std::vector<std::string> &held)
{
    auto whole = nlohmann::json::object();
    auto settings = nlohmann::json::object();
    std::map<std::string, std::string> cameFrom;
    auto schema = 0;
    auto found = false;

    for (const auto &name : sectionsOf(layout)) {
        const auto text = section(name);
        if (text.empty()) {
            continue;
        }
        const auto document = nlohmann::json::parse(text, nullptr, false);
        if (document.is_discarded() || !document.is_object()) {
            return ConfigStoreStatus::Malformed;
        }
        const auto version = document.value("schemaVersion", 0);
        if (version <= 0) {
            return ConfigStoreStatus::Malformed;
        }
        if (version > kCurrentSchemaVersion) {
            return ConfigStoreStatus::UnsupportedSchema;
        }
        found = true;
        schema = std::max(schema, version);

        if (const auto accounts = document.find("accounts"); accounts != document.end()) {
            whole["accounts"] = *accounts;
        }
        if (const auto next = document.find("nextAccountNumber"); next != document.end()) {
            whole["nextAccountNumber"] = *next;
        }
        const auto groups = document.value("settings", nlohmann::json::object());
        for (const auto &[group, content] : groups.items()) {
            settings[group] = content;

            cameFrom[group] = name;
        }
    }

    const auto ours = sectionsOf(layout);
    for (const auto &name : held) {
        if (name.empty() || name == kLegacySettingsSection ||
            std::ranges::find(ours, name) != ours.end()) {
            continue;
        }

        const auto document = nlohmann::json::parse(section(name), nullptr, false);
        if (document.is_discarded() || !document.is_object()) {
            continue;
        }

        const auto groups = document.value("settings", nlohmann::json::object());
        if (!groups.is_object()) {
            continue;
        }

        for (const auto &[group, content] : groups.items()) {
            const auto known = std::ranges::any_of(
                kGroups, [&wanted = group](const auto &row) { return row.name == wanted; });
            if (known) {
                continue;
            }
            found = true;
            settings[group] = content;
            cameFrom[group] = name;
        }
    }

    if (!found) {
        return ConfigStoreStatus::NotFound;
    }

    whole["schemaVersion"] = schema > 0 ? schema : kCurrentSchemaVersion;
    whole["settings"] = std::move(settings);
    const auto status = readDocument(whole, layout, out);
    if (status != ConfigStoreStatus::Ok) {
        return status;
    }

    for (auto &[group, group_] : out.owned) {
        if (const auto where = cameFrom.find(group); where != cameFrom.end()) {
            group_.owner = where->second;
        }
    }
    return status;
}

ConfigStoreStatus ConfigStore::load(StoredConfiguration &out) const
{
    if (document_.bound()) {

        const auto section = [this](std::string_view name) { return document_.read(name); };
        const auto status =
            readSections(section, layout_, out,
                         document_.sections ? document_.sections() : std::vector<std::string>{});
        if (status != ConfigStoreStatus::NotFound) {
            return status;
        }
        const auto legacy = read(document_.read(kLegacySettingsSection), layout_, out);
        if (legacy == ConfigStoreStatus::Ok) {

            static_cast<void>(save(out));
        }
        return legacy;
    }

    std::error_code error;
    const auto path = filePath();
    if (!std::filesystem::exists(path, error) || error) {
        return ConfigStoreStatus::NotFound;
    }

    std::ifstream file{path};
    if (!file) {
        return ConfigStoreStatus::Unreadable;
    }

    const auto json = nlohmann::json::parse(file, nullptr, false);
    return readDocument(json, layout_, out);
}

ConfigStoreStatus ConfigStore::save(const StoredConfiguration &configuration) const
{
    if (document_.bound()) {
        for (const auto &[name, text] : writeSections(configuration, layout_)) {
            document_.write(name, text);
        }

        document_.erase(kLegacySettingsSection);

        return document_.flush() ? ConfigStoreStatus::Ok : ConfigStoreStatus::NotWritable;
    }

    std::error_code error;
    std::filesystem::create_directories(directory_, error);
    if (error) {
        return ConfigStoreStatus::NotWritable;
    }

    const auto path = filePath();
    const auto temporary = std::filesystem::path{path}.concat(".new");

    {
        std::ofstream file{temporary, std::ios::trunc};
        if (!file) {
            return ConfigStoreStatus::NotWritable;
        }
        file << write(configuration) << '\n';
        if (!file) {
            return ConfigStoreStatus::NotWritable;
        }
    }

    if (!restrictToOwner(temporary)) {
        std::filesystem::remove(temporary, error);
        return ConfigStoreStatus::NotWritable;
    }

    std::filesystem::rename(temporary, path, error);
    if (error) {
        std::filesystem::remove(temporary, error);
        return ConfigStoreStatus::NotWritable;
    }

    return ConfigStoreStatus::Ok;
}

}
