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

#include "autostart.hpp"

#include <filesystem>

namespace softphone::ui {

[[nodiscard]] std::filesystem::path defaultAutostartDirectory();

class XdgAutostart final : public IAutostart {
public:

    explicit XdgAutostart(std::filesystem::path directory = defaultAutostartDirectory(),
                          std::filesystem::path binary = runningBinary());

    [[nodiscard]] AutostartState state() const override;
    [[nodiscard]] bool enable(bool startMinimised) override;
    [[nodiscard]] bool disable() override;

    [[nodiscard]] std::filesystem::path entryPath() const;

private:
    std::filesystem::path directory_;
    std::filesystem::path binary_;
};
}
