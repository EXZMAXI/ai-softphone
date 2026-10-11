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

#include <softphone/core/backoff.hpp>
#include <softphone/core/call_log.hpp>
#include <softphone/core/store.hpp>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace softphone::core {

enum class JobKind {

    Transcribe,

    Generate,

    Delete,
};

[[nodiscard]] std::string_view toString(JobKind kind) noexcept;
[[nodiscard]] std::optional<JobKind> parseJobKind(std::string_view text) noexcept;

enum class JobState {
    Queued,
    Running,
    Done,

    Failed,
    Cancelled,
};

[[nodiscard]] std::string_view toString(JobState state) noexcept;
[[nodiscard]] std::optional<JobState> parseJobState(std::string_view text) noexcept;

[[nodiscard]] constexpr bool finished(JobState state) noexcept
{
    return state == JobState::Done || state == JobState::Failed || state == JobState::Cancelled;
}

struct Job {
    std::string id;
    JobKind kind{JobKind::Transcribe};

    std::string recordingId;
    std::string transcriptId;
    std::string providerId;
    std::string templateId;
    std::string automationId;

    JobState state{JobState::Queued};

    unsigned attempts{0};

    std::chrono::system_clock::time_point nextAttemptAt{};

    std::string error;

    int priority{0};

    std::string params;

    std::int64_t costMicros{0};

    std::chrono::system_clock::time_point createdAt{};
    std::chrono::system_clock::time_point updatedAt{};
};

struct JobQuery {

    std::optional<JobKind> kind;
    std::optional<JobState> state;
    std::string recordingId;

    bool newestFirst{true};
    std::size_t limit{200};
};

class IJobStore {
public:
    IJobStore() = default;
    IJobStore(const IJobStore &) = delete;
    IJobStore &operator=(const IJobStore &) = delete;
    IJobStore(IJobStore &&) = delete;
    IJobStore &operator=(IJobStore &&) = delete;
    virtual ~IJobStore() = default;

    [[nodiscard]] virtual StoreStatus save(Job &job) = 0;

    [[nodiscard]] virtual StoreStatus job(const std::string &id, Job &out) = 0;
    [[nodiscard]] virtual StoreStatus jobs(const JobQuery &query, std::vector<Job> &out) = 0;

    [[nodiscard]] virtual StoreStatus claim(std::chrono::system_clock::time_point now,
                                            Job &out) = 0;

    [[nodiscard]] virtual StoreStatus reviveRunning(std::size_t &revived) = 0;

    [[nodiscard]] virtual StoreStatus cancelJob(const std::string &id) = 0;

    [[nodiscard]] virtual StoreStatus forgetDone(std::chrono::system_clock::time_point before,
                                                 std::size_t &removed) = 0;

    [[nodiscard]] virtual StoreStatus forgetJob(const std::string &id) = 0;
};

struct JobOutcome {
    bool done{false};

    bool worthRetrying{false};

    std::string error;
    std::int64_t costMicros{0};
};

struct JobPolicy {

    unsigned workers{2};

    unsigned attempts{3};

    BackoffPolicy backoff{std::chrono::seconds{30}, std::chrono::minutes{15}, 3.0, 0.2};

    std::chrono::milliseconds idle{std::chrono::seconds{2}};
};

class JobRunner {
public:

    using Cancelled = std::function<bool()>;

    using Handler = std::function<JobOutcome(const Job &, const Cancelled &)>;

    using Watcher = std::function<void(const Job &)>;

    JobRunner(IJobStore &store, JobPolicy policy = {}, WallClock clock = defaultWallClock(),
              JitterSource jitter = {});
    JobRunner(const JobRunner &) = delete;
    JobRunner &operator=(const JobRunner &) = delete;
    JobRunner(JobRunner &&) = delete;
    JobRunner &operator=(JobRunner &&) = delete;
    ~JobRunner();

    void handle(JobKind kind, Handler handler);

    void watch(Watcher watcher);

    void start();

    void stop();

    [[nodiscard]] StoreStatus post(Job &job);

    void wake();

    [[nodiscard]] StoreStatus cancel(const std::string &id);

    [[nodiscard]] bool drain(std::chrono::milliseconds limit);

    struct Tally {
        std::size_t claimed{0};
        std::size_t done{0};
        std::size_t failed{0};
        std::size_t retried{0};
        std::size_t cancelled{0};

        std::size_t unhandled{0};

        std::size_t running{0};

        std::size_t threw{0};
    };

    [[nodiscard]] Tally tally() const;

private:
    void work();
    [[nodiscard]] bool takeOne();
    void finish(Job &job, const JobOutcome &outcome);
    void announce(const Job &job);

    IJobStore &store_;
    JobPolicy policy_;
    WallClock clock_;
    JitterSource jitter_;

    std::map<JobKind, Handler> handlers_;
    Watcher watcher_;

    mutable std::mutex mutex_;
    std::condition_variable work_;
    std::condition_variable idle_;
    bool stopping_{false};
    unsigned busy_{0};
    Tally tally_;

    std::uint64_t looked_{0};

    std::vector<std::string> cancelling_;

    std::vector<std::thread> workers_;
};

}
