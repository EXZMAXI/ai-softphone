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
#include <softphone/container/loader.hpp>
#include <softphone/shell/ui_loading.hpp>
#include <softphone/shell/ui_registry.hpp>

#include <filesystem>

#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace softphone::shell {

class UiLoader {
public:
    using Contribution = std::function<void(UiRegistry &)>;
    using Refusal = container::Loader::Refusal;
    using Refused = container::Loader::Refused;

    struct Report {
        std::vector<std::string> loaded;
        std::vector<Refusal> refused;

        [[nodiscard]] bool clean() const noexcept
        {
            return refused.empty();
        }
    };

    UiLoader() = default;
    explicit UiLoader(container::LogSink log);

    UiLoader(const UiLoader &) = delete;
    UiLoader &operator=(const UiLoader &) = delete;
    UiLoader(UiLoader &&) = delete;
    UiLoader &operator=(UiLoader &&) = delete;
    ~UiLoader();

    void offerBuiltIn(std::string id, Contribution contribute);

    struct Trouble {
        Refused what{Refused::None};
        std::string why;

        [[nodiscard]] bool ok() const noexcept
        {
            return what == Refused::None;
        }
    };

    [[nodiscard]] Trouble contribute(const std::filesystem::path &directory, std::string_view id,
                                     UiRegistry &registry);

    void withdraw(std::string_view id, UiRegistry &registry);

    [[nodiscard]] Report load(const std::filesystem::path &directory,
                              const std::vector<std::string> &ids, UiRegistry &registry);

    void unload() noexcept;

    [[nodiscard]] std::size_t open() const noexcept
    {
        return opened_.size();
    }

private:
    struct Opened {
        std::string id;

        void *handle{nullptr};

        bool showing{false};
    };

    [[nodiscard]] Opened *findOpen(std::string_view id);

    void write(container::Severity severity, std::string_view message) const;

    container::LogSink log_;
    std::vector<Opened> opened_;

    std::vector<std::pair<std::string, Contribution>> builtIn_;

    UiRegistry *registry_{nullptr};
};

}
