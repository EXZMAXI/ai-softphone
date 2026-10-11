// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "global_hotkeys.hpp"

#include <QKeySequence>
#include <QLatin1StringView>
#include <QList>
#include <QString>
#include <QStringList>

#include <QGuiApplication>

#include <algorithm>
#include <array>
#include <optional>
#include <utility>

#ifdef __APPLE__
#include "global_hotkeys_apple.hpp"
#endif

#ifdef SOFTPHONE_HAS_X11
#include "global_hotkeys_x11.hpp"
#endif

#ifdef _WIN32
#include "global_hotkeys_win32.hpp"
#endif

namespace softphone::ui {
namespace {

[[nodiscard]] QStringList parts(std::string_view sequence)
{
    return QString::fromUtf8(sequence.data(), static_cast<qsizetype>(sequence.size()))
        .split(QLatin1Char('+'), Qt::KeepEmptyParts);
}

[[nodiscard]] QString lastPiece(std::string_view sequence)
{
    if (sequence.empty()) {
        return {};
    }

    const auto pieces = parts(sequence);
    return pieces.isEmpty() ? QString{} : pieces.constLast().trimmed();
}

[[nodiscard]] bool isModifierName(const QString &piece)
{
    static const QStringList modifiers{QStringLiteral("Ctrl"), QStringLiteral("Alt"),
                                       QStringLiteral("Shift"), QStringLiteral("Meta")};
    return modifiers.contains(piece, Qt::CaseInsensitive);
}

constexpr std::uint32_t kFunctionKey1 = 0x70;
constexpr std::uint32_t kOemPlus = 0xBB;
constexpr std::uint32_t kOemComma = 0xBC;
constexpr std::uint32_t kOemMinus = 0xBD;
constexpr std::uint32_t kOemPeriod = 0xBE;

[[nodiscard]] std::uint32_t namedVirtualKey(const QString &name)
{
    static const QList<std::pair<QLatin1StringView, std::uint32_t>> keys{
        {QLatin1StringView{"Backspace"}, 0x08}, {QLatin1StringView{"Tab"}, 0x09},
        {QLatin1StringView{"Clear"}, 0x0C},     {QLatin1StringView{"Return"}, 0x0D},
        {QLatin1StringView{"Enter"}, 0x0D},     {QLatin1StringView{"Pause"}, 0x13},
        {QLatin1StringView{"CapsLock"}, 0x14},  {QLatin1StringView{"Esc"}, 0x1B},
        {QLatin1StringView{"Space"}, 0x20},     {QLatin1StringView{"PgUp"}, 0x21},
        {QLatin1StringView{"PgDown"}, 0x22},    {QLatin1StringView{"End"}, 0x23},
        {QLatin1StringView{"Home"}, 0x24},      {QLatin1StringView{"Left"}, 0x25},
        {QLatin1StringView{"Up"}, 0x26},        {QLatin1StringView{"Right"}, 0x27},
        {QLatin1StringView{"Down"}, 0x28},      {QLatin1StringView{"Print"}, 0x2C},
        {QLatin1StringView{"Ins"}, 0x2D},       {QLatin1StringView{"Del"}, 0x2E},
        {QLatin1StringView{"Help"}, 0x2F},      {QLatin1StringView{"Menu"}, 0x5D},
        {QLatin1StringView{"NumLock"}, 0x90},   {QLatin1StringView{"ScrollLock"}, 0x91},
    };

    for (const auto &[spelling, code] : keys) {
        if (name.compare(spelling, Qt::CaseInsensitive) == 0) {
            return code;
        }
    }

    return 0;
}

class NoGlobalHotkeys final : public IGlobalHotkeys {
public:
    [[nodiscard]] bool supported() const override
    {
        return false;
    }

    [[nodiscard]] std::vector<GrabOutcome> grab(std::span<const Hotkey> bindings) override
    {
        std::vector<GrabOutcome> outcomes;
        for (const auto &binding : bindings) {
            if (binding.global && !binding.sequence.empty()) {
                outcomes.push_back(GrabOutcome{binding.action, GrabProblem::Unsupported});
            }
        }
        return outcomes;
    }
};

}

std::unique_ptr<IGlobalHotkeys> makeGlobalHotkeys(IGlobalHotkeys::Activated activated)
{
#if defined(_WIN32)

    return makeWin32GlobalHotkeys(std::move(activated));
#elif defined(__APPLE__)

    return makeAppleGlobalHotkeys(std::move(activated));
#elif defined(SOFTPHONE_HAS_X11)

    if (QGuiApplication::platformName().startsWith(QLatin1String("xcb"))) {
        auto x11 = makeX11GlobalHotkeys(std::move(activated));
        if (x11->supported()) {
            return x11;
        }
    }
    return std::make_unique<NoGlobalHotkeys>();
#else
    static_cast<void>(activated);
    return std::make_unique<NoGlobalHotkeys>();
#endif
}

std::uint16_t xModifiers(std::string_view sequence)
{
    std::uint16_t mask = 0;

    for (const auto &part : parts(sequence)) {
        const auto name = part.trimmed();
        if (name.compare(QLatin1String("Ctrl"), Qt::CaseInsensitive) == 0) {
            mask |= kControlMask;
        } else if (name.compare(QLatin1String("Alt"), Qt::CaseInsensitive) == 0) {
            mask |= kMod1Mask;
        } else if (name.compare(QLatin1String("Shift"), Qt::CaseInsensitive) == 0) {
            mask |= kShiftMask;
        } else if (name.compare(QLatin1String("Meta"), Qt::CaseInsensitive) == 0) {

            mask |= kMod4Mask;
        }
    }

    return mask;
}

std::string xKeyName(std::string_view sequence)
{
    if (sequence.empty()) {
        return {};
    }

    const auto last = lastPiece(sequence);

    if (last.isEmpty()) {
        return "plus";
    }

    if (isModifierName(last)) {
        return {};
    }

    return last.toStdString();
}

std::string hotkeyKeyName(std::string_view sequence)
{
    if (sequence.empty()) {
        return {};
    }

    const auto last = lastPiece(sequence);
    return isModifierName(last) ? std::string{} : last.toStdString();
}

std::uint32_t winModifiers(std::string_view sequence)
{
    std::uint32_t flags = 0;

    for (const auto &part : parts(sequence)) {
        const auto name = part.trimmed();
        if (name.compare(QLatin1String("Ctrl"), Qt::CaseInsensitive) == 0) {
            flags |= kControlFlag;
        } else if (name.compare(QLatin1String("Alt"), Qt::CaseInsensitive) == 0) {
            flags |= kAltFlag;
        } else if (name.compare(QLatin1String("Shift"), Qt::CaseInsensitive) == 0) {
            flags |= kShiftFlag;
        } else if (name.compare(QLatin1String("Meta"), Qt::CaseInsensitive) == 0) {

            flags |= kSuperFlag;
        }
    }

    return flags;
}

std::uint32_t winVirtualKey(std::string_view sequence)
{
    if (sequence.empty()) {
        return 0;
    }

    const auto last = lastPiece(sequence);

    if (last.isEmpty()) {
        return kOemPlus;
    }

    if (isModifierName(last)) {
        return 0;
    }

    if (last.size() == 1) {
        const auto letter = last.at(0).toUpper().unicode();
        if ((letter >= u'A' && letter <= u'Z') || (letter >= u'0' && letter <= u'9')) {
            return letter;
        }

        switch (last.at(0).unicode()) {
        case u'+':
            return kOemPlus;
        case u',':
            return kOemComma;
        case u'-':
            return kOemMinus;
        case u'.':
            return kOemPeriod;
        default:

            return 0;
        }
    }

    if (last.at(0).toUpper() == QLatin1Char('F')) {
        bool numeric = false;
        const auto number = last.sliced(1).toUInt(&numeric);
        if (numeric && number >= 1 && number <= 24) {
            return kFunctionKey1 + number - 1;
        }
    }

    return namedVirtualKey(last);
}

std::vector<std::uint16_t> withIgnoredModifiers(std::uint16_t modifiers)
{

    std::vector<std::uint16_t> combinations;
    combinations.reserve(8);

    for (const std::uint16_t lock : {std::uint16_t{0}, kLockMask}) {
        for (const std::uint16_t num : {std::uint16_t{0}, kMod2Mask}) {
            for (const std::uint16_t scroll : {std::uint16_t{0}, kMod5Mask}) {
                combinations.push_back(static_cast<std::uint16_t>(modifiers | lock | num | scroll));
            }
        }
    }

    return combinations;
}

namespace {

[[nodiscard]] std::optional<std::uint32_t> ansiKeyCode(QChar letter)
{
    static const QList<std::pair<QChar, std::uint32_t>> keys{
        {u'a', 0x00}, {u's', 0x01}, {u'd', 0x02}, {u'f', 0x03}, {u'h', 0x04}, {u'g', 0x05},
        {u'z', 0x06}, {u'x', 0x07}, {u'c', 0x08}, {u'v', 0x09}, {u'b', 0x0B}, {u'q', 0x0C},
        {u'w', 0x0D}, {u'e', 0x0E}, {u'r', 0x0F}, {u'y', 0x10}, {u't', 0x11}, {u'1', 0x12},
        {u'2', 0x13}, {u'3', 0x14}, {u'4', 0x15}, {u'6', 0x16}, {u'5', 0x17}, {u'=', 0x18},
        {u'9', 0x19}, {u'7', 0x1A}, {u'-', 0x1B}, {u'8', 0x1C}, {u'0', 0x1D}, {u']', 0x1E},
        {u'o', 0x1F}, {u'u', 0x20}, {u'[', 0x21}, {u'i', 0x22}, {u'p', 0x23}, {u'l', 0x25},
        {u'j', 0x26}, {u'k', 0x28}, {u';', 0x29}, {u',', 0x2B}, {u'/', 0x2C}, {u'n', 0x2D},
        {u'm', 0x2E}, {u'.', 0x2F}, {u'`', 0x32},
    };

    const auto lowered = letter.toLower();
    for (const auto &[key, code] : keys) {
        if (key == lowered) {
            return code;
        }
    }
    return std::nullopt;
}

[[nodiscard]] std::optional<std::uint32_t> namedKeyCode(const QString &name)
{
    static const QList<std::pair<QLatin1StringView, std::uint32_t>> keys{
        {QLatin1StringView{"Return"}, 0x24},    {QLatin1StringView{"Enter"}, 0x24},
        {QLatin1StringView{"Tab"}, 0x30},       {QLatin1StringView{"Space"}, 0x31},
        {QLatin1StringView{"Backspace"}, 0x33}, {QLatin1StringView{"Esc"}, 0x35},
        {QLatin1StringView{"Clear"}, 0x47},     {QLatin1StringView{"Home"}, 0x73},
        {QLatin1StringView{"PgUp"}, 0x74},      {QLatin1StringView{"Del"}, 0x75},
        {QLatin1StringView{"End"}, 0x77},       {QLatin1StringView{"PgDown"}, 0x79},
        {QLatin1StringView{"Left"}, 0x7B},      {QLatin1StringView{"Right"}, 0x7C},
        {QLatin1StringView{"Down"}, 0x7D},      {QLatin1StringView{"Up"}, 0x7E},
        {QLatin1StringView{"Help"}, 0x72},
    };

    for (const auto &[spelling, code] : keys) {
        if (name.compare(spelling, Qt::CaseInsensitive) == 0) {
            return code;
        }
    }
    return std::nullopt;
}

[[nodiscard]] std::optional<std::uint32_t> functionKeyCode(unsigned number)
{
    static const std::array<std::uint32_t, 20> codes{0x7A, 0x78, 0x63, 0x76, 0x60, 0x61, 0x62,
                                                     0x64, 0x65, 0x6D, 0x67, 0x6F, 0x69, 0x6B,
                                                     0x71, 0x6A, 0x40, 0x4F, 0x50, 0x5A};
    if (number < 1 || number > codes.size()) {
        return std::nullopt;
    }
    return codes.at(number - 1);
}

}

std::uint32_t carbonModifiers(std::string_view sequence)
{
    std::uint32_t flags = 0;

    for (const auto &part : parts(sequence)) {
        const auto name = part.trimmed();
        if (name.compare(QLatin1String("Ctrl"), Qt::CaseInsensitive) == 0) {

            flags |= kCommandFlag;
        } else if (name.compare(QLatin1String("Alt"), Qt::CaseInsensitive) == 0) {
            flags |= kOptionFlag;
        } else if (name.compare(QLatin1String("Shift"), Qt::CaseInsensitive) == 0) {
            flags |= kCarbonShiftFlag;
        } else if (name.compare(QLatin1String("Meta"), Qt::CaseInsensitive) == 0) {
            flags |= kCarbonControlFlag;
        }
    }

    return flags;
}

std::optional<std::uint32_t> carbonKeyCode(std::string_view sequence)
{
    if (sequence.empty()) {
        return std::nullopt;
    }

    const auto last = lastPiece(sequence);

    if (last.isEmpty()) {
        return ansiKeyCode(u'=');
    }

    if (isModifierName(last)) {
        return std::nullopt;
    }

    if (last.size() == 1) {

        return ansiKeyCode(last.at(0) == u'+' ? QChar{u'='} : last.at(0));
    }

    if (last.at(0).toUpper() == QLatin1Char('F')) {
        bool numeric = false;
        const auto number = last.sliced(1).toUInt(&numeric);
        if (numeric) {
            return functionKeyCode(number);
        }
    }

    return namedKeyCode(last);
}

}
