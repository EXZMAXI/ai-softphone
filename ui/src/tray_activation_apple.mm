// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "tray_activation_apple.hpp"

#import <AppKit/AppKit.h>

#include <objc/runtime.h>

namespace softphone::ui {
namespace {

constexpr const char *kDelegateClass = "QStatusItemDelegate";
constexpr const char *kClicked = "statusItemClicked";
constexpr const char *kMenuBeganTracking = "statusItemMenuBeganTracking:";

using Clicked = void (*)(id, SEL);
using BeganTracking = void (*)(id, SEL, id);

Clicked theirClicked = nullptr;
BeganTracking theirBeganTracking = nullptr;

[[nodiscard]] bool theCurrentEventCanBeAsked()
{
    NSEvent *const current = NSApp.currentEvent;
    if (current == nil) {

        return true;
    }
    return clickCountIsRead(static_cast<unsigned long>(current.type));
}

void ourClicked(id self, SEL selector)
{
    if (!theCurrentEventCanBeAsked()) {
        return;
    }
    theirClicked(self, selector);
}

void ourBeganTracking(id self, SEL selector, id notification)
{
    if (!theCurrentEventCanBeAsked()) {
        return;
    }
    theirBeganTracking(self, selector, notification);
}

}

bool clickCountIsRead(unsigned long eventType) noexcept
{
    switch (eventType) {
    case NSEventTypeLeftMouseDown:
    case NSEventTypeLeftMouseUp:
    case NSEventTypeRightMouseDown:
    case NSEventTypeRightMouseUp:
    case NSEventTypeMouseMoved:
    case NSEventTypeLeftMouseDragged:
    case NSEventTypeRightMouseDragged:
    case NSEventTypeOtherMouseDown:
    case NSEventTypeOtherMouseUp:
    case NSEventTypeOtherMouseDragged:
        return true;
    default:

        return false;
    }
}

std::string_view toString(TrayGuard found) noexcept
{
    switch (found) {
    case TrayGuard::installed:
        return "installed";
    case TrayGuard::noCocoaPlugin:
        return "no cocoa plugin";
    case TrayGuard::qtHasMoved:
        return "qt has moved";
    }
    return "unknown";
}

TrayGuard keepTheTrayFromAborting()
{
    if (theirBeganTracking != nullptr) {
        return TrayGuard::installed;
    }

    const Class delegate = objc_lookUpClass(kDelegateClass);
    if (delegate == nil) {
        return TrayGuard::noCocoaPlugin;
    }

    Method const clicked = class_getInstanceMethod(delegate, sel_registerName(kClicked));
    Method const tracking = class_getInstanceMethod(delegate, sel_registerName(kMenuBeganTracking));
    if (clicked == nullptr || tracking == nullptr) {
        return TrayGuard::qtHasMoved;
    }

    theirClicked = reinterpret_cast<Clicked>(
            method_setImplementation(clicked, reinterpret_cast<IMP>(&ourClicked)));
    theirBeganTracking = reinterpret_cast<BeganTracking>(
            method_setImplementation(tracking, reinterpret_cast<IMP>(&ourBeganTracking)));
    return TrayGuard::installed;
}

}
