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

#include <memory>
#include <optional>

namespace softphone::cli {

class IKeyboard {
public:
    IKeyboard() = default;
    IKeyboard(const IKeyboard &) = delete;
    IKeyboard &operator=(const IKeyboard &) = delete;
    IKeyboard(IKeyboard &&) = delete;
    IKeyboard &operator=(IKeyboard &&) = delete;
    virtual ~IKeyboard() = default;

    [[nodiscard]] virtual bool available() const noexcept = 0;

    [[nodiscard]] virtual std::optional<char> pressed() const = 0;
};

[[nodiscard]] std::unique_ptr<IKeyboard> makeKeyboard();

}
