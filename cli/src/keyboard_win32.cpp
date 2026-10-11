// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "keyboard.hpp"

#include <conio.h>
#include <io.h>
#include <stdio.h>

namespace softphone::cli {
namespace {

class WindowsKeyboard final : public IKeyboard {
public:
    WindowsKeyboard() : console_{::_isatty(::_fileno(stdin)) != 0} {}

    [[nodiscard]] bool available() const noexcept override
    {
        return console_;
    }

    [[nodiscard]] std::optional<char> pressed() const override
    {
        if (!console_ || ::_kbhit() == 0) {
            return std::nullopt;
        }

        const int key = ::_getch();
        if (key <= 0) {
            return std::nullopt;
        }

        if (key == 0x00 || key == 0xE0) {
            static_cast<void>(::_getch());
            return std::nullopt;
        }

        return static_cast<char>(key);
    }

private:
    bool console_{false};
};

}

std::unique_ptr<IKeyboard> makeKeyboard()
{
    return std::make_unique<WindowsKeyboard>();
}

}
