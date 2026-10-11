// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "link_handler_win32.hpp"

#include "autostart.hpp"

#include <softphone/core/dial_link.hpp>
#include <softphone/core/logging.hpp>
#include <softphone/core/paths.hpp>

#include <QDesktopServices>
#include <QSettings>
#include <QString>
#include <QUrl>

#include <utility>

namespace softphone::ui {
namespace {

const auto kClasses = QStringLiteral("HKEY_CURRENT_USER\\Software\\Classes");
const auto kCapabilities = QStringLiteral("HKEY_CURRENT_USER\\Software\\AI Softphone");
const auto kRegistered = QStringLiteral("HKEY_CURRENT_USER\\Software\\RegisteredApplications");

[[nodiscard]] QString progId()
{
    return QString::fromUtf8(kWindowsProgId.data(), static_cast<qsizetype>(kWindowsProgId.size()));
}

[[nodiscard]] QString chosenFor(std::string_view scheme)
{
    QSettings chosen{
        QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\Shell\\"
                       "Associations\\UrlAssociations\\%1\\UserChoice")
            .arg(QString::fromUtf8(scheme.data(), static_cast<qsizetype>(scheme.size()))),
        QSettings::NativeFormat};
    return chosen.value(QStringLiteral("ProgId")).toString();
}

}

WindowsLinkHandler::WindowsLinkHandler(std::filesystem::path binary) : binary_{std::move(binary)} {}

LinkHandlerState WindowsLinkHandler::state() const
{
    LinkHandlerState answer;
    answer.possible = true;

    const auto ours = progId();
    answer.preferred = true;
    for (const auto scheme : core::kDialSchemes) {
        const auto chosen = chosenFor(scheme);
        if (chosen != ours) {
            answer.preferred = false;

            if (answer.heldBy.empty() && !chosen.isEmpty()) {
                answer.heldBy = chosen.toStdString();
            }
        }
    }
    return answer;
}

bool WindowsLinkHandler::declareWhatWeCanOpen()
{
    const auto ours = progId();
    const auto command = QString::fromStdString(urlOpenCommand(binary_));
    const auto name = QStringLiteral("AI Softphone");

    QSettings classes{kClasses, QSettings::NativeFormat};

    classes.setValue(ours + QStringLiteral("/."), name);
    classes.setValue(ours + QStringLiteral("/URL Protocol"), QString{});
    classes.setValue(ours + QStringLiteral("/DefaultIcon/."),
                     QStringLiteral("\"%1\",0").arg(QString::fromStdString(core::asUtf8(binary_))));
    classes.setValue(ours + QStringLiteral("/shell/open/command/."), command);

    for (const auto scheme : core::kDialSchemes) {
        const auto key = QString::fromUtf8(scheme.data(), static_cast<qsizetype>(scheme.size()));
        classes.setValue(key + QStringLiteral("/."), name);
        classes.setValue(key + QStringLiteral("/URL Protocol"), QString{});
        classes.setValue(key + QStringLiteral("/shell/open/command/."), command);
    }

    QSettings capabilities{kCapabilities, QSettings::NativeFormat};
    capabilities.setValue(QStringLiteral("Capabilities/ApplicationName"), name);
    capabilities.setValue(QStringLiteral("Capabilities/ApplicationDescription"),
                          QStringLiteral("Make and take SIP calls"));
    for (const auto scheme : core::kDialSchemes) {
        capabilities.setValue(
            QStringLiteral("Capabilities/UrlAssociations/%1")
                .arg(QString::fromUtf8(scheme.data(), static_cast<qsizetype>(scheme.size()))),
            ours);
    }

    QSettings registered{kRegistered, QSettings::NativeFormat};
    registered.setValue(name, QStringLiteral("Software\\AI Softphone\\Capabilities"));

    classes.sync();
    capabilities.sync();
    registered.sync();

    const auto wrote = classes.status() == QSettings::NoError &&
                       capabilities.status() == QSettings::NoError &&
                       registered.status() == QSettings::NoError;
    if (!wrote) {
        core::Logger::instance().log(core::LogLevel::Warning, "ui",
                                     "the registry would not take this phone's link handler");
    }
    return wrote;
}

bool WindowsLinkHandler::claim()
{
    if (!declareWhatWeCanOpen()) {
        return false;
    }

    const auto opened = QDesktopServices::openUrl(QUrl{QStringLiteral("ms-settings:defaultapps")});
    if (!opened) {
        core::Logger::instance().log(core::LogLevel::Warning, "ui",
                                     "could not open the default applications settings");
    }

    return true;
}

std::unique_ptr<ILinkHandler> makeLinkHandler()
{
    return std::make_unique<WindowsLinkHandler>(runningBinary());
}

}
