// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "screenshots.hpp"

#include <softphone/core/logging.hpp>

#include <QCoreApplication>
#include <QCursor>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QImage>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLoggingCategory>
#include <QQuickWindow>
#include <QScreen>
#include <QTimer>

#if defined(Q_OS_MACOS)
#include <CoreGraphics/CoreGraphics.h>
#endif

#ifdef _WIN32
#include <windows.h>
#endif

#include <algorithm>
#include <cmath>
#include <optional>
#include <string>
#include <utility>

namespace softphone::ui {
namespace {

[[nodiscard]] bool hasSomethingInIt(const QImage &picture)
{
    if (picture.isNull() || picture.width() < 2 || picture.height() < 2) {
        return false;
    }

    const auto first = picture.pixel(0, 0);
    for (int y = 0; y < picture.height(); y += 11) {
        for (int x = 0; x < picture.width(); x += 11) {
            if (picture.pixel(x, y) != first) {
                return true;
            }
        }
    }
    return false;
}

void complain(const QString &name, const std::string &why)
{
    core::Logger::instance().log(core::LogLevel::Warning, "screenshots",
                                 name.toStdString() + ": " + why);
}

constexpr int kForgetThePointerMs = 250;

void parkThePointer(QQuickWindow *window)
{
    QScreen *screen = window->screen();
    if (screen == nullptr) {
        return;
    }

    const QRect within = screen->geometry();
    const QPoint middle = window->frameGeometry().center();

    const int x = middle.x() < within.center().x() ? within.right() - 1 : within.left() + 1;
    const int y = middle.y() < within.center().y() ? within.bottom() - 1 : within.top() + 1;
    QCursor::setPos(screen, QPoint{x, y});
}

void letItHappen(int milliseconds)
{
    QEventLoop waiting;
    QTimer::singleShot(milliseconds, &waiting, &QEventLoop::quit);
    waiting.exec();
}

#ifdef _WIN32

void comeToTheFront(QQuickWindow *window)
{
    auto drawnInto = reinterpret_cast<HWND>(window->winId());
    if (drawnInto == nullptr) {
        return;
    }
    HWND handle = GetAncestor(drawnInto, GA_ROOT);
    if (handle == nullptr) {
        handle = drawnInto;
    }
    SetWindowPos(handle, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

[[nodiscard]] bool nothingIsInFrontOf(QQuickWindow *window, QString &what)
{
    const qreal ratio = window->devicePixelRatio();
    const auto physical = [ratio](const QPoint &logical) {
        return QPoint{static_cast<int>(std::lround(logical.x() * ratio)),
                      static_cast<int>(std::lround(logical.y() * ratio))};
    };
    const QRect frame{
        physical(window->frameGeometry().topLeft()),
        QSize{static_cast<int>(std::lround(window->frameGeometry().width() * ratio)),
              static_cast<int>(std::lround(window->frameGeometry().height() * ratio))}};
    const auto ours = GetCurrentProcessId();
    const QPoint corners[] = {frame.center(), frame.topLeft() + QPoint{8, 8},
                              frame.topRight() + QPoint{-8, 8}, frame.bottomLeft() + QPoint{8, -8},
                              frame.bottomRight() + QPoint{-8, -8}};
    for (const QPoint &where : corners) {
        HWND at = WindowFromPoint(POINT{where.x(), where.y()});
        if (at == nullptr) {
            return false;
        }
        DWORD owner = 0;
        GetWindowThreadProcessId(at, &owner);
        if (owner != ours) {

            wchar_t title[256] = {};
            GetWindowTextW(at, title, 255);
            what = QString::fromWCharArray(title).simplified();
            if (what.isEmpty()) {
                wchar_t klass[128] = {};
                GetClassNameW(at, klass, 127);
                what = QString::fromWCharArray(klass);
            }
            return false;
        }
    }
    return true;
}

#endif

#if defined(Q_OS_MACOS)

[[nodiscard]] CGWindowID numberOf(const QRect &frame)
{
    CFArrayRef every = CGWindowListCopyWindowInfo(
        kCGWindowListOptionOnScreenOnly | kCGWindowListExcludeDesktopElements, kCGNullWindowID);
    if (every == nullptr) {
        return 0;
    }

    const auto asInt = [](CFDictionaryRef entry, CFStringRef key) {
        int value = 0;
        if (const auto *number = static_cast<CFNumberRef>(CFDictionaryGetValue(entry, key));
            number != nullptr) {
            CFNumberGetValue(number, kCFNumberIntType, &value);
        }
        return value;
    };

    CGWindowID found = 0;
    for (CFIndex at = 0; at < CFArrayGetCount(every) && found == 0; ++at) {
        auto entry = static_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(every, at));
        if (asInt(entry, kCGWindowOwnerPID) !=
                static_cast<int>(QCoreApplication::applicationPid()) ||
            asInt(entry, kCGWindowLayer) != 0) {
            continue;
        }

        CGRect bounds{};
        if (!CGRectMakeWithDictionaryRepresentation(
                static_cast<CFDictionaryRef>(CFDictionaryGetValue(entry, kCGWindowBounds)),
                &bounds)) {
            continue;
        }

        if (std::abs(bounds.origin.x - frame.x()) <= 2.0 &&
            std::abs(bounds.origin.y - frame.y()) <= 2.0 &&
            std::abs(bounds.size.width - frame.width()) <= 2.0 &&
            std::abs(bounds.size.height - frame.height()) <= 2.0) {
            found = static_cast<CGWindowID>(asInt(entry, kCGWindowNumber));
        }
    }
    CFRelease(every);
    return found;
}

[[nodiscard]] QImage windowFromTheCompositor(QQuickWindow *window)
{
    const auto number = numberOf(window->frameGeometry());
    if (number == 0) {
        return {};
    }

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    CGImageRef made = CGWindowListCreateImage(CGRectNull, kCGWindowListOptionIncludingWindow,
                                              number, kCGWindowImageBoundsIgnoreFraming);
#pragma clang diagnostic pop
    if (made == nullptr) {
        return {};
    }

    QImage picture;
    if (CGImageGetBitsPerPixel(made) == 32 && CGImageGetBitsPerComponent(made) == 8) {
        CFDataRef bytes = CGDataProviderCopyData(CGImageGetDataProvider(made));
        if (bytes != nullptr) {

            picture = QImage{CFDataGetBytePtr(bytes), static_cast<int>(CGImageGetWidth(made)),
                             static_cast<int>(CGImageGetHeight(made)),
                             static_cast<int>(CGImageGetBytesPerRow(made)),
                             QImage::Format_ARGB32_Premultiplied}
                          .copy();
            CFRelease(bytes);
        }
    }
    CGImageRelease(made);
    return picture;
}

#endif

[[nodiscard]] QImage windowAndItsFrame(QQuickWindow *window)
{
#if defined(Q_OS_MACOS)

    if (auto asked = windowFromTheCompositor(window); !asked.isNull()) {
        return asked;
    }
#endif

    QScreen *screen = window->screen();
    if (screen == nullptr) {
        return {};
    }

    const qreal ratio = window->devicePixelRatio();

    const auto inDevicePixels = [ratio](const QRect &logical) {
        return QRect{QPoint{static_cast<int>(std::lround(logical.x() * ratio)),
                            static_cast<int>(std::lround(logical.y() * ratio))},
                     QSize{static_cast<int>(std::lround(logical.width() * ratio)),
                           static_cast<int>(std::lround(logical.height() * ratio))}};
    };

    QRect wanted =
        inDevicePixels(window->frameGeometry().translated(-screen->geometry().topLeft()));

#ifdef _WIN32

    wanted.setTop(wanted.top() + 1);
#endif

    QImage whole = screen->grabWindow(0).toImage();
    if (whole.isNull() || !whole.rect().contains(wanted)) {
        return {};
    }
    return whole.copy(wanted);
}

}

QStringList screenshotsWanted(const QStringList &arguments)
{
    const auto at = arguments.indexOf(QString::fromUtf8(kScreenshotOption));
    if (at < 0 || at + 1 >= arguments.size()) {
        return {};
    }

    QStringList named;
    for (const auto &one : arguments.at(at + 1).split(QLatin1Char{','})) {
        const auto trimmed = one.trimmed();
        if (!trimmed.isEmpty()) {
            named.append(trimmed);
        }
    }
    return named;
}

QString screenshotsInto(const QStringList &arguments)
{
    const auto at = arguments.indexOf(QString::fromUtf8(kScreenshotIntoOption));
    if (at < 0 || at + 1 >= arguments.size()) {
        return {};
    }
    return arguments.at(at + 1);
}

QString screenshotsCapturing(const QStringList &arguments)
{
    const auto at = arguments.indexOf(QString::fromUtf8(kScreenshotCapturingOption));
    if (at < 0 || at + 1 >= arguments.size()) {
        return {};
    }
    return arguments.at(at + 1);
}

QString screenshotsContent(const QStringList &arguments)
{
    const auto at = arguments.indexOf(QString::fromUtf8(kScreenshotContentOption));
    if (at < 0 || at + 1 >= arguments.size()) {
        return {};
    }
    return arguments.at(at + 1);
}

namespace {

[[nodiscard]] QVariantMap officeIn(const QString &path)
{
    if (path.isEmpty()) {
        return {};
    }

    QFile file{path};
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "screenshots: cannot read the office at" << path << file.errorString();
        return {};
    }

    QJsonParseError trouble{};
    const auto document = QJsonDocument::fromJson(file.readAll(), &trouble);
    if (trouble.error != QJsonParseError::NoError || !document.isObject()) {
        qWarning() << "screenshots: the office at" << path
                   << "is not an object:" << trouble.errorString();
        return {};
    }
    return document.object().toVariantMap();
}

}

Screenshots::Screenshots(QStringList wanted, QString into, QString capturing, QString content,
                         QObject *parent)
    : QObject{parent}, wanted_{std::move(wanted)}, into_{std::move(into)},
      capturing_{std::move(capturing)}, content_{officeIn(content)}
{

    if (wanted_.isEmpty() || into_.isEmpty()) {
        wanted_.clear();
        into_.clear();
        capturing_.clear();
    }
}

bool Screenshots::take(const QString &name, QQuickWindow *window)
{
    if (into_.isEmpty()) {
        return false;
    }

    if (window == nullptr) {
        complain(name, "there is no such window");
        return false;
    }

    if (!QDir{}.mkpath(into_)) {
        complain(name, "cannot make " + into_.toStdString());
        return false;
    }

    window->raise();
#ifdef _WIN32
    comeToTheFront(window);
#endif

    parkThePointer(window);
    letItHappen(kForgetThePointerMs);

#ifdef _WIN32

    if (QString what; !nothingIsInFrontOf(window, what)) {
        complain(name, "the desktop draws " + what.toStdString() +
                           " over this window, so the picture would have been of that");
        return false;
    }
#endif

    const QImage picture = windowAndItsFrame(window);
    if (!hasSomethingInIt(picture)) {
        complain(name, "the window grabbed as one flat colour, so it had not painted or it "
                       "does not fit the screen");
        return false;
    }

    const auto path = QDir{into_}.filePath(name + QStringLiteral(".png"));
    if (!picture.save(path, "PNG")) {
        complain(name, "cannot write " + path.toStdString());
        return false;
    }

    core::Logger::instance().log(core::LogLevel::Info, "screenshots",
                                 name.toStdString() + ": " + std::to_string(picture.width()) + "x" +
                                     std::to_string(picture.height()) + " into " +
                                     path.toStdString());
    return true;
}

}
