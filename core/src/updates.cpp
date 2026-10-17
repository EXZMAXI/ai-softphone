// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/updates.hpp>

#include <map>

#include <softphone/core/logging.hpp>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <system_error>
#include <tuple>
#include <utility>

namespace softphone::core {
namespace {

constexpr std::string_view kSignatureSuffix{".sig"};

constexpr std::string_view kArea{"updates"};

constexpr std::string_view configuredManifestUrls() noexcept
{
#ifdef SOFTPHONE_UPDATE_MANIFEST_URLS
    return SOFTPHONE_UPDATE_MANIFEST_URLS;
#else
    return {};
#endif
}

constexpr std::string_view configuredManifestUrlsByCountry() noexcept
{
#ifdef SOFTPHONE_UPDATE_MANIFEST_URLS_BY_COUNTRY
    return SOFTPHONE_UPDATE_MANIFEST_URLS_BY_COUNTRY;
#else
    return {};
#endif
}

[[nodiscard]] std::vector<std::string> asUrlList(std::string_view text)
{
    std::vector<std::string> urls;
    while (!text.empty()) {
        const auto stop = text.find(' ');
        const auto one = text.substr(0, stop);
        if (!one.empty()) {
            urls.emplace_back(one);
        }
        if (stop == std::string_view::npos) {
            break;
        }
        text.remove_prefix(stop + 1);
    }
    return urls;
}

[[nodiscard]] std::map<std::string, std::vector<std::string>>
asUrlListByCountry(std::string_view text)
{
    std::map<std::string, std::vector<std::string>> byCountry;
    for (const auto &entry : asUrlList(text)) {
        const auto divide = entry.find('=');
        if (divide == std::string::npos || divide == 0 || divide + 1 == entry.size()) {
            continue;
        }
        byCountry[entry.substr(0, divide)].push_back(entry.substr(divide + 1));
    }
    return byCountry;
}

constexpr std::string_view configuredPublicKey() noexcept
{
#ifdef SOFTPHONE_UPDATE_PUBLIC_KEY
    return SOFTPHONE_UPDATE_PUBLIC_KEY;
#else
    return {};
#endif
}

constexpr std::string_view configuredFormat() noexcept
{
#ifdef SOFTPHONE_PACKAGE_FORMAT
    return SOFTPHONE_PACKAGE_FORMAT;
#else
    return "source";
#endif
}

[[nodiscard]] std::vector<std::string> eachOnce(std::vector<std::string> asked)
{
    std::vector<std::string> once;
    once.reserve(asked.size());
    for (auto &url : asked) {
        if (std::find(once.begin(), once.end(), url) == once.end()) {
            once.push_back(std::move(url));
        }
    }
    return once;
}

[[nodiscard]] std::int64_t sinceInMilliseconds(std::chrono::steady_clock::time_point began)
{
    const auto spent = std::chrono::steady_clock::now() - began;
    return std::chrono::duration_cast<std::chrono::milliseconds>(spent).count();
}

[[nodiscard]] std::string_view trimmed(std::string_view text) noexcept
{
    const auto isSpace = [](char character) {
        return std::isspace(static_cast<unsigned char>(character)) != 0;
    };

    while (!text.empty() && isSpace(text.front())) {
        text.remove_prefix(1);
    }
    while (!text.empty() && isSpace(text.back())) {
        text.remove_suffix(1);
    }
    return text;
}

[[nodiscard]] std::string fileNameFrom(std::string_view url, std::string_view fallback)
{
    const auto slash = url.find_last_of('/');
    auto candidate = slash == std::string_view::npos ? url : url.substr(slash + 1);

    const auto query = candidate.find_first_of("?#");
    if (query != std::string_view::npos) {
        candidate = candidate.substr(0, query);
    }

    std::string name;
    name.reserve(candidate.size());
    for (const char character : candidate) {
        const auto plain = static_cast<unsigned char>(character);
        if (std::isalnum(plain) != 0 || character == '.' || character == '-' || character == '_' ||
            character == '+') {
            name.push_back(character);
        }
    }

    if (name.empty() || name.find_first_not_of('.') == std::string::npos) {
        return std::string{fallback};
    }
    return name;
}

}

std::string_view toString(Delivery delivery) noexcept
{
    switch (delivery) {
    case Delivery::Source:
        return "source";
    case Delivery::Deb:
        return "deb";
    case Delivery::AppImage:
        return "appimage";
    case Delivery::Installer:
        return "installer";
    case Delivery::Dmg:
        return "dmg";
    }
    return "source";
}

std::optional<Delivery> parseDelivery(std::string_view text) noexcept
{
    if (text == "source") {
        return Delivery::Source;
    }
    if (text == "deb") {
        return Delivery::Deb;
    }
    if (text == "appimage") {
        return Delivery::AppImage;
    }
    if (text == "installer") {
        return Delivery::Installer;
    }
    if (text == "dmg") {
        return Delivery::Dmg;
    }
    return std::nullopt;
}

std::string_view toString(UpdateState state) noexcept
{
    switch (state) {
    case UpdateState::Unconfigured:
        return "unconfigured";
    case UpdateState::Idle:
        return "idle";
    case UpdateState::Checking:
        return "checking";
    case UpdateState::UpToDate:
        return "up-to-date";
    case UpdateState::Available:
        return "available";
    case UpdateState::Downloading:
        return "downloading";
    case UpdateState::Ready:
        return "ready";
    case UpdateState::Failed:
        return "failed";
    }
    return "idle";
}

std::vector<std::string> UpdateChannel::hostsFor(const std::string &country) const
{
    auto asked = manifestUrls;
    if (const auto found = manifestUrlsByCountry.find(country);
        !country.empty() && found != manifestUrlsByCountry.end()) {
        asked.insert(asked.end(), found->second.begin(), found->second.end());
    }

    return eachOnce(asked);
}

std::vector<std::string> UpdateChannel::downloadHostsFor(const std::string &country) const
{

    std::vector<std::string> asked;
    if (const auto found = manifestUrlsByCountry.find(country);
        !country.empty() && found != manifestUrlsByCountry.end()) {
        asked = found->second;
    }
    asked.insert(asked.end(), manifestUrls.begin(), manifestUrls.end());
    return eachOnce(asked);
}

UpdateChannel buildChannel()
{
    UpdateChannel channel;
    channel.manifestUrls = asUrlList(configuredManifestUrls());
    channel.manifestUrlsByCountry = asUrlListByCountry(configuredManifestUrlsByCountry());
    channel.publicKey = std::string{configuredPublicKey()};
    channel.delivery = parseDelivery(configuredFormat()).value_or(Delivery::Source);

    if (const char *image = std::getenv("APPIMAGE"); image != nullptr && *image != '\0') {
        channel.delivery = Delivery::AppImage;
        channel.running = std::filesystem::path{image};
    }

    return channel;
}

const ReleasePackage *Release::packageFor(Delivery delivery) const noexcept
{
    const auto wanted = toString(delivery);
    const auto found =
        std::find_if(packages.begin(), packages.end(),
                     [wanted](const ReleasePackage &package) { return package.format == wanted; });
    return found == packages.end() ? nullptr : &*found;
}

std::optional<Version> parseVersion(std::string_view text) noexcept
{
    std::array<int, 3> parts{};

    for (std::size_t index = 0; index < parts.size(); ++index) {
        if (text.empty()) {
            return std::nullopt;
        }

        const auto stop = text.find('.');
        const auto piece = text.substr(0, stop);

        if (piece.empty() || piece.size() > 9) {
            return std::nullopt;
        }
        if (!std::all_of(piece.begin(), piece.end(), [](char character) {
                return std::isdigit(static_cast<unsigned char>(character)) != 0;
            })) {
            return std::nullopt;
        }

        int value = 0;
        const auto *const begin = piece.data();
        const auto [_, error] = std::from_chars(begin, begin + piece.size(), value);
        if (error != std::errc{}) {
            return std::nullopt;
        }
        parts[index] = value;

        if (index + 1 == parts.size()) {

            if (stop != std::string_view::npos) {
                return std::nullopt;
            }
            text = {};
        } else {
            if (stop == std::string_view::npos) {
                return std::nullopt;
            }
            text.remove_prefix(stop + 1);
        }
    }

    return Version{parts[0], parts[1], parts[2]};
}

bool isNewer(const Version &offered, const Version &running) noexcept
{
    return std::tie(offered.major, offered.minor, offered.patch) >
           std::tie(running.major, running.minor, running.patch);
}

std::optional<Release> readManifest(std::string_view document, std::string &complaint)
{
    complaint.clear();

    const auto parsed = nlohmann::json::parse(document, nullptr, false);
    if (parsed.is_discarded() || !parsed.is_object()) {
        complaint = "the manifest is not a JSON object";
        return std::nullopt;
    }

    Release release;

    const auto versionField = parsed.find("version");
    if (versionField == parsed.end() || !versionField->is_string()) {
        complaint = "the manifest names no version";
        return std::nullopt;
    }
    release.versionText = versionField->get<std::string>();

    const auto version = parseVersion(release.versionText);
    if (!version) {
        complaint = "the version is not major.minor.patch: " + release.versionText;
        return std::nullopt;
    }
    release.version = *version;

    if (const auto notes = parsed.find("notes"); notes != parsed.end() && notes->is_string()) {
        release.notes = notes->get<std::string>();
    }

    const auto packages = parsed.find("packages");
    if (packages == parsed.end() || !packages->is_array()) {
        complaint = "the manifest lists no packages";
        return std::nullopt;
    }

    for (const auto &entry : *packages) {
        if (!entry.is_object()) {
            continue;
        }

        ReleasePackage package;
        if (const auto format = entry.find("format");
            format != entry.end() && format->is_string()) {
            package.format = format->get<std::string>();
        }
        if (const auto url = entry.find("url"); url != entry.end() && url->is_string()) {
            package.url = url->get<std::string>();
        }
        if (const auto size = entry.find("size");
            size != entry.end() && size->is_number_unsigned()) {
            package.size = size->get<std::uint64_t>();
        }
        if (const auto digest = entry.find("sha256");
            digest != entry.end() && digest->is_string()) {
            package.sha256 = digest->get<std::string>();
        }

        if (package.format.empty() || package.url.empty() || package.sha256.empty()) {
            continue;
        }

        if (!package.url.starts_with("https://")) {
            continue;
        }

        release.packages.push_back(std::move(package));
    }

    if (release.packages.empty()) {
        complaint = "no package in the manifest is complete enough to use";
        return std::nullopt;
    }

    return release;
}

UpdateService::UpdateService(UpdateChannel channel, Version running, IUpdateTransport &transport,
                             IUpdateSignatures &signatures, std::filesystem::path workshop)
    : channel_{std::move(channel)}, running_{running}, transport_{transport},
      signatures_{signatures}, workshop_{std::move(workshop)}
{
    status_.state = channel_.configured() ? UpdateState::Idle : UpdateState::Unconfigured;
    status_.selfReplacing = channel_.selfReplacing();
    launcher_ = [](const std::filesystem::path &installer, std::string &complaint) {
        return runInstaller(installer, complaint);
    };

    worker_ = std::thread{[this] { work(); }};
}

void UpdateService::installWith(InstallerLauncher launcher)
{
    const std::lock_guard guard{mutex_};
    launcher_ = std::move(launcher);
}

UpdateService::~UpdateService()
{
    {
        const std::lock_guard guard{mutex_};
        stopping_ = true;
    }
    wakeup_.notify_all();

    if (worker_.joinable()) {
        worker_.join();
    }
}

void UpdateService::observe(std::function<void(const UpdateStatus &)> watcher)
{
    const std::lock_guard guard{watcherMutex_};
    watcher_ = std::move(watcher);
}

UpdateStatus UpdateService::status() const
{
    const std::lock_guard guard{mutex_};
    return status_;
}

void UpdateService::check()
{
    if (!channel_.configured()) {
        return;
    }

    {
        const std::lock_guard guard{mutex_};
        if (status_.state == UpdateState::Checking || status_.state == UpdateState::Downloading) {
            return;
        }
        jobs_.push_back(Job::Check);
    }
    wakeup_.notify_one();
}

void UpdateService::fetch()
{
    {
        const std::lock_guard guard{mutex_};
        if (!offered_ || status_.state == UpdateState::Downloading ||
            status_.state == UpdateState::Ready) {
            return;
        }
        jobs_.push_back(Job::Fetch);
    }
    wakeup_.notify_one();
}

void UpdateService::work()
{
    for (;;) {
        Job job{};
        {
            std::unique_lock guard{mutex_};
            wakeup_.wait(guard, [this] { return stopping_ || !jobs_.empty(); });
            if (stopping_) {
                return;
            }
            job = jobs_.front();
            jobs_.pop_front();
        }

        switch (job) {
        case Job::Check:
            doCheck();
            break;
        case Job::Fetch:
            doFetch();
            break;
        }
    }
}

void UpdateService::publish(const UpdateStatus &status)
{
    const std::lock_guard guard{watcherMutex_};
    if (watcher_) {
        watcher_(status);
    }
}

void UpdateService::logTheState(UpdateState state, const UpdateStatus &copy) const
{

    const auto said = toString(state);

    const auto here = std::to_string(running_.major) + "." + std::to_string(running_.minor) + "." +
                      std::to_string(running_.patch);
    const LogAttribute fields[]{
        {"state", said},
        {"running", here},
        {"offered", copy.offered},
        {"why", copy.complaint},
    };
    const auto level = state == UpdateState::Failed     ? LogLevel::Warning
                       : state == UpdateState::Checking ? LogLevel::Debug
                                                        : LogLevel::Info;
    Logger::instance().log(level, kArea, "the update state moved", fields);
}

void UpdateService::report(UpdateState state, const std::string &complaint)
{
    UpdateStatus copy;
    {
        const std::lock_guard guard{mutex_};
        status_.state = state;
        status_.complaint = complaint;

        if (state == UpdateState::UpToDate || state == UpdateState::Available ||
            state == UpdateState::Failed) {
            status_.checkedAt = std::chrono::system_clock::now();
        }
        copy = status_;

        logTheState(state, copy);
    }

    publish(copy);
}

void UpdateService::setCountry(std::string country)
{
    const std::lock_guard guard{mutex_};
    country_ = std::move(country);
}

std::optional<Release> UpdateService::manifestFrom(const std::string &url, std::string &complaint)
{
    const auto document = transport_.fetch(url);
    if (!document.ok) {
        complaint = document.error.empty() ? "the update server did not answer" : document.error;
        const LogAttribute refused[]{{"host", url}, {"why", complaint}};
        Logger::instance().log(LogLevel::Warning, kArea, "a host did not answer", refused);
        return std::nullopt;
    }

    const auto signature = transport_.fetch(url + std::string{kSignatureSuffix});
    if (!signature.ok) {
        complaint = "the manifest arrived without its signature";
        return std::nullopt;
    }

    if (!signatures_.verify(document.body, trimmed(signature.body), channel_.publicKey)) {
        complaint = "the manifest is not signed by this product's key";
        return std::nullopt;
    }

    auto found = readManifest(document.body, complaint);
    if (!found) {
        return std::nullopt;
    }
    return found;
}

void UpdateService::doCheck()
{
    report(UpdateState::Checking, {});

    std::optional<Release> best;
    std::string lastComplaint;
    std::size_t answered = 0;

    const auto asked = [this]() {
        const std::lock_guard guard{mutex_};
        return channel_.hostsFor(country_);
    }();

    std::vector<std::pair<std::string, Release>> proved;

    for (const std::string &url : asked) {
        const auto began = std::chrono::steady_clock::now();

        std::string complaint;
        auto found = manifestFrom(url, complaint);
        if (!found) {
            lastComplaint = complaint;
            continue;
        }

        ++answered;

        const auto shown = found->versionText;
        const auto took = std::to_string(sinceInMilliseconds(began));
        const LogAttribute answeredWith[]{{"host", url}, {"version", shown}, {"ms", took}};
        Logger::instance().log(LogLevel::Debug, kArea, "a host proved a version", answeredWith);

        if (!best || isNewer(found->version, best->version)) {
            best = *found;
        }
        proved.emplace_back(url, std::move(*found));
    }

    if (answered == 0) {
        report(UpdateState::Failed,
               lastComplaint.empty() ? "no update server answered" : lastComplaint);
        return;
    }

    auto release = std::move(best);

    if (!isNewer(release->version, running_)) {
        {
            const std::lock_guard guard{mutex_};
            offered_.reset();
            candidates_.clear();
            status_.offered.clear();
            status_.notes.clear();
            status_.downloadUrl.clear();
            status_.ready.clear();
        }
        const LogAttribute current[]{{"offered", release->versionText}};
        Logger::instance().log(LogLevel::Info, kArea,
                               "nothing newer is published than what is running", current);
        report(UpdateState::UpToDate, {});
        return;
    }

    std::vector<ReleasePackage> candidates;
    {
        const auto order = [this]() {
            const std::lock_guard guard{mutex_};
            return channel_.downloadHostsFor(country_);
        }();

        for (const std::string &url : order) {
            const auto found = std::find_if(proved.begin(), proved.end(),
                                            [&url](const auto &one) { return one.first == url; });
            if (found == proved.end() || !(found->second.version == release->version)) {
                continue;
            }
            const auto *one = found->second.packageFor(channel_.delivery);
            if (one == nullptr) {
                continue;
            }

            const auto already =
                std::find_if(candidates.begin(), candidates.end(),
                             [&one](const ReleasePackage &had) { return had.url == one->url; });
            if (already == candidates.end()) {
                candidates.push_back(*one);
            }
        }
    }

    const auto *package = candidates.empty() ? nullptr : &candidates.front();

    UpdateStatus copy;
    {
        const std::lock_guard guard{mutex_};
        status_.offered = release->versionText;
        status_.notes = release->notes;

        status_.downloadUrl = package != nullptr ? package->url : std::string{};
        status_.received = 0;
        status_.total = package != nullptr ? package->size : 0;
        status_.ready.clear();
        status_.state = UpdateState::Available;
        status_.complaint.clear();

        status_.checkedAt = std::chrono::system_clock::now();

        offered_ = std::move(release);
        candidates_ = candidates;
        copy = status_;

        logTheState(UpdateState::Available, copy);
        sayWhatWasFound(copy, candidates.size());
    }

    publish(copy);
}

void UpdateService::sayWhatWasFound(const UpdateStatus &copy, std::size_t copies) const
{

    const auto delivery = toString(channel_.delivery);
    const auto actionable = copy.selfReplacing || !copy.downloadUrl.empty();

    const auto spare = std::to_string(copies);
    const LogAttribute found[]{
        {"offered", copy.offered},
        {"delivery", delivery},
        {"from", copy.downloadUrl},
        {"copies", spare},
        {"package", copy.downloadUrl.empty() ? "none for this delivery" : "yes"},
        {"canBeOffered", actionable ? "yes" : "no"},
    };
    Logger::instance().log(actionable ? LogLevel::Info : LogLevel::Warning, kArea,
                           actionable ? "a newer version is available"
                                      : "a newer version exists but this build can do nothing "
                                        "with it, so nothing will be offered",
                           found);
}

void UpdateService::doFetch()
{
    std::vector<ReleasePackage> candidates;
    {
        const std::lock_guard guard{mutex_};
        if (!offered_) {
            return;
        }
        candidates = candidates_;
    }

    if (candidates.empty()) {
        report(UpdateState::Failed, "this release publishes nothing for this kind of installation");
        return;
    }

    std::error_code failure;
    std::filesystem::create_directories(workshop_, failure);
    if (failure) {
        report(UpdateState::Failed, "there is nowhere to download to: " + failure.message());
        return;
    }

    for (const auto &entry : std::filesystem::directory_iterator{workshop_, failure}) {
        std::error_code ignored;
        std::filesystem::remove_all(entry.path(), ignored);
    }

    report(UpdateState::Downloading, {});

    std::string complaint;
    for (const auto &package : candidates) {
        if (tryToDownload(package, complaint)) {
            return;
        }
    }

    report(UpdateState::Failed, complaint.empty() ? "the download did not finish" : complaint);
}

bool UpdateService::tryToDownload(const ReleasePackage &package, std::string &complaint)
{
    const auto landing = workshop_ / fileNameFrom(package.url, "ai-softphone-update");

    {
        const std::lock_guard guard{mutex_};
        status_.received = 0;
        status_.total = package.size;
    }

    const auto expected = std::to_string(package.size);
    const LogAttribute going[]{{"from", package.url}, {"bytes", expected}};
    Logger::instance().log(LogLevel::Info, kArea, "downloading the update", going);

    const auto began = std::chrono::steady_clock::now();

    const auto arrived = transport_.download(
        package.url, landing,
        [this, whole = package.size](std::uint64_t received, std::uint64_t total) {
            UpdateStatus copy;
            bool carryOn = false;
            {
                const std::lock_guard guard{mutex_};
                carryOn = !stopping_;
                status_.received = received;
                status_.total = total != 0 ? total : whole;
                copy = status_;
            }

            if (carryOn) {
                publish(copy);
            }
            return carryOn;
        },
        complaint);

    const auto giveUp = [&](std::string reason) {
        std::error_code ignored;
        std::filesystem::remove(landing, ignored);
        complaint = std::move(reason);

        const auto received = std::to_string(status().received);
        const auto spent = std::to_string(sinceInMilliseconds(began));
        const LogAttribute gaveUp[]{{"from", package.url},
                                    {"received", received},
                                    {"of", expected},
                                    {"ms", spent},
                                    {"why", complaint}};
        Logger::instance().log(LogLevel::Warning, kArea, "a download did not finish", gaveUp);
        return false;
    };

    if (!arrived) {
        return giveUp(complaint.empty() ? "the download did not finish" : complaint);
    }

    std::error_code failure;
    if (package.size != 0) {
        const auto size = std::filesystem::file_size(landing, failure);
        if (failure || size != package.size) {
            return giveUp("the download is not the size the manifest gave");
        }
    }

    const auto digest = signatures_.digest(landing);
    if (digest.empty() || digest != package.sha256) {
        return giveUp("the download does not match the digest the manifest signed");
    }

    if (channel_.delivery == Delivery::AppImage) {
        std::error_code ignored;
        std::filesystem::permissions(
            landing,
            std::filesystem::perms::owner_all | std::filesystem::perms::group_read |
                std::filesystem::perms::group_exec | std::filesystem::perms::others_read |
                std::filesystem::perms::others_exec,
            std::filesystem::perm_options::replace, ignored);
    }

    UpdateStatus copy;
    {
        const std::lock_guard guard{mutex_};
        status_.ready = landing;
        status_.downloadUrl = package.url;
        status_.state = UpdateState::Ready;
        status_.complaint.clear();
        copy = status_;
    }
    publish(copy);
    return true;
}

bool UpdateService::apply(std::string &complaint)
{
    complaint.clear();

    std::filesystem::path ready;
    InstallerLauncher launcher;
    {
        const std::lock_guard guard{mutex_};
        if (status_.state != UpdateState::Ready || status_.ready.empty()) {
            complaint = "there is nothing downloaded to install";
            return false;
        }
        ready = status_.ready;
        launcher = launcher_;
    }

    if (!channel_.selfReplacing()) {
        complaint = "this kind of installation is replaced by its package manager";
        return false;
    }

    if (channel_.delivery == Delivery::Installer) {
        if (!launcher) {
            complaint = "there is no way to start an installer on this build";
            return false;
        }

        if (!launcher(ready, complaint)) {
            if (complaint.empty()) {
                complaint = "the installer would not start";
            }
            return false;
        }

        {
            const std::lock_guard guard{mutex_};
            status_.ready.clear();
        }
        return true;
    }

    const auto target = channel_.running;
    const auto staging = target.parent_path() / (target.filename().string() + ".new");

    std::error_code failure;
    std::filesystem::copy_file(ready, staging, std::filesystem::copy_options::overwrite_existing,
                               failure);
    if (failure) {
        complaint = "the new version could not be written beside the old one: " + failure.message();
        return false;
    }

    std::filesystem::permissions(
        staging,
        std::filesystem::perms::owner_all | std::filesystem::perms::group_read |
            std::filesystem::perms::group_exec | std::filesystem::perms::others_read |
            std::filesystem::perms::others_exec,
        std::filesystem::perm_options::replace, failure);
    if (failure) {
        std::error_code ignored;
        std::filesystem::remove(staging, ignored);
        complaint = "the new version could not be made runnable: " + failure.message();
        return false;
    }

    std::filesystem::rename(staging, target, failure);
    if (failure) {
        std::error_code ignored;
        std::filesystem::remove(staging, ignored);
        complaint = "the new version could not take the old one's place: " + failure.message();
        return false;
    }

    std::error_code ignored;
    std::filesystem::remove(ready, ignored);

    {
        const std::lock_guard guard{mutex_};
        status_.ready.clear();
    }

    return true;
}

}
