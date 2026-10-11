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

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

namespace softphone::ui {

struct RunningImage {
    std::filesystem::path image;
    std::filesystem::path mount;

    [[nodiscard]] bool valid() const noexcept
    {
        return !image.empty() && !mount.empty();
    }

    friend bool operator==(const RunningImage &, const RunningImage &) = default;
};

[[nodiscard]] RunningImage runningImage();

[[nodiscard]] std::filesystem::path defaultDataHome();

[[nodiscard]] std::string menuEntryText(std::string_view carried,
                                        const std::filesystem::path &image,
                                        const std::filesystem::path &icon = {});

[[nodiscard]] std::string quotedForExec(std::string_view path);

[[nodiscard]] std::string escapedForTheFile(std::string_view text);

class IMenuEntry {
public:
    IMenuEntry() = default;
    IMenuEntry(const IMenuEntry &) = delete;
    IMenuEntry &operator=(const IMenuEntry &) = delete;
    IMenuEntry(IMenuEntry &&) = delete;
    IMenuEntry &operator=(IMenuEntry &&) = delete;
    virtual ~IMenuEntry() = default;

    [[nodiscard]] virtual bool present() const = 0;

    [[nodiscard]] virtual bool add() = 0;

    [[nodiscard]] virtual bool remove() = 0;
};

class XdgMenuEntry final : public IMenuEntry {
public:

    explicit XdgMenuEntry(RunningImage image = runningImage(),
                          std::filesystem::path dataHome = defaultDataHome());

    [[nodiscard]] bool present() const override;
    [[nodiscard]] bool add() override;
    [[nodiscard]] bool remove() override;

    [[nodiscard]] std::filesystem::path entryPath() const;

private:

    [[nodiscard]] std::filesystem::path copyTheIcons() const;

    RunningImage image_;
    std::filesystem::path dataHome_;
};

[[nodiscard]] std::unique_ptr<IMenuEntry> makeMenuEntry();

}
