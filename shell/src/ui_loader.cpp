// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/shell/ui_loader.hpp>

#include <softphone/container/loader.hpp>
#include <softphone/container/loading.hpp>

#include <QString>

#include <algorithm>
#include <cstdint>
#include <string>
#include <system_error>
#include <utility>

namespace softphone::shell {

std::string uiLibraryFileName(std::string_view id)
{
    auto name = container::libraryFileName(id);

    const auto dot = name.find('.', name.rfind('/') + 1);
    return dot == std::string::npos ? name + "_ui" : name.insert(dot, "_ui");
}

UiLoader::UiLoader(container::LogSink log) : log_{std::move(log)} {}

UiLoader::~UiLoader()
{
    unload();
}

void UiLoader::write(container::Severity severity, std::string_view message) const
{
    if (log_) {
        log_(severity, "plugins", message);
    }
}

void UiLoader::offerBuiltIn(std::string id, Contribution contribute)
{
    builtIn_.emplace_back(std::move(id), std::move(contribute));
}

UiLoader::Opened *UiLoader::findOpen(std::string_view id)
{
    const auto found = std::ranges::find(opened_, id, &Opened::id);
    return found == opened_.end() ? nullptr : &*found;
}

UiLoader::Trouble UiLoader::contribute(const std::filesystem::path &directory, std::string_view id,
                                       UiRegistry &registry)
{
    registry_ = &registry;

    if (auto *const already = findOpen(id); already != nullptr) {
        if (already->showing) {
            return {};
        }

        if (already->handle == nullptr) {
            const auto linked = std::ranges::find(builtIn_, already->id,
                                                  &std::pair<std::string, Contribution>::first);
            if (linked != builtIn_.end()) {
                linked->second(registry);
            }
        } else {
            const auto ask = reinterpret_cast<UiEntryPoint>(
                container::dynamic::symbol(already->handle, kUiEntryPoint));
            if (ask == nullptr || ask() == nullptr || ask()->contribute == nullptr) {
                return {Refused::NotAPlugin, "its interface would not answer a second time"};
            }
            ask()->contribute(registry);
        }
        already->showing = true;
        return {};
    }

    const auto linked =
        std::ranges::find(builtIn_, std::string{id}, &std::pair<std::string, Contribution>::first);
    if (linked != builtIn_.end()) {
        linked->second(registry);
        opened_.push_back({std::string{id}, nullptr, true});
        return {};
    }

    const auto library = directory / id / uiLibraryFileName(id);

    std::error_code missing;
    if (!std::filesystem::exists(library, missing)) {

        return {};
    }

    void *const handle = container::dynamic::open(library);
    if (handle == nullptr) {
        return {Refused::CannotOpen, container::dynamic::whyItWouldNotOpen()};
    }

    const auto ask =
        reinterpret_cast<UiEntryPoint>(container::dynamic::symbol(handle, kUiEntryPoint));
    if (ask == nullptr) {
        container::dynamic::close(handle);
        return {Refused::NotAPlugin, "it has no " + std::string{kUiEntryPoint} +
                                         ", so it is not this plugin's interface"};
    }

    const UiEntry *const entry = ask();
    if (entry == nullptr) {
        container::dynamic::close(handle);
        return {Refused::NotAPlugin, "its entry point answered with nothing"};
    }

    if (entry->abi != kUiEntryAbi) {

        const auto why = "it was built against version " + std::to_string(entry->abi) +
                         " of the interface contract and this phone is version " +
                         std::to_string(kUiEntryAbi);
        container::dynamic::close(handle);
        return {Refused::WrongEntryVersion, why};
    }

    if (auto differs = container::mismatch(
            thisUiBuild(),
            container::parseBuildKey(entry->buildKey == nullptr ? "" : entry->buildKey));
        !differs.empty()) {
        container::dynamic::close(handle);
        return {Refused::BuiltElsewhere, differs};
    }

    if (entry->id == nullptr || id != entry->id) {
        container::dynamic::close(handle);
        return {Refused::Disagrees,
                "it is another plugin's interface, standing under this one's name"};
    }

    if (entry->contribute == nullptr) {
        container::dynamic::close(handle);
        return {Refused::Incomplete, "it contributes nothing"};
    }

    entry->contribute(registry);
    opened_.push_back({std::string{id}, handle, true});
    return {};
}

void UiLoader::withdraw(std::string_view id, UiRegistry &registry)
{
    auto *const showing = findOpen(id);
    if (showing == nullptr || !showing->showing) {
        return;
    }

    registry.removeContributionsOf(
        QString::fromUtf8(showing->id.data(), static_cast<qsizetype>(showing->id.size())));
    showing->showing = false;
}

UiLoader::Report UiLoader::load(const std::filesystem::path &directory,
                                const std::vector<std::string> &ids, UiRegistry &registry)
{
    Report report;

    for (const auto &id : ids) {
        auto trouble = contribute(directory, id, registry);
        if (!trouble.ok()) {
            report.refused.push_back(
                {trouble.what, id, directory / id / uiLibraryFileName(id), std::move(trouble.why)});
            continue;
        }
        if (findOpen(id) != nullptr) {
            report.loaded.push_back(id);
        }
    }

    for (const auto &refusal : report.refused) {
        write(container::Severity::Warning,
              refusal.id + " has an interface that will not load: " + refusal.why);
    }
    return report;
}

void UiLoader::unload() noexcept
{
    while (!opened_.empty()) {
        const auto &last = opened_.back();

        if (registry_ != nullptr && last.showing) {
            registry_->removeContributionsOf(
                QString::fromUtf8(last.id.data(), static_cast<qsizetype>(last.id.size())));
        }

        static_cast<void>(last.handle);
        opened_.pop_back();
    }
    registry_ = nullptr;
}

}
