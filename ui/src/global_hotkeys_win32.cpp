// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "global_hotkeys_win32.hpp"

#include <softphone/core/logging.hpp>

#include <QAbstractNativeEventFilter>
#include <QByteArray>
#include <QCoreApplication>
#include <QString>

#include <windows.h>

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace softphone::ui {
namespace {

struct Registered {
    int id{0};
    HotkeyAction action{HotkeyAction::Answer};
};

class Win32GlobalHotkeys final : public IGlobalHotkeys, public QAbstractNativeEventFilter {
public:
    explicit Win32GlobalHotkeys(Activated activated) : activated_{std::move(activated)}
    {
        QCoreApplication::instance()->installNativeEventFilter(this);
    }

    Win32GlobalHotkeys(const Win32GlobalHotkeys &) = delete;
    Win32GlobalHotkeys &operator=(const Win32GlobalHotkeys &) = delete;
    Win32GlobalHotkeys(Win32GlobalHotkeys &&) = delete;
    Win32GlobalHotkeys &operator=(Win32GlobalHotkeys &&) = delete;

    ~Win32GlobalHotkeys() override
    {
        release();
        if (QCoreApplication::instance() != nullptr) {
            QCoreApplication::instance()->removeNativeEventFilter(this);
        }
    }

    [[nodiscard]] bool supported() const override
    {
        return true;
    }

    [[nodiscard]] std::vector<GrabOutcome> grab(std::span<const Hotkey> bindings) override
    {
        release();

        std::vector<GrabOutcome> outcomes;

        for (const auto &binding : bindings) {
            if (!binding.global || binding.sequence.empty()) {
                continue;
            }

            outcomes.push_back(GrabOutcome{binding.action, registerOne(binding)});
        }

        return outcomes;
    }

    bool nativeEventFilter(const QByteArray &type, void *message, qintptr * ) override
    {
        if (type != "windows_generic_MSG" || message == nullptr) {
            return false;
        }

        const auto *event = static_cast<const MSG *>(message);
        if (event->message != WM_HOTKEY) {
            return false;
        }

        const auto id = static_cast<int>(event->wParam);

        for (const auto &held : registered_) {
            if (held.id == id) {
                if (activated_) {
                    activated_(held.action);
                }
                return true;
            }
        }

        return false;
    }

private:
    [[nodiscard]] GrabProblem registerOne(const Hotkey &binding)
    {
        const auto key = virtualKey(binding.sequence);
        if (key == 0) {
            return GrabProblem::Unreadable;
        }

        const auto id = static_cast<int>(registered_.size()) + 1;

        const auto modifiers = winModifiers(binding.sequence) | kNoRepeatFlag;

        if (::RegisterHotKey(nullptr, id, modifiers, key) != FALSE) {
            registered_.push_back(Registered{id, binding.action});
            core::Logger::instance().log(core::LogLevel::Info, "ui",
                                         "hotkey " + binding.sequence + " taken from the desktop");
            return GrabProblem::None;
        }

        const auto error = ::GetLastError();
        if (error == ERROR_HOTKEY_ALREADY_REGISTERED) {

            return GrabProblem::Taken;
        }

        core::Logger::instance().log(core::LogLevel::Warning, "ui",
                                     "cannot take " + binding.sequence +
                                         " from the desktop: " + std::to_string(error));
        return GrabProblem::Unreadable;
    }

    [[nodiscard]] static std::uint32_t virtualKey(std::string_view sequence)
    {
        if (const auto known = winVirtualKey(sequence); known != 0) {
            return known;
        }

        const auto name = QString::fromStdString(hotkeyKeyName(sequence));
        if (name.size() != 1) {
            return 0;
        }

        const auto scanned = ::VkKeyScanW(static_cast<WCHAR>(name.at(0).unicode()));
        if (scanned == -1) {

            return 0;
        }

        return static_cast<std::uint32_t>(scanned) & 0xFFU;
    }

    void release()
    {
        for (const auto &held : registered_) {
            static_cast<void>(::UnregisterHotKey(nullptr, held.id));
        }
        registered_.clear();
    }

    Activated activated_;
    std::vector<Registered> registered_;
};

}

std::unique_ptr<IGlobalHotkeys> makeWin32GlobalHotkeys(IGlobalHotkeys::Activated activated)
{
    return std::make_unique<Win32GlobalHotkeys>(std::move(activated));
}

}
