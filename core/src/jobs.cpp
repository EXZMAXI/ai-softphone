// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/jobs.hpp>

#include <softphone/core/logging.hpp>

#include <algorithm>
#include <chrono>
#include <string>
#include <utility>

namespace softphone::core {
namespace {

using namespace std::chrono_literals;

constexpr auto kDrainStep = 5ms;

constexpr std::string_view kArea{"jobs"};

}

std::string_view toString(JobKind kind) noexcept
{
    switch (kind) {
    case JobKind::Transcribe:
        return "transcribe";
    case JobKind::Generate:
        return "generate";
    case JobKind::Delete:
        return "delete";
    }
    return "transcribe";
}

std::optional<JobKind> parseJobKind(std::string_view text) noexcept
{
    if (text == "transcribe") {
        return JobKind::Transcribe;
    }
    if (text == "generate") {
        return JobKind::Generate;
    }
    if (text == "delete") {
        return JobKind::Delete;
    }
    return std::nullopt;
}

std::string_view toString(JobState state) noexcept
{
    switch (state) {
    case JobState::Queued:
        return "queued";
    case JobState::Running:
        return "running";
    case JobState::Done:
        return "done";
    case JobState::Failed:
        return "failed";
    case JobState::Cancelled:
        return "cancelled";
    }
    return "queued";
}

std::optional<JobState> parseJobState(std::string_view text) noexcept
{
    if (text == "queued") {
        return JobState::Queued;
    }
    if (text == "running") {
        return JobState::Running;
    }
    if (text == "done") {
        return JobState::Done;
    }
    if (text == "failed") {
        return JobState::Failed;
    }
    if (text == "cancelled") {
        return JobState::Cancelled;
    }
    return std::nullopt;
}

JobRunner::JobRunner(IJobStore &store, JobPolicy policy, WallClock clock, JitterSource jitter)
    : store_{store}, policy_{policy}, clock_{clock ? std::move(clock) : defaultWallClock()},
      jitter_{jitter ? std::move(jitter) : defaultJitterSource()}
{
    if (policy_.workers == 0) {
        policy_.workers = 1;
    }
    if (policy_.attempts == 0) {
        policy_.attempts = 1;
    }
}

JobRunner::~JobRunner()
{
    stop();
}

void JobRunner::handle(JobKind kind, Handler handler)
{
    handlers_[kind] = std::move(handler);
}

void JobRunner::watch(Watcher watcher)
{
    watcher_ = std::move(watcher);
}

void JobRunner::start()
{
    {
        const std::lock_guard<std::mutex> held{mutex_};
        if (!workers_.empty()) {
            return;
        }
        stopping_ = false;
    }

    std::size_t revived = 0;
    if (const auto status = store_.reviveRunning(revived); !succeeded(status)) {
        const LogAttribute fields[] = {{"status", toString(status)}};
        Logger::instance().log(LogLevel::Warning, kArea, "the job queue could not be read at start",
                               fields);
    } else if (revived > 0) {
        const auto count = std::to_string(revived);
        const LogAttribute fields[] = {{"jobs", count}};
        Logger::instance().log(LogLevel::Info, kArea,
                               "jobs left running by a previous start are queued again", fields);
    }

    const std::lock_guard<std::mutex> held{mutex_};
    workers_.reserve(policy_.workers);
    for (unsigned index = 0; index < policy_.workers; ++index) {
        workers_.emplace_back([this] { work(); });
    }
}

void JobRunner::stop()
{
    std::vector<std::thread> leaving;
    {
        const std::lock_guard<std::mutex> held{mutex_};
        if (workers_.empty()) {
            return;
        }
        stopping_ = true;
        leaving.swap(workers_);
    }
    work_.notify_all();
    for (auto &worker : leaving) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

StoreStatus JobRunner::post(Job &job)
{
    job.state = JobState::Queued;
    const auto now = clock_();
    if (job.createdAt.time_since_epoch().count() == 0) {
        job.createdAt = now;
    }
    job.updatedAt = now;

    const auto status = store_.save(job);
    if (succeeded(status)) {
        announce(job);
        wake();
    }
    return status;
}

void JobRunner::wake()
{
    work_.notify_all();
}

StoreStatus JobRunner::cancel(const std::string &id)
{
    {
        const std::lock_guard<std::mutex> held{mutex_};
        if (std::find(cancelling_.begin(), cancelling_.end(), id) == cancelling_.end()) {
            cancelling_.push_back(id);
        }
    }
    const auto status = store_.cancelJob(id);
    wake();
    return status;
}

bool JobRunner::drain(std::chrono::milliseconds limit)
{
    const auto deadline = std::chrono::steady_clock::now() + limit;

    std::unique_lock<std::mutex> held{mutex_};

    if (workers_.empty()) {
        held.unlock();
        JobQuery query;
        query.state = JobState::Queued;
        query.newestFirst = false;
        query.limit = 32;
        std::vector<Job> waiting;
        if (const auto status = store_.jobs(query, waiting); !succeeded(status)) {
            return true;
        }
        const auto now = clock_();
        return std::none_of(waiting.begin(), waiting.end(),
                            [now](const Job &job) { return job.nextAttemptAt <= now; });
    }

    const auto mark = looked_;
    for (;;) {

        work_.notify_all();
        if (idle_.wait_for(held, kDrainStep,
                           [this, mark] { return busy_ == 0 && looked_ > mark; })) {
            return true;
        }
        if (std::chrono::steady_clock::now() >= deadline) {
            return false;
        }
    }
}

JobRunner::Tally JobRunner::tally() const
{
    const std::lock_guard<std::mutex> held{mutex_};
    auto copy = tally_;
    copy.running = busy_;
    return copy;
}

void JobRunner::work()
{
    for (;;) {
        {
            const std::lock_guard<std::mutex> held{mutex_};
            if (stopping_) {
                return;
            }
        }
        if (takeOne()) {
            continue;
        }
        std::unique_lock<std::mutex> held{mutex_};
        if (stopping_) {
            return;
        }
        work_.wait_for(held, policy_.idle);
    }
}

bool JobRunner::takeOne()
{

    {
        const std::lock_guard<std::mutex> held{mutex_};
        ++busy_;
    }

    Job job;
    const auto status = store_.claim(clock_(), job);
    if (!succeeded(status)) {
        {
            const std::lock_guard<std::mutex> held{mutex_};
            --busy_;
            if (status == StoreStatus::Missing) {

                ++looked_;
            }
        }
        idle_.notify_all();
        if (status != StoreStatus::Missing) {
            const LogAttribute fields[] = {{"status", toString(status)}};
            Logger::instance().log(LogLevel::Warning, kArea, "a job could not be claimed", fields);
        }
        return false;
    }

    {
        const std::lock_guard<std::mutex> held{mutex_};
        ++tally_.claimed;
    }
    announce(job);

    JobOutcome outcome;
    const auto handler = handlers_.find(job.kind);
    if (handler == handlers_.end()) {
        outcome.error = "no handler for this kind of job";
        const std::lock_guard<std::mutex> held{mutex_};
        ++tally_.unhandled;
    } else {
        const auto id = job.id;
        const Cancelled cancelled = [this, id] {
            const std::lock_guard<std::mutex> held{mutex_};
            return stopping_ ||
                   std::find(cancelling_.begin(), cancelling_.end(), id) != cancelling_.end();
        };

        try {
            outcome = handler->second(job, cancelled);
        } catch (const std::exception &thrown) {
            outcome = JobOutcome{};
            outcome.error =
                std::string{"this job stopped with an error nobody expected: "} + thrown.what();
            const LogAttribute fields[] = {{"job", job.id}, {"kind", toString(job.kind)}};
            Logger::instance().log(LogLevel::Error, kArea, outcome.error, fields);
            const std::lock_guard<std::mutex> held{mutex_};
            ++tally_.threw;
        } catch (...) {
            outcome = JobOutcome{};
            outcome.error = "this job stopped with an error nobody expected";
            const LogAttribute fields[] = {{"job", job.id}, {"kind", toString(job.kind)}};
            Logger::instance().log(LogLevel::Error, kArea, outcome.error, fields);
            const std::lock_guard<std::mutex> held{mutex_};
            ++tally_.threw;
        }
    }

    finish(job, outcome);

    {
        const std::lock_guard<std::mutex> held{mutex_};
        --busy_;
    }
    idle_.notify_all();
    return true;
}

void JobRunner::finish(Job &job, const JobOutcome &outcome)
{
    bool asked = false;
    bool leaving = false;
    {
        const std::lock_guard<std::mutex> held{mutex_};
        leaving = stopping_;
        const auto at = std::find(cancelling_.begin(), cancelling_.end(), job.id);
        asked = at != cancelling_.end();
        if (asked) {
            cancelling_.erase(at);
        }
    }

    job.costMicros += outcome.costMicros;
    job.updatedAt = clock_();

    if (outcome.done) {
        job.state = JobState::Done;
        job.error.clear();
        const std::lock_guard<std::mutex> held{mutex_};
        ++tally_.done;
    } else if (asked) {
        job.state = JobState::Cancelled;
        job.error = outcome.error;
        const std::lock_guard<std::mutex> held{mutex_};
        ++tally_.cancelled;
    } else if (leaving) {

        job.state = JobState::Queued;
        if (job.attempts > 0) {
            --job.attempts;
        }
        job.nextAttemptAt = {};
    } else if (outcome.worthRetrying && job.attempts < policy_.attempts) {
        job.state = JobState::Queued;
        job.error = outcome.error;
        job.nextAttemptAt =
            clock_() + backoffDelay(policy_.backoff, job.attempts > 0 ? job.attempts - 1 : 0,
                                    jitter_ ? jitter_() : 0.0);
        const std::lock_guard<std::mutex> held{mutex_};
        ++tally_.retried;
    } else {
        job.state = JobState::Failed;
        job.error = outcome.error;
        const std::lock_guard<std::mutex> held{mutex_};
        ++tally_.failed;
    }

    if (const auto status = store_.save(job); !succeeded(status)) {
        const LogAttribute fields[] = {{"job", job.id}, {"status", toString(status)}};
        Logger::instance().log(LogLevel::Warning, kArea, "a finished job could not be written back",
                               fields);
    }
    announce(job);
}

void JobRunner::announce(const Job &job)
{
    if (watcher_) {
        watcher_(job);
    }
}

}
