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

#include <softphone/container/host.hpp>
#include <softphone/container/loading.hpp>
#include <softphone/container/manifest.hpp>

#include <filesystem>

#include <string>
#include <string_view>
#include <vector>

namespace softphone::container {

namespace dynamic {

[[nodiscard]] void *open(const std::filesystem::path &file);

void close(void *handle);

[[nodiscard]] void *symbol(void *handle, const char *name);

[[nodiscard]] std::string lastError();

}

[[nodiscard]] std::string libraryFileName(std::string_view id);

class Loader {
public:

    enum class Refused {
        None,

        NoManifest,

        BadManifest,

        CannotOpen,

        NotAPlugin,

        WrongEntryVersion,

        BuiltElsewhere,

        Disagrees,

        Incomplete,

        Rejected,
    };

    struct Refusal {
        Refused what{Refused::None};

        std::string id;
        std::filesystem::path where;

        std::string why;
    };

    struct Report {
        std::vector<std::string> loaded;

        std::vector<std::string> skipped;

        std::vector<Refusal> refused;

        [[nodiscard]] bool clean() const noexcept
        {
            return refused.empty();
        }
    };

    Loader() = default;
    explicit Loader(LogSink log);

    Loader(const Loader &) = delete;
    Loader &operator=(const Loader &) = delete;
    Loader(Loader &&) = delete;
    Loader &operator=(Loader &&) = delete;
    ~Loader();

    [[nodiscard]] Report load(const std::filesystem::path &directory, Host &host,
                              const std::vector<std::string> &disabled = {});

    [[nodiscard]] std::string open(const std::filesystem::path &directory, std::string_view id,
                                   Host &host);

    [[nodiscard]] bool isOpen(std::string_view id) const;

    void unload() noexcept;

    [[nodiscard]] std::size_t open() const noexcept
    {
        return opened_.size();
    }

private:
    struct Opened {
        std::string id;

        void *handle{nullptr};

        IPlugin *plugin{nullptr};
        void (*destroy)(IPlugin *){nullptr};
    };

    struct Made {
        void *handle{nullptr};
        IPlugin *plugin{nullptr};
        void (*destroy)(IPlugin *){nullptr};
    };

    [[nodiscard]] Made make(const std::filesystem::path &where, const Manifest &manifest,
                            Refusal &refusal) const;

    void write(Severity severity, std::string_view message) const;

    LogSink log_;
    std::vector<Opened> opened_;

    Host *host_{nullptr};
};

}
