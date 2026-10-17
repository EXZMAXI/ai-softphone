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

#include <softphone/core/version.hpp>

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace softphone::core {

enum class Delivery {
    Source,
    Deb,
    AppImage,
    Installer,
    Dmg,
};

[[nodiscard]] std::string_view toString(Delivery delivery) noexcept;
[[nodiscard]] std::optional<Delivery> parseDelivery(std::string_view text) noexcept;

struct UpdateChannel {

    std::vector<std::string> manifestUrls;

    std::map<std::string, std::vector<std::string>> manifestUrlsByCountry;

    [[nodiscard]] std::vector<std::string> hostsFor(const std::string &country) const;

    [[nodiscard]] std::vector<std::string> downloadHostsFor(const std::string &country) const;

    std::string publicKey;
    Delivery delivery{Delivery::Source};

    std::filesystem::path running;

    [[nodiscard]] bool configured() const noexcept
    {
        return !manifestUrls.empty() && !publicKey.empty();
    }

    [[nodiscard]] bool selfReplacing() const noexcept
    {
        switch (delivery) {
        case Delivery::AppImage:
            return !running.empty();
        case Delivery::Installer:
            return true;
        case Delivery::Source:
        case Delivery::Deb:
        case Delivery::Dmg:

            return false;
        }
        return false;
    }
};

[[nodiscard]] UpdateChannel buildChannel();

[[nodiscard]] bool runInstaller(const std::filesystem::path &installer, std::string &complaint);

struct ReleasePackage {

    std::string format;
    std::string url;
    std::uint64_t size{};

    std::string sha256;
};

struct Release {
    Version version;

    std::string versionText;

    std::string notes;

    std::vector<ReleasePackage> packages;

    [[nodiscard]] const ReleasePackage *packageFor(Delivery delivery) const noexcept;
};

[[nodiscard]] std::optional<Version> parseVersion(std::string_view text) noexcept;

[[nodiscard]] bool isNewer(const Version &offered, const Version &running) noexcept;

[[nodiscard]] std::optional<Release> readManifest(std::string_view document,
                                                  std::string &complaint);

struct Fetched {
    bool ok{false};
    int status{0};
    std::string body;
    std::string error;
};

class IUpdateTransport {
public:
    virtual ~IUpdateTransport() = default;

    virtual Fetched fetch(const std::string &url) = 0;

    virtual bool download(const std::string &url, const std::filesystem::path &into,
                          const std::function<bool(std::uint64_t, std::uint64_t)> &progress,
                          std::string &complaint) = 0;
};

class IUpdateSignatures {
public:
    virtual ~IUpdateSignatures() = default;

    [[nodiscard]] virtual bool verify(std::string_view document, std::string_view signature,
                                      std::string_view publicKey) = 0;

    [[nodiscard]] virtual std::string digest(const std::filesystem::path &file) = 0;
};

enum class UpdateState {

    Unconfigured,
    Idle,
    Checking,
    UpToDate,
    Available,
    Downloading,

    Ready,
    Failed,
};

[[nodiscard]] std::string_view toString(UpdateState state) noexcept;

struct UpdateStatus {
    UpdateState state{UpdateState::Unconfigured};

    std::string offered;
    std::string notes;

    std::string downloadUrl;

    bool selfReplacing{false};

    std::uint64_t received{};
    std::uint64_t total{};

    std::string complaint;

    std::filesystem::path ready;

    std::chrono::system_clock::time_point checkedAt{};
};

class UpdateService {
public:
    UpdateService(UpdateChannel channel, Version running, IUpdateTransport &transport,
                  IUpdateSignatures &signatures, std::filesystem::path workshop);
    ~UpdateService();

    UpdateService(const UpdateService &) = delete;
    UpdateService &operator=(const UpdateService &) = delete;

    void observe(std::function<void(const UpdateStatus &)> watcher);

    [[nodiscard]] const UpdateChannel &channel() const noexcept
    {
        return channel_;
    }

    [[nodiscard]] UpdateStatus status() const;

    void setCountry(std::string country);

    void check();

    void fetch();

    [[nodiscard]] bool apply(std::string &complaint);

    using InstallerLauncher =
        std::function<bool(const std::filesystem::path &installer, std::string &complaint)>;

    void installWith(InstallerLauncher launcher);

private:
    enum class Job {
        Check,
        Fetch,
    };

    void work();
    void doCheck();
    void doFetch();

    [[nodiscard]] std::optional<Release> manifestFrom(const std::string &url,
                                                      std::string &complaint);

    [[nodiscard]] bool tryToDownload(const ReleasePackage &package, std::string &complaint);
    void publish(const UpdateStatus &status);
    void report(UpdateState state, const std::string &complaint);

    void logTheState(UpdateState state, const UpdateStatus &copy) const;

    void sayWhatWasFound(const UpdateStatus &copy, std::size_t copies) const;

    UpdateChannel channel_;

    std::string country_;
    Version running_;
    IUpdateTransport &transport_;
    IUpdateSignatures &signatures_;
    std::filesystem::path workshop_;

    mutable std::mutex mutex_;

    mutable std::mutex watcherMutex_;
    std::condition_variable wakeup_;
    std::deque<Job> jobs_;
    bool stopping_{false};
    UpdateStatus status_;
    std::optional<Release> offered_;

    std::vector<ReleasePackage> candidates_;
    std::function<void(const UpdateStatus &)> watcher_;

    std::thread worker_;

    InstallerLauncher launcher_;
};

}
