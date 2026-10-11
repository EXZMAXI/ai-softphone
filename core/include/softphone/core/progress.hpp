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
#include <set>
#include <string>
#include <vector>

namespace softphone::core {

class ProgressStore {
public:

    explicit ProgressStore(std::filesystem::path file);

    [[nodiscard]] bool done(const std::string &code) const;

    void note(const std::string &code);

    [[nodiscard]] std::vector<std::string> all() const;

private:
    void write();

    std::filesystem::path file_;
    std::set<std::string> done_;
};

}
