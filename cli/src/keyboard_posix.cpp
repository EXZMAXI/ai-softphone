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

#include <termios.h>
#include <unistd.h>

namespace softphone::cli {
namespace {

class PosixKeyboard final : public IKeyboard {
public:
    PosixKeyboard()
    {
        if (::isatty(STDIN_FILENO) != 1 || ::tcgetattr(STDIN_FILENO, &saved_) != 0) {
            return;
        }

        auto raw = saved_;
        raw.c_lflag &= static_cast<tcflag_t>(~(ICANON | ECHO));

        raw.c_cc[VMIN] = 0;
        raw.c_cc[VTIME] = 0;

        if (::tcsetattr(STDIN_FILENO, TCSANOW, &raw) == 0) {
            restore_ = true;
        }
    }

    ~PosixKeyboard() override
    {
        if (restore_) {
            ::tcsetattr(STDIN_FILENO, TCSANOW, &saved_);
        }
    }

    [[nodiscard]] bool available() const noexcept override
    {
        return restore_;
    }

    [[nodiscard]] std::optional<char> pressed() const override
    {
        if (!restore_) {
            return std::nullopt;
        }

        char key = 0;
        return ::read(STDIN_FILENO, &key, 1) == 1 ? std::optional<char>{key} : std::nullopt;
    }

private:
    termios saved_{};
    bool restore_{false};
};

}

std::unique_ptr<IKeyboard> makeKeyboard()
{
    return std::make_unique<PosixKeyboard>();
}

}
