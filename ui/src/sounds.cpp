// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "sounds.hpp"

#include <softphone/core/logging.hpp>
#include <softphone/core/paths.hpp>
#include <softphone/core/ringtones.hpp>

#include <QFile>
#include <QFileInfo>
#include <QString>

#include <filesystem>
#include <system_error>

namespace softphone::ui {
namespace {

[[nodiscard]] QString fileNameFor(core::CallTone tone)
{
    switch (tone) {
    case core::CallTone::Ringtone:

        break;
    case core::CallTone::Ringback:
        return QStringLiteral("ringback.wav");
    case core::CallTone::Disconnect:
        return QStringLiteral("disconnect.wav");
    case core::CallTone::AutoAnswered:
        return QStringLiteral("auto-answer.wav");
    case core::CallTone::Busy:
        return QStringLiteral("busy.wav");
    case core::CallTone::None:
        break;
    }
    return {};
}

[[nodiscard]] std::string unpack(const QString &fileName)
{
    const auto directory = core::cacheDirectory();
    if (directory.empty()) {
        return {};
    }

    std::error_code failed;
    std::filesystem::create_directories(directory, failed);

    const auto target = directory / core::fromUtf8(fileName.toStdString());
    const auto targetName = QString::fromStdString(core::asUtf8(target));

    QFile resource{QStringLiteral(":/sounds/") + fileName};
    if (!resource.open(QIODevice::ReadOnly)) {
        return {};
    }

    const auto wanted = resource.readAll();

    if (QFileInfo::exists(targetName)) {
        QFile existing{targetName};
        if (existing.open(QIODevice::ReadOnly) && existing.readAll() == wanted) {
            return core::asUtf8(target);
        }
    }

    QFile output{targetName};
    if (!output.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return {};
    }

    if (output.write(wanted) < 0) {
        return {};
    }
    output.close();

    return core::asUtf8(target);
}

}

std::string toneFile(core::CallTone tone, const core::RingtoneChoice &ringtone)
{
    if (tone == core::CallTone::Ringtone) {
        return ringtoneFile(ringtone);
    }

    const auto fileName = fileNameFor(tone);
    if (fileName.isEmpty()) {
        return {};
    }
    return unpack(fileName);
}

std::string ringtoneFile(const core::RingtoneChoice &ringtone)
{
    const auto bundled = [](std::string_view id) {
        const auto file = core::bundledRingtoneFile(id);
        return file.empty()
                   ? std::string{}
                   : unpack(QString::fromUtf8(file.data(), static_cast<qsizetype>(file.size())));
    };

    if (!ringtone.ownFile()) {

        auto file = bundled(ringtone.bundled);
        return file.empty() ? bundled(core::defaultRingtone()) : file;
    }

    const auto chosen = core::fromUtf8(ringtone.file);
    if (const auto fault = core::inspectRingtoneFile(chosen); fault != core::RingtoneFault::None) {
        core::Logger::instance().log(core::LogLevel::Warning, "ui",
                                     "the chosen ring cannot be played, so the phone rings with "
                                     "its own: " +
                                         ringtone.file);
        return bundled(core::defaultRingtone());
    }

    return ringtone.file;
}

std::string testSoundFile()
{
    return unpack(QStringLiteral("test.wav"));
}

std::string noticeFile(const std::string &chosen)
{
    if (chosen.empty()) {
        return unpack(QStringLiteral("recording-notice.wav"));
    }

    const auto file = core::fromUtf8(chosen);
    if (const auto fault = core::inspectRingtoneFile(file); fault != core::RingtoneFault::None) {
        core::Logger::instance().log(core::LogLevel::Warning, "ui",
                                     "the chosen recording notice cannot be played, so the "
                                     "shipped one is used instead: " +
                                         chosen);
        return unpack(QStringLiteral("recording-notice.wav"));
    }

    return chosen;
}

std::string shippedNoticeFile()
{
    return unpack(QStringLiteral("recording-notice.wav"));
}

std::string recordingToneFile()
{
    return unpack(QStringLiteral("recording-tone.wav"));
}

std::string dtmfFile(char digit)
{

    const auto name = [digit]() -> QString {
        if (digit >= '0' && digit <= '9') {
            return QStringLiteral("dtmf-%1.wav").arg(QChar::fromLatin1(digit));
        }
        if (digit == '*') {
            return QStringLiteral("dtmf-star.wav");
        }
        if (digit == '#') {
            return QStringLiteral("dtmf-hash.wav");
        }
        return {};
    }();

    return name.isEmpty() ? std::string{} : unpack(name);
}

bool toneLoops(core::CallTone tone) noexcept
{
    switch (tone) {
    case core::CallTone::Ringtone:
    case core::CallTone::Ringback:

        return true;
    case core::CallTone::Disconnect:
    case core::CallTone::AutoAnswered:
    case core::CallTone::Busy:

    case core::CallTone::None:
        break;
    }
    return false;
}

}
