// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "global_hotkeys_x11.hpp"

#include <softphone/core/logging.hpp>

#include <QAbstractNativeEventFilter>
#include <QByteArray>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QtGui/qguiapplication_platform.h>

#include <xcb/xcb.h>

#include <X11/Xlib.h>
#include <X11/keysym.h>

#undef None
#undef Bool
#undef Status
#undef Success
#undef KeyPress
#undef KeyRelease
#undef FocusIn
#undef FocusOut
#undef CursorShape
#undef Expose
#undef Unsorted

#include <algorithm>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

namespace softphone::ui {
namespace {

using X11Application = QNativeInterface::QX11Application;

struct Grabbed {
    std::uint8_t keycode{0};
    std::uint16_t modifiers{0};
};

class X11GlobalHotkeys final : public IGlobalHotkeys, public QAbstractNativeEventFilter {
public:
    explicit X11GlobalHotkeys(Activated activated) : activated_{std::move(activated)}
    {
        if (auto *x11 = qGuiApp->nativeInterface<X11Application>()) {
            display_ = x11->display();
            connection_ = x11->connection();
        }

        if (connection_ != nullptr) {
            root_ = xcb_setup_roots_iterator(xcb_get_setup(connection_)).data->root;
            QCoreApplication::instance()->installNativeEventFilter(this);
        }
    }

    X11GlobalHotkeys(const X11GlobalHotkeys &) = delete;
    X11GlobalHotkeys &operator=(const X11GlobalHotkeys &) = delete;
    X11GlobalHotkeys(X11GlobalHotkeys &&) = delete;
    X11GlobalHotkeys &operator=(X11GlobalHotkeys &&) = delete;

    ~X11GlobalHotkeys() override
    {
        release();
        if (connection_ != nullptr && QCoreApplication::instance() != nullptr) {
            QCoreApplication::instance()->removeNativeEventFilter(this);
        }
    }

    [[nodiscard]] bool supported() const override
    {
        return connection_ != nullptr && display_ != nullptr;
    }

    [[nodiscard]] std::vector<GrabOutcome> grab(std::span<const Hotkey> bindings) override
    {
        release();

        wanted_.assign(bindings.begin(), bindings.end());

        std::vector<GrabOutcome> outcomes;

        for (const auto &binding : bindings) {
            if (!binding.global || binding.sequence.empty()) {
                continue;
            }

            if (!supported()) {
                outcomes.push_back(GrabOutcome{binding.action, GrabProblem::Unsupported});
                continue;
            }

            outcomes.push_back(GrabOutcome{binding.action, grabOne(binding)});
        }

        return outcomes;
    }

    bool nativeEventFilter(const QByteArray &type, void *message, qintptr * ) override
    {
        if (type != "xcb_generic_event_t" || message == nullptr) {
            return false;
        }

        auto *event = static_cast<xcb_generic_event_t *>(message);
        const auto kind = static_cast<std::uint8_t>(event->response_type & ~0x80U);

        if (kind == XCB_MAPPING_NOTIFY) {

            auto again = wanted_;
            static_cast<void>(grab(again));
            return false;
        }

        if (kind != XCB_KEY_PRESS) {
            return false;
        }

        auto *press = static_cast<xcb_key_press_event_t *>(message);
        const auto modifiers = static_cast<std::uint16_t>(press->state & ~kIgnoredModifiers);

        for (const auto &[keycode, wanted, action] : bound_) {
            if (keycode == press->detail && wanted == modifiers) {
                if (activated_) {
                    activated_(action);
                }
                return true;
            }
        }

        return false;
    }

private:
    struct Bound {
        std::uint8_t keycode{0};
        std::uint16_t modifiers{0};
        HotkeyAction action{HotkeyAction::Answer};
    };

    [[nodiscard]] GrabProblem grabOne(const Hotkey &binding)
    {
        const auto name = xKeyName(binding.sequence);
        if (name.empty()) {
            return GrabProblem::Unreadable;
        }

        auto keysym = XStringToKeysym(name.c_str());
        if (keysym == NoSymbol) {
            auto lowered = name;
            std::ranges::transform(lowered, lowered.begin(),
                                   [](unsigned char letter) { return std::tolower(letter); });
            keysym = XStringToKeysym(lowered.c_str());
        }

        if (keysym == NoSymbol) {
            return GrabProblem::Unreadable;
        }

        const auto keycode = XKeysymToKeycode(display_, keysym);
        if (keycode == 0) {

            return GrabProblem::Unreadable;
        }

        const auto modifiers = xModifiers(binding.sequence);
        std::vector<Grabbed> made;

        for (const auto combination : withIgnoredModifiers(modifiers)) {
            const auto cookie = xcb_grab_key_checked(connection_, 1, root_, combination, keycode,
                                                     XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);

            std::unique_ptr<xcb_generic_error_t, decltype(&free)> error{
                xcb_request_check(connection_, cookie), &free};

            if (error) {
                for (const auto &undo : made) {
                    xcb_ungrab_key(connection_, undo.keycode, root_, undo.modifiers);
                }
                xcb_flush(connection_);
                return GrabProblem::Taken;
            }

            made.push_back(Grabbed{keycode, combination});
        }

        xcb_flush(connection_);

        for (const auto &held : made) {
            grabs_.push_back(held);
        }
        bound_.push_back(Bound{keycode, modifiers, binding.action});

        core::Logger::instance().log(core::LogLevel::Info, "ui",
                                     "hotkey " + binding.sequence + " taken from the desktop");
        return GrabProblem::None;
    }

    void release()
    {
        if (connection_ == nullptr) {
            grabs_.clear();
            bound_.clear();
            return;
        }

        for (const auto &held : grabs_) {
            xcb_ungrab_key(connection_, held.keycode, root_, held.modifiers);
        }
        xcb_flush(connection_);

        grabs_.clear();
        bound_.clear();
    }

    Activated activated_;
    Display *display_{nullptr};
    xcb_connection_t *connection_{nullptr};
    xcb_window_t root_{0};
    std::vector<Grabbed> grabs_;
    std::vector<Bound> bound_;
    std::vector<Hotkey> wanted_;
};

}

std::unique_ptr<IGlobalHotkeys> makeX11GlobalHotkeys(IGlobalHotkeys::Activated activated)
{
    return std::make_unique<X11GlobalHotkeys>(std::move(activated));
}

}
