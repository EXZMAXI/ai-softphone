// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/container/loader.hpp>

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace softphone::container {

std::string dynamic::lastError()
{
#if defined(_WIN32)
    const auto code = ::GetLastError();
    char *text = nullptr;
    const auto length = ::FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, code, 0, reinterpret_cast<char *>(&text), 0, nullptr);
    std::string message = length != 0 && text != nullptr ? std::string{text, length}
                                                         : "error " + std::to_string(code);
    if (text != nullptr) {
        ::LocalFree(text);
    }
    while (!message.empty() && (message.back() == '\n' || message.back() == '\r')) {
        message.pop_back();
    }
    return message;
#else
    const char *const text = ::dlerror();
    return text != nullptr ? std::string{text} : std::string{"the system did not say why"};
#endif
}

void *dynamic::open(const std::filesystem::path &file)
{
#if defined(_WIN32)
    return ::LoadLibraryW(file.wstring().c_str());
#else

    return ::dlopen(file.c_str(), RTLD_NOW | RTLD_LOCAL);
#endif
}

void dynamic::close(void *handle)
{
    if (handle == nullptr) {
        return;
    }
#if defined(_WIN32)
    static_cast<void>(::FreeLibrary(static_cast<HMODULE>(handle)));
#else
    static_cast<void>(::dlclose(handle));
#endif
}

void *dynamic::symbol(void *handle, const char *name)
{
#if defined(_WIN32)
    auto *const found = ::GetProcAddress(static_cast<HMODULE>(handle), name);
#else
    auto *const found = ::dlsym(handle, name);
#endif

    return reinterpret_cast<void *>(reinterpret_cast<std::uintptr_t>(found));
}

namespace {

[[nodiscard]] bool readWholeFile(const std::filesystem::path &file, std::string &out)
{
    std::ifstream stream{file, std::ios::binary};
    if (!stream) {
        return false;
    }
    std::ostringstream all;
    all << stream.rdbuf();
    out = all.str();
    return true;
}

}

std::string libraryFileName(std::string_view id)
{
#if defined(_WIN32)
    return "softphone_plugin_" + std::string{id} + ".dll";
#elif defined(__APPLE__)
    return "libsoftphone_plugin_" + std::string{id} + ".dylib";
#else
    return "libsoftphone_plugin_" + std::string{id} + ".so";
#endif
}

Loader::Loader(LogSink log) : log_{std::move(log)} {}

Loader::~Loader()
{
    unload();
}

void Loader::write(Severity severity, std::string_view message) const
{
    if (log_) {
        log_(severity, "plugins", message);
    }
}

Loader::Report Loader::load(const std::filesystem::path &directory, Host &host,
                            const std::vector<std::string> &disabled)
{
    Report report;

    std::error_code problem;
    if (!std::filesystem::is_directory(directory, problem)) {

        return report;
    }

    std::vector<std::filesystem::path> candidates;
    for (const auto &entry : std::filesystem::directory_iterator{directory, problem}) {
        if (entry.is_directory(problem)) {
            candidates.push_back(entry.path());
        }
    }
    std::ranges::sort(candidates);

    for (const auto &where : candidates) {
        const auto file = where / "manifest.json";

        std::string document;
        if (!readWholeFile(file, document)) {

            report.refused.push_back(
                {Refused::NoManifest, {}, where, "there is no manifest.json in it"});
            continue;
        }

        Manifest manifest;
        if (const auto refused = parseManifest(document, manifest);
            refused != ManifestProblem::None) {
            report.refused.push_back(
                {Refused::BadManifest,
                 {},
                 where,
                 "its manifest is not one this build accepts: " + std::string{toString(refused)}});
            continue;
        }

        if (std::ranges::find(disabled, manifest.id) != disabled.end()) {

            const auto id = manifest.id;
            static_cast<void>(host.add(std::move(manifest), nullptr));
            host_ = &host;
            report.skipped.push_back(id);
            continue;
        }

        Refusal refusal;
        const auto made = make(where, manifest, refusal);
        if (made.plugin == nullptr) {

            static_cast<void>(host.add(manifest, nullptr));
            host_ = &host;
            report.refused.push_back(std::move(refusal));
            continue;
        }

        const auto id = manifest.id;
        if (const auto added = host.add(std::move(manifest), made.plugin);
            added != AddProblem::None) {
            made.destroy(made.plugin);
            dynamic::close(made.handle);
            report.refused.push_back(
                {Refused::Rejected, id, where / libraryFileName(id), std::string{toString(added)}});
            continue;
        }

        host_ = &host;
        opened_.emplace_back(Opened{id, made.handle, made.plugin, made.destroy});
        report.loaded.push_back(id);
    }

    for (const auto &refusal : report.refused) {
        write(Severity::Warning,
              (refusal.id.empty() ? refusal.where.filename().string() : refusal.id) + ": " +
                  refusal.why);
    }
    write(Severity::Info, "plugins on disk: " + std::to_string(report.loaded.size()) + " loaded, " +
                              std::to_string(report.skipped.size()) + " off, " +
                              std::to_string(report.refused.size()) + " refused");
    return report;
}

Loader::Made Loader::make(const std::filesystem::path &where, const Manifest &manifest,
                          Refusal &refusal) const
{
    const auto library = where / libraryFileName(manifest.id);
    const auto refuse = [&refusal, &manifest, &library](Refused what, std::string why) {
        refusal = {what, manifest.id, library, std::move(why)};
        return Made{};
    };

    void *const handle = dynamic::open(library);
    if (handle == nullptr) {
        return refuse(Refused::CannotOpen, dynamic::lastError());
    }

    const auto ask = reinterpret_cast<EntryPoint>(dynamic::symbol(handle, kEntryPoint));
    if (ask == nullptr) {
        dynamic::close(handle);
        return refuse(Refused::NotAPlugin,
                      "it has no " + std::string{kEntryPoint} +
                          ", so it is a library this container did not build as a plugin");
    }

    const PluginEntry *const entry = ask();
    if (entry == nullptr) {
        dynamic::close(handle);
        return refuse(Refused::NotAPlugin, "its entry point answered with nothing");
    }

    if (entry->abi != kEntryAbi) {

        auto why = "it was built against version " + std::to_string(entry->abi) +
                   " of the plugin interface and this phone is version " +
                   std::to_string(kEntryAbi);
        dynamic::close(handle);
        return refuse(Refused::WrongEntryVersion, std::move(why));
    }

    if (auto differs =
            mismatch(thisBuild(), parseBuildKey(entry->buildKey == nullptr ? "" : entry->buildKey));
        !differs.empty()) {
        dynamic::close(handle);
        return refuse(Refused::BuiltElsewhere, std::move(differs));
    }

    Manifest baked;
    if (entry->manifest == nullptr ||
        parseManifest(entry->manifest(), baked) != ManifestProblem::None ||
        baked.id != manifest.id || baked.version != manifest.version) {
        dynamic::close(handle);
        return refuse(
            Refused::Disagrees,
            "the library and the manifest.json beside it do not describe the same plugin");
    }

    if (entry->create == nullptr || entry->destroy == nullptr) {
        dynamic::close(handle);
        return refuse(Refused::Incomplete, "its entry point is incomplete");
    }

    IPlugin *const plugin = entry->create();
    if (plugin == nullptr) {
        dynamic::close(handle);
        return refuse(Refused::Incomplete, "it would not make an instance");
    }

    return Made{handle, plugin, entry->destroy};
}

std::string Loader::open(const std::filesystem::path &directory, std::string_view id, Host &host)
{
    if (isOpen(id)) {
        return {};
    }

    const auto where = directory / id;

    std::string document;
    if (!readWholeFile(where / "manifest.json", document)) {
        return "there is no manifest.json in " + where.filename().string();
    }

    Manifest manifest;
    if (const auto refused = parseManifest(document, manifest); refused != ManifestProblem::None) {
        return "its manifest is not one this build accepts: " + std::string{toString(refused)};
    }
    if (manifest.id != id) {
        return "the manifest in that directory is another plugin's";
    }

    Refusal refusal;
    const auto made = make(where, manifest, refusal);
    if (made.plugin == nullptr) {
        write(Severity::Warning, std::string{id} + ": " + refusal.why);
        return refusal.why;
    }

    if (!host.attach(id, made.plugin)) {

        if (const auto added = host.add(std::move(manifest), made.plugin);
            added != AddProblem::None) {
            made.destroy(made.plugin);
            dynamic::close(made.handle);
            return std::string{toString(added)};
        }
    }

    host_ = &host;
    opened_.emplace_back(Opened{std::string{id}, made.handle, made.plugin, made.destroy});
    write(Severity::Info, std::string{id} + ": opened");
    return {};
}

bool Loader::isOpen(std::string_view id) const
{
    return std::ranges::any_of(opened_, [id](const Opened &one) { return one.id == id; });
}

void Loader::unload() noexcept
{

    if (host_ != nullptr) {
        host_->leave();
    }

    while (!opened_.empty()) {
        auto &last = opened_.back();
        if (last.plugin != nullptr && last.destroy != nullptr) {
            last.destroy(last.plugin);
        }
        dynamic::close(last.handle);
        opened_.pop_back();
    }
    host_ = nullptr;
}

}
