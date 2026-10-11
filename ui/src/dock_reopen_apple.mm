// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "dock_reopen_apple.hpp"

#import <AppKit/AppKit.h>

#include <objc/runtime.h>

#include <utility>

namespace softphone::ui {
namespace {

constexpr const char *kDelegateClass = "QCocoaApplicationDelegate";
constexpr const char *kShouldReopen = "applicationShouldHandleReopen:hasVisibleWindows:";

using ShouldReopen = BOOL (*)(id, SEL, id, BOOL);

ShouldReopen theirShouldReopen = nullptr;
std::function<void()> theAnswer;

BOOL ourShouldReopen(id self, SEL selector, id application, BOOL hasVisibleWindows)
{

    const BOOL said = theirShouldReopen(self, selector, application, hasVisibleWindows);
    if (theAnswer) {
        theAnswer();
    }
    return said;
}

}

std::string_view toString(DockHook found) noexcept
{
    switch (found) {
    case DockHook::installed:
        return "installed";
    case DockHook::noCocoaPlugin:
        return "no cocoa plugin";
    case DockHook::qtHasMoved:
        return "qt has moved";
    }
    return "unknown";
}

DockHook whenTheDockIconIsClicked(std::function<void()> answer)
{

    theAnswer = std::move(answer);
    if (theirShouldReopen != nullptr) {
        return DockHook::installed;
    }

    const Class delegate = objc_lookUpClass(kDelegateClass);
    if (delegate == nil) {
        return DockHook::noCocoaPlugin;
    }

    Method const reopen = class_getInstanceMethod(delegate, sel_registerName(kShouldReopen));
    if (reopen == nullptr) {
        return DockHook::qtHasMoved;
    }

    theirShouldReopen = reinterpret_cast<ShouldReopen>(
            method_setImplementation(reopen, reinterpret_cast<IMP>(&ourShouldReopen)));
    return DockHook::installed;
}

}
