// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/container/registry.hpp>

#include <algorithm>
#include <unordered_map>
#include <unordered_set>

namespace softphone::container {
namespace {

[[nodiscard]] std::vector<const Dependency *> everythingNamed(const Manifest &manifest)
{
    std::vector<const Dependency *> named;
    named.reserve(manifest.requirements.size() + manifest.suggestions.size());
    for (const auto &dependency : manifest.requirements) {
        named.push_back(&dependency);
    }
    for (const auto &dependency : manifest.suggestions) {
        named.push_back(&dependency);
    }
    return named;
}

[[nodiscard]] std::string ringDescription(std::span<const std::string> ring)
{
    std::string text;
    for (const auto &id : ring) {
        if (!text.empty()) {
            text += " → ";
        }
        text += id;
    }
    if (!ring.empty()) {
        text += " → " + ring.front();
    }
    return text;
}

}

std::string_view toString(LoadState state) noexcept
{
    switch (state) {
    case LoadState::Waiting:
        return "waiting";
    case LoadState::Ready:
        return "ready";
    case LoadState::Disabled:
        return "disabled";
    case LoadState::Blocked:
        return "blocked";
    case LoadState::Cycle:
        return "cycle";
    case LoadState::Failed:
        return "failed";
    }
    return "unknown";
}

std::string_view toString(AddProblem problem) noexcept
{
    switch (problem) {
    case AddProblem::None:
        return "none";
    case AddProblem::BadManifest:
        return "bad manifest";
    case AddProblem::DuplicateId:
        return "duplicate id";
    case AddProblem::AlreadyStarted:
        return "the container has already started";
    }
    return "unknown";
}

AddProblem PluginRegistry::add(Manifest manifest, IPlugin *plugin)
{
    if (!isValidPluginId(manifest.id)) {
        return AddProblem::BadManifest;
    }
    if (find(manifest.id) != nullptr) {
        return AddProblem::DuplicateId;
    }

    entries_.push_back(Entry{std::move(manifest), plugin, LoadState::Waiting, {}, {}});
    return AddProblem::None;
}

bool PluginRegistry::disable(std::string_view id)
{
    auto *const entry = find(id);
    if (entry == nullptr) {
        return false;
    }
    if (entry->manifest.required) {
        return false;
    }
    entry->state = LoadState::Disabled;
    return true;
}

const Entry *PluginRegistry::find(std::string_view id) const
{
    const auto found = std::ranges::find_if(
        entries_, [id](const Entry &entry) { return entry.manifest.id == id; });
    return found == entries_.end() ? nullptr : &*found;
}

Entry *PluginRegistry::find(std::string_view id)
{
    const auto found = std::ranges::find_if(
        entries_, [id](const Entry &entry) { return entry.manifest.id == id; });
    return found == entries_.end() ? nullptr : &*found;
}

void PluginRegistry::resolve()
{
    order_.clear();
    for (auto &entry : entries_) {
        if (entry.state == LoadState::Disabled) {
            continue;
        }
        entry.state = LoadState::Waiting;
        entry.problem.clear();
        entry.blockedBy.clear();
    }

    enum class Colour { White, Grey, Black };
    std::unordered_map<std::string, Colour> colour;
    for (const auto &entry : entries_) {
        colour[entry.manifest.id] = Colour::White;
    }

    std::vector<std::string> stack;
    const auto reachable = [this](std::string_view id) {
        const auto *const entry = find(id);
        return entry != nullptr && entry->state != LoadState::Disabled;
    };

    const auto walk = [&](auto &&self, const std::string &id) -> void {
        colour[id] = Colour::Grey;
        stack.push_back(id);

        const auto *const entry = find(id);
        if (entry != nullptr) {
            for (const auto *dependency : everythingNamed(entry->manifest)) {
                if (!reachable(dependency->id)) {
                    continue;
                }
                const auto state = colour[dependency->id];
                if (state == Colour::Grey) {
                    const auto from = std::ranges::find(stack, dependency->id);
                    const std::vector<std::string> ring{from, stack.end()};
                    const auto description = ringDescription(ring);
                    for (const auto &member : ring) {
                        auto *const inRing = find(member);
                        if (inRing != nullptr && inRing->state != LoadState::Disabled) {
                            inRing->state = LoadState::Cycle;
                            inRing->problem = "part of a dependency cycle: " + description;
                        }
                    }
                    continue;
                }
                if (state == Colour::White) {
                    self(self, dependency->id);
                }
            }
        }

        stack.pop_back();
        colour[id] = Colour::Black;
    };

    for (const auto &entry : entries_) {
        if (entry.state != LoadState::Disabled && colour[entry.manifest.id] == Colour::White) {
            walk(walk, entry.manifest.id);
        }
    }

    bool moved = true;
    while (moved) {
        moved = false;
        for (auto &entry : entries_) {
            if (entry.state != LoadState::Waiting) {
                continue;
            }

            for (const auto &requirement : entry.manifest.requirements) {
                const auto *const target = find(requirement.id);
                if (target == nullptr) {
                    entry.state = LoadState::Blocked;
                    entry.blockedBy.push_back(requirement.id);
                    entry.problem = "requires " + requirement.id + ", which is not installed";
                    break;
                }
                if (!requirement.versions.allows(target->manifest.version)) {
                    entry.state = LoadState::Blocked;
                    entry.blockedBy.push_back(requirement.id);
                    entry.problem = "requires " + requirement.id + " " +
                                    toString(requirement.versions) + ", and this is " +
                                    toString(target->manifest.version);
                    break;
                }
                if (target->state == LoadState::Disabled || target->state == LoadState::Blocked ||
                    target->state == LoadState::Cycle || target->state == LoadState::Failed) {
                    entry.state = LoadState::Blocked;
                    entry.blockedBy.push_back(requirement.id);
                    entry.problem = "requires " + requirement.id + ", which is " +
                                    std::string{toString(target->state)};
                    break;
                }
            }

            if (entry.state != LoadState::Waiting) {
                moved = true;
            }
        }
    }

    for (auto &entry : entries_) {
        if (entry.state == LoadState::Waiting) {
            entry.state = LoadState::Ready;
        }
    }

    std::unordered_set<std::string> placed;
    bool progressed = true;
    while (progressed) {
        progressed = false;
        for (const auto &entry : entries_) {
            if (entry.state != LoadState::Ready || placed.contains(entry.manifest.id)) {
                continue;
            }

            const auto satisfied =
                std::ranges::all_of(everythingNamed(entry.manifest), [&](const Dependency *d) {
                    const auto *const target = find(d->id);
                    if (target == nullptr || target->state != LoadState::Ready) {
                        return true;
                    }
                    return placed.contains(d->id);
                });
            if (!satisfied) {
                continue;
            }

            order_.push_back(entry.manifest.id);
            placed.insert(entry.manifest.id);
            progressed = true;
        }
    }
}

void PluginRegistry::markFailed(std::string_view id, std::string problem)
{
    auto *const entry = find(id);
    if (entry == nullptr) {
        return;
    }

    entry->state = LoadState::Failed;
    entry->problem = std::move(problem);
    blockDependentsOf(id);
}

bool PluginRegistry::attach(std::string_view id, IPlugin *plugin)
{
    auto *const entry = find(id);
    if (entry == nullptr || plugin == nullptr || entry->plugin != nullptr) {
        return false;
    }

    entry->plugin = plugin;
    return true;
}

bool PluginRegistry::enable(std::string_view id)
{
    auto *const entry = find(id);
    if (entry == nullptr || entry->state != LoadState::Disabled) {
        return false;
    }

    entry->state = LoadState::Waiting;
    entry->problem.clear();
    entry->blockedBy.clear();
    return true;
}

bool PluginRegistry::requirementsReady(std::string_view id) const
{
    const auto *const entry = find(id);
    if (entry == nullptr) {
        return false;
    }

    return std::ranges::all_of(entry->manifest.requirements, [this](const Dependency &wanted) {
        const auto *const other = find(wanted.id);
        return other != nullptr && other->state == LoadState::Ready &&
               wanted.versions.allows(other->manifest.version);
    });
}

std::vector<std::string> PluginRegistry::requirementsOf(std::string_view id) const
{
    std::vector<std::string> found;

    const auto walk = [this, &found](std::string_view wanted, auto &self) -> void {
        const auto *const entry = find(wanted);
        if (entry == nullptr) {
            return;
        }
        for (const auto &requirement : entry->manifest.requirements) {
            if (std::ranges::find(found, requirement.id) != found.end()) {
                continue;
            }
            self(requirement.id, self);
            if (find(requirement.id) != nullptr &&
                std::ranges::find(found, requirement.id) == found.end()) {
                found.push_back(requirement.id);
            }
        }
    };
    walk(id, walk);
    return found;
}

std::vector<std::string> PluginRegistry::dependentsOf(std::string_view id) const
{
    std::vector<std::string> found;

    std::vector<std::string> frontier{std::string{id}};
    while (!frontier.empty()) {
        const auto wanted = std::move(frontier.back());
        frontier.pop_back();

        for (const auto &entry : entries_) {
            if (entry.manifest.id == id) {
                continue;
            }
            if (std::ranges::find(found, entry.manifest.id) != found.end()) {
                continue;
            }
            const auto requiresIt =
                std::ranges::any_of(entry.manifest.requirements,
                                    [&wanted](const Dependency &d) { return d.id == wanted; });
            if (!requiresIt) {
                continue;
            }
            found.push_back(entry.manifest.id);
            frontier.push_back(entry.manifest.id);
        }
    }

    std::ranges::sort(found);
    return found;
}

std::vector<std::string> PluginRegistry::suggestersOf(std::string_view id) const
{
    std::vector<std::string> found;
    for (const auto &entry : entries_) {
        if (entry.manifest.id == id || entry.state != LoadState::Ready) {
            continue;
        }
        const auto suggests = std::ranges::any_of(
            entry.manifest.suggestions, [id](const Dependency &one) { return one.id == id; });
        if (suggests) {
            found.push_back(entry.manifest.id);
        }
    }

    std::ranges::sort(found);
    return found;
}

void PluginRegistry::blockDependentsOf(std::string_view id)
{
    for (auto &entry : entries_) {
        if (entry.state != LoadState::Ready && entry.state != LoadState::Waiting) {
            continue;
        }

        const auto requiresIt = std::ranges::any_of(
            entry.manifest.requirements, [id](const Dependency &d) { return d.id == id; });
        if (!requiresIt) {
            continue;
        }

        entry.state = LoadState::Blocked;
        entry.blockedBy.emplace_back(id);
        entry.problem = "requires " + std::string{id} + ", which failed";

        blockDependentsOf(entry.manifest.id);
    }
}

}
