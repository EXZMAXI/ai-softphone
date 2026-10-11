// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/container/host.hpp>

#include <algorithm>
#include <ranges>
#include <utility>

namespace softphone::container {

Host::Host(LogSink log) : log_{std::move(log)} {}

Host::Host(LogSink log, std::filesystem::path settingsFile, FileProtector protect,
           std::string adoptOlderDocumentsAs)
    : log_{std::move(log)},
      settings_{std::make_unique<Settings>(std::move(settingsFile), std::move(protect))}
{
    if (!adoptOlderDocumentsAs.empty()) {
        settings_->adoptOlderDocumentsAs(std::move(adoptOlderDocumentsAs));
    }
    settingsStatus_ = settings_->load();
}

Host::~Host()
{
    leave();
}

void Host::write(Severity severity, std::string_view category, std::string_view message) const
{
    if (log_) {
        log_(severity, category, message);
    }
}

AddProblem Host::add(Manifest manifest, IPlugin *plugin)
{

    return registry_.add(std::move(manifest), plugin);
}

bool Host::attach(std::string_view id, IPlugin *plugin)
{
    return registry_.attach(id, plugin);
}

bool Host::disable(std::string_view id)
{
    if (started_once_) {
        return false;
    }
    return registry_.disable(id);
}

Host::StartReport Host::start()
{
    started_once_ = true;

    if (settings_ != nullptr) {
        for (const auto &id : settings_->disabledPlugins()) {
            static_cast<void>(registry_.disable(id));
        }
    }

    registry_.resolve();

    watcher_ = bus_.watch([this](const Trace &trace) {
        std::string line{trace.name};
        line += " v" + std::to_string(trace.version) + " from " + std::string{trace.publisher} +
                " to " + std::to_string(trace.subscribers) + " subscriber(s), #" +
                std::to_string(trace.correlation);
        write(Severity::Trace, "bus", line);
    });

    for (const auto &id : registry_.order()) {
        auto *const entry = registry_.find(id);
        if (entry == nullptr || entry->state != LoadState::Ready) {

            continue;
        }
        if (entry->plugin == nullptr) {
            registry_.markFailed(id, "no code behind the manifest");
            continue;
        }

        auto context =
            std::make_unique<PluginContext>(entry->manifest, bus_, services_, settings_.get());
        std::string problem;
        if (!entry->plugin->initialize(*context, problem)) {
            services_.withdraw(id);
            registry_.markFailed(id, problem.empty() ? "would not initialise" : problem);
            continue;
        }

        contexts_.push_back(std::move(context));
        started_.push_back(id);
    }

    for (std::size_t index = 0; index < started_.size();) {
        const auto &id = started_[index];
        auto *const entry = registry_.find(id);
        if (entry == nullptr || entry->plugin == nullptr) {
            ++index;
            continue;
        }
        if (entry->state != LoadState::Ready) {
            ++index;
            continue;
        }

        std::string problem;
        if (!entry->plugin->ready(*contexts_[index], problem)) {
            entry->plugin->leaving();
            services_.withdraw(id);
            registry_.markFailed(id, problem.empty() ? "would not become ready" : problem);
            contexts_.erase(contexts_.begin() + static_cast<std::ptrdiff_t>(index));
            started_.erase(started_.begin() + static_cast<std::ptrdiff_t>(index));
            continue;
        }
        ++index;
    }

    StartReport report;
    for (const auto &entry : registry_.entries()) {
        switch (entry.state) {
        case LoadState::Ready:
            ++report.started;
            break;
        case LoadState::Disabled:
            ++report.disabled;
            break;
        case LoadState::Blocked:
            ++report.blocked;
            break;
        case LoadState::Failed:
            ++report.failed;
            break;
        case LoadState::Cycle:
            ++report.cycled;
            break;
        case LoadState::Waiting:
            break;
        }
    }

    for (const auto &line : describe()) {
        write(report.clean() ? Severity::Info : Severity::Warning, "container", line);
    }

    return report;
}

Host::LiveChange Host::activate(std::string_view id)
{
    LiveChange change;

    auto *const entry = registry_.find(id);
    if (entry == nullptr) {
        change.problem = "there is no plugin called " + std::string{id};
        return change;
    }
    if (entry->state == LoadState::Ready) {

        return change;
    }
    if (entry->plugin == nullptr) {
        change.problem = "there is no code behind its manifest";
        return change;
    }
    if (entry->state == LoadState::Failed || entry->state == LoadState::Cycle) {
        change.problem = entry->problem.empty() ? "it did not start" : entry->problem;
        return change;
    }
    if (!registry_.requirementsReady(id)) {
        change.problem = "something it needs is not running";
        return change;
    }

    static_cast<void>(registry_.enable(id));

    auto context =
        std::make_unique<PluginContext>(entry->manifest, bus_, services_, settings_.get());
    std::string problem;
    if (!entry->plugin->initialize(*context, problem)) {
        services_.withdraw(id);
        registry_.markFailed(id, problem.empty() ? "would not initialise" : problem);
        change.problem = problem.empty() ? "it would not initialise" : problem;
        return change;
    }

    if (!entry->plugin->ready(*context, problem)) {
        entry->plugin->leaving();
        services_.withdraw(id);
        registry_.markFailed(id, problem.empty() ? "would not become ready" : problem);
        change.problem = problem.empty() ? "it would not become ready" : problem;
        return change;
    }

    entry->state = LoadState::Ready;
    contexts_.push_back(std::move(context));
    started_.emplace_back(id);
    change.changed.emplace_back(id);

    for (const auto &other : registry_.entries()) {
        if (other.state != LoadState::Ready || other.manifest.id == id || other.plugin == nullptr) {
            continue;
        }
        const auto suggested = std::ranges::any_of(
            other.manifest.suggestions, [id](const Dependency &one) { return one.id == id; });
        if (!suggested) {
            continue;
        }

        const auto at = std::ranges::find(started_, other.manifest.id);
        if (at == started_.end()) {
            continue;
        }
        const auto which = static_cast<std::size_t>(std::distance(started_.begin(), at));

        std::string ignored;
        if (!other.plugin->ready(*contexts_[which], ignored)) {

            write(Severity::Warning, "container",
                  other.manifest.id + " could not take up " + std::string{id} + ": " + ignored);
        }
    }

    write(Severity::Info, "container", std::string{id} + " is on");
    return change;
}

Host::LiveChange Host::deactivate(std::string_view id)
{
    LiveChange change;

    auto *const entry = registry_.find(id);
    if (entry == nullptr) {
        change.problem = "there is no plugin called " + std::string{id};
        return change;
    }
    if (entry->manifest.required) {
        change.problem = "it cannot be turned off";
        return change;
    }

    auto going = registry_.dependentsOf(id);
    going.emplace_back(id);

    std::vector<std::string> order;
    for (auto walk = started_.rbegin(); walk != started_.rend(); ++walk) {
        if (std::ranges::find(going, *walk) != going.end()) {
            order.push_back(*walk);
        }
    }

    for (auto walk = order.begin(); walk != order.end(); ++walk) {
        auto *const doomed = registry_.find(*walk);
        if (doomed == nullptr || doomed->state != LoadState::Ready) {
            continue;
        }

        if (doomed->plugin != nullptr) {

            doomed->plugin->leaving();
        }
        services_.withdraw(*walk);

        const auto at = std::ranges::find(started_, *walk);
        if (at != started_.end()) {
            const auto index = static_cast<std::ptrdiff_t>(std::distance(started_.begin(), at));
            contexts_.erase(contexts_.begin() + index);
            started_.erase(at);
        }

        doomed->state = LoadState::Disabled;
        doomed->problem.clear();
        doomed->blockedBy.clear();
        change.changed.push_back(*walk);
    }

    if (!change.changed.empty()) {
        std::string line = "off:";
        for (const auto &gone : change.changed) {
            line += " " + gone;
        }
        write(Severity::Info, "container", line);
    }
    return change;
}

void Host::leave() noexcept
{
    if (left_) {
        return;
    }
    left_ = true;

    for (auto index = started_.size(); index > 0; --index) {
        const auto &id = started_[index - 1];
        auto *const entry = registry_.find(id);
        if (entry != nullptr && entry->plugin != nullptr) {
            entry->plugin->leaving();
        }
        services_.withdraw(id);
    }

    started_.clear();
    contexts_.clear();

    if (watcher_ != EventBus::kNoToken) {
        bus_.unwatch(watcher_);
        watcher_ = EventBus::kNoToken;
    }
}

std::vector<std::string> Host::describe() const
{
    std::vector<std::string> lines;
    lines.reserve(registry_.entries().size());

    std::vector<const Entry *> ordered;
    ordered.reserve(registry_.entries().size());
    for (const auto &id : registry_.order()) {
        if (const auto *found = registry_.find(id); found != nullptr) {
            ordered.push_back(found);
        }
    }
    for (const auto &entry : registry_.entries()) {
        if (entry.state != LoadState::Ready) {
            ordered.push_back(&entry);
        }
    }

    for (const auto &entry :
         ordered | std::views::transform([](const Entry *e) -> const Entry & { return *e; })) {
        std::string line = entry.manifest.id + " " + toString(entry.manifest.version) + " — " +
                           std::string{toString(entry.state)};
        if (!entry.problem.empty()) {
            line += ": " + entry.problem;
        }
        lines.push_back(std::move(line));
    }
    return lines;
}

}
