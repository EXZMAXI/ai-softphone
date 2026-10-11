// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "global_hotkeys_apple.hpp"

#include <softphone/core/logging.hpp>

#include <Carbon/Carbon.h>

#include <cstdint>
#include <utility>
#include <vector>

namespace softphone::ui {
namespace {

constexpr ::OSType kSignature = 'sftp';

class AppleGlobalHotkeys final : public IGlobalHotkeys {
public:
    explicit AppleGlobalHotkeys(Activated activated) : activated_{std::move(activated)} {}

    ~AppleGlobalHotkeys() override
    {
        release();
        if (handler_ != nullptr) {
            ::RemoveEventHandler(handler_);
        }
    }

    [[nodiscard]] std::vector<GrabOutcome> grab(std::span<const Hotkey> bindings) override
    {
        release();
        install();

        std::vector<GrabOutcome> outcomes;
        for (const auto &binding : bindings) {
            if (!binding.global || binding.sequence.empty()) {
                continue;
            }

            GrabOutcome outcome{binding.action, GrabProblem::None};

            const auto key = carbonKeyCode(binding.sequence);
            const auto modifiers = carbonModifiers(binding.sequence);
            if (!key.has_value() || modifiers == 0) {

                outcome.problem = GrabProblem::Unreadable;
                outcomes.push_back(outcome);
                continue;
            }

            const ::EventHotKeyID identity{kSignature, static_cast<::UInt32>(binding.action)};

            ::EventHotKeyRef held = nullptr;
            const auto status =
                ::RegisterEventHotKey(static_cast<::UInt32>(*key), static_cast<::UInt32>(modifiers),
                                      identity, ::GetApplicationEventTarget(), 0, &held);
            if (status != noErr || held == nullptr) {

                outcome.problem = GrabProblem::Taken;
                core::Logger::instance().log(core::LogLevel::Info, "ui",
                                             "the desktop would not give up " + binding.sequence);
            } else {
                held_.push_back(held);
            }

            outcomes.push_back(outcome);
        }

        return outcomes;
    }

    [[nodiscard]] bool supported() const override
    {

        return true;
    }

private:

    void install()
    {
        if (handler_ != nullptr) {
            return;
        }

        static const ::EventTypeSpec wanted{kEventClassKeyboard, kEventHotKeyPressed};
        static_cast<void>(::InstallApplicationEventHandler(&AppleGlobalHotkeys::pressed, 1, &wanted,
                                                           this, &handler_));
    }

    void release()
    {
        for (auto *held : held_) {
            ::UnregisterEventHotKey(held);
        }
        held_.clear();
    }

    static ::OSStatus pressed(::EventHandlerCallRef, ::EventRef event, void *context)
    {
        auto *self = static_cast<AppleGlobalHotkeys *>(context);
        if (self == nullptr || self->activated_ == nullptr) {
            return noErr;
        }

        ::EventHotKeyID identity{};
        if (::GetEventParameter(event, kEventParamDirectObject, typeEventHotKeyID, nullptr,
                                sizeof(identity), nullptr, &identity) != noErr) {
            return noErr;
        }
        if (identity.signature != kSignature) {
            return eventNotHandledErr;
        }

        self->activated_(static_cast<HotkeyAction>(identity.id));
        return noErr;
    }

    Activated activated_;
    ::EventHandlerRef handler_{nullptr};
    std::vector<::EventHotKeyRef> held_;
};

}

std::unique_ptr<IGlobalHotkeys> makeAppleGlobalHotkeys(IGlobalHotkeys::Activated activated)
{
    return std::make_unique<AppleGlobalHotkeys>(std::move(activated));
}

}
