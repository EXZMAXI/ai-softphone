// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "hotkeys.hpp"

#include <QKeySequence>
#include <QString>

#include <algorithm>
#include <map>
#include <string>
#include <string_view>

namespace softphone::ui {
namespace {

[[nodiscard]] QKeySequence parse(std::string_view sequence)
{
    return QKeySequence::fromString(
        QString::fromUtf8(sequence.data(), static_cast<qsizetype>(sequence.size())),
        QKeySequence::PortableText);
}

[[nodiscard]] bool nameable(const QKeySequence &sequence)
{
    return !sequence.isEmpty() && sequence.count() == 1 && sequence[0].key() != Qt::Key_unknown;
}

}

std::vector<HotkeyAction> hotkeyActions()
{
    return {HotkeyAction::Answer, HotkeyAction::HangUp, HotkeyAction::HoldResume,
            HotkeyAction::Mute, HotkeyAction::ShowWindow};
}

constexpr std::string_view kHeld =
#if defined(__APPLE__)
    "Meta+Alt+";
#else
    "Ctrl+Alt+";
#endif

std::vector<Hotkey> defaultHotkeys()
{

    const auto on = [](char letter) { return std::string{kHeld} + letter; };

    return {
        Hotkey{HotkeyAction::Answer, on('A'), false},
        Hotkey{HotkeyAction::HangUp, on('E'), false},
        Hotkey{HotkeyAction::HoldResume, on('H'), false},
        Hotkey{HotkeyAction::Mute, on('M'), false},

        Hotkey{HotkeyAction::ShowWindow, on('S'), true},
    };
}

std::string normaliseHotkey(std::string_view sequence)
{
    if (sequence.empty()) {
        return {};
    }

    const auto parsed = parse(sequence);
    if (!nameable(parsed)) {
        return {};
    }

    return parsed.toString(QKeySequence::PortableText).toStdString();
}

HotkeyProblem checkHotkey(std::string_view sequence)
{
    if (sequence.empty()) {
        return HotkeyProblem::None;
    }

    const auto parsed = parse(sequence);
    if (!nameable(parsed)) {

        return HotkeyProblem::Unreadable;
    }

    const auto modifiers = parsed[0].keyboardModifiers();
    const bool held = modifiers.testFlag(Qt::ControlModifier) ||
                      modifiers.testFlag(Qt::AltModifier) || modifiers.testFlag(Qt::MetaModifier);

    return held ? HotkeyProblem::None : HotkeyProblem::NoModifier;
}

std::vector<Hotkey> planHotkeys(std::span<const Hotkey> stored)
{
    if (stored.empty()) {
        return defaultHotkeys();
    }

    std::map<HotkeyAction, Hotkey> byAction;
    for (const auto &binding : stored) {
        auto kept = binding;
        kept.sequence = checkHotkey(kept.sequence) == HotkeyProblem::None
                            ? normaliseHotkey(kept.sequence)
                            : std::string{};

        if (kept.sequence.empty()) {
            kept.global = false;
        }

        byAction.emplace(kept.action, std::move(kept));
    }

    const auto shipped = defaultHotkeys();

    std::vector<Hotkey> planned;
    planned.reserve(hotkeyActions().size());

    for (const auto action : hotkeyActions()) {
        if (const auto found = byAction.find(action); found != byAction.end()) {
            planned.push_back(found->second);
            continue;
        }

        const auto fallback = std::ranges::find(shipped, action, &Hotkey::action);
        planned.push_back(fallback != shipped.end() ? *fallback : Hotkey{action, {}, false});
    }

    return planned;
}

std::vector<HotkeyAction> clashingHotkeys(std::span<const Hotkey> bindings)
{
    std::vector<HotkeyAction> clashing;

    for (const auto &binding : bindings) {
        if (binding.sequence.empty()) {

            continue;
        }

        const auto others = std::ranges::count_if(bindings, [&binding](const Hotkey &other) {
            return other.sequence == binding.sequence;
        });

        if (others > 1) {
            clashing.push_back(binding.action);
        }
    }

    return clashing;
}

}
