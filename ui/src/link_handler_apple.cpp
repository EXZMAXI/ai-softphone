// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "link_handler_apple.hpp"

#include <softphone/core/dial_link.hpp>
#include <softphone/core/logging.hpp>

#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>

#include <algorithm>
#include <cctype>
#include <string>

namespace softphone::ui {
namespace {

class Held {
public:
    explicit Held(CFStringRef held) : held_{held} {}

    Held(const Held &) = delete;
    Held &operator=(const Held &) = delete;
    Held(Held &&) = delete;
    Held &operator=(Held &&) = delete;

    ~Held()
    {
        if (held_ != nullptr) {
            CFRelease(held_);
        }
    }

    [[nodiscard]] CFStringRef get() const noexcept
    {
        return held_;
    }

    [[nodiscard]] std::string text() const
    {
        if (held_ == nullptr) {
            return {};
        }
        const auto length = CFStringGetLength(held_);
        const auto room = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;
        std::string out(static_cast<std::size_t>(room), '\0');
        if (!CFStringGetCString(held_, out.data(), room, kCFStringEncodingUTF8)) {
            return {};
        }
        out.resize(std::char_traits<char>::length(out.c_str()));
        return out;
    }

private:
    CFStringRef held_{nullptr};
};

[[nodiscard]] std::string ourIdentifier()
{
    auto *const bundle = CFBundleGetMainBundle();
    if (bundle == nullptr) {
        return {};
    }
    auto *const identifier = CFBundleGetIdentifier(bundle);
    if (identifier == nullptr) {
        return {};
    }

    const auto length = CFStringGetLength(identifier);
    const auto room = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;
    std::string out(static_cast<std::size_t>(room), '\0');
    if (!CFStringGetCString(identifier, out.data(), room, kCFStringEncodingUTF8)) {
        return {};
    }
    out.resize(std::char_traits<char>::length(out.c_str()));
    return out;
}

[[nodiscard]] CFStringRef asCFString(std::string_view text)
{
    return CFStringCreateWithBytes(kCFAllocatorDefault,
                                   reinterpret_cast<const UInt8 *>(text.data()),
                                   static_cast<CFIndex>(text.size()), kCFStringEncodingUTF8, false);
}

}

LinkHandlerState AppleLinkHandler::state() const
{
    LinkHandlerState answer;

    const auto ours = ourIdentifier();
    answer.possible = !ours.empty();
    if (!answer.possible) {
        return answer;
    }

    answer.preferred = true;
    for (const auto scheme : core::kDialSchemes) {
        const Held name{asCFString(scheme)};

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
        const Held holder{name.get() == nullptr ? nullptr
                                                : LSCopyDefaultHandlerForURLScheme(name.get())};
#pragma clang diagnostic pop
        const auto held = holder.text();

        const auto same = held.size() == ours.size() &&
                          std::equal(held.begin(), held.end(), ours.begin(), [](char a, char b) {
                              return std::tolower(static_cast<unsigned char>(a)) ==
                                     std::tolower(static_cast<unsigned char>(b));
                          });
        if (!same) {
            answer.preferred = false;
            if (answer.heldBy.empty() && !held.empty()) {
                answer.heldBy = held;
            }
        }
    }
    return answer;
}

bool AppleLinkHandler::claim()
{
    const auto ours = ourIdentifier();
    if (ours.empty()) {
        return false;
    }

    const Held us{asCFString(ours)};
    if (us.get() == nullptr) {
        return false;
    }

    bool every = true;
    for (const auto scheme : core::kDialSchemes) {
        const Held name{asCFString(scheme)};
        if (name.get() == nullptr) {
            every = false;
            continue;
        }
        if (LSSetDefaultHandlerForURLScheme(name.get(), us.get()) != noErr) {
            core::Logger::instance().log(core::LogLevel::Warning, "ui",
                                         "LaunchServices refused this phone for " +
                                             std::string{scheme} + ": links");
            every = false;
        }
    }
    return every;
}

std::unique_ptr<ILinkHandler> makeLinkHandler()
{
    return std::make_unique<AppleLinkHandler>();
}

}
