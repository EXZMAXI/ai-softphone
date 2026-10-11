// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "desktop_platform.hpp"

#include <softphone/core/logging.hpp>

#include <QMargins>
#include <QScreen>
#include <QTimer>
#include <QWindow>
#include <cmath>

#include <windows.h>

#include <dwmapi.h>
#include <shellapi.h>
#include <shobjidl.h>

#include <cstdint>
#include <memory>
#include <string>

namespace softphone::ui::platform {
namespace {

struct ComDeleter {
    void operator()(ITaskbarList *list) const noexcept
    {
        if (list != nullptr) {
            list->Release();
        }
    }
};

[[nodiscard]] std::unique_ptr<ITaskbarList, ComDeleter> taskbar()
{
    ITaskbarList *raw = nullptr;
    if (FAILED(::CoCreateInstance(CLSID_TaskbarList, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(&raw)))) {
        return nullptr;
    }

    std::unique_ptr<ITaskbarList, ComDeleter> list{raw};

    if (FAILED(list->HrInit())) {
        return nullptr;
    }
    return list;
}

}

bool controlsTaskbarEntry()
{

    return true;
}

namespace {

HWND reservedFor = nullptr;

[[nodiscard]] UINT appbarMessage()
{
    static const UINT kCallback = ::RegisterWindowMessageW(L"AiSoftphoneAppBar");
    return kCallback;
}

}

void giveBackTheRoom()
{
    if (reservedFor == nullptr) {
        return;
    }

    APPBARDATA bar{};
    bar.cbSize = sizeof(bar);
    bar.hWnd = reservedFor;
    bar.uCallbackMessage = appbarMessage();
    ::SHAppBarMessage(ABM_REMOVE, &bar);

    reservedFor = nullptr;
}

bool edgesCanBeReserved()
{

    return true;
}

bool reserveEdge(QWindow &window, ScreenEdge edge, int thickness)
{

    auto *handle = reinterpret_cast<HWND>(window.winId());
    if (handle == nullptr) {
        return false;
    }

    APPBARDATA bar{};
    bar.cbSize = sizeof(bar);
    bar.hWnd = handle;
    bar.uCallbackMessage = appbarMessage();

    if (thickness <= 0) {
        ::SHAppBarMessage(ABM_REMOVE, &bar);
        reservedFor = nullptr;
        return true;
    }

    ::SHAppBarMessage(ABM_NEW, &bar);
    reservedFor = handle;

    const auto *screen = window.screen();
    const auto ratio = screen != nullptr ? screen->devicePixelRatio() : 1.0;
    const auto deep = static_cast<LONG>(std::lround(thickness * ratio));
    const auto area = screen != nullptr ? screen->geometry() : QRect{};

    bar.uEdge = edge == ScreenEdge::Top ? ABE_TOP : ABE_BOTTOM;
    bar.rc.left = static_cast<LONG>(std::lround(area.left() * ratio));
    bar.rc.right = static_cast<LONG>(std::lround((area.right() + 1) * ratio));
    if (edge == ScreenEdge::Top) {
        bar.rc.top = static_cast<LONG>(std::lround(area.top() * ratio));
        bar.rc.bottom = bar.rc.top + deep;
    } else {
        bar.rc.bottom = static_cast<LONG>(std::lround((area.bottom() + 1) * ratio));
        bar.rc.top = bar.rc.bottom - deep;
    }

    ::SHAppBarMessage(ABM_QUERYPOS, &bar);
    ::SHAppBarMessage(ABM_SETPOS, &bar);

    window.setGeometry(
        QRect{QPoint{static_cast<int>(std::lround(bar.rc.left / ratio)),
                     static_cast<int>(std::lround(bar.rc.top / ratio))},
              QSize{static_cast<int>(std::lround((bar.rc.right - bar.rc.left) / ratio)),
                    static_cast<int>(std::lround((bar.rc.bottom - bar.rc.top) / ratio))}});
    return true;
}

void releaseEdge(QWindow &window)
{

    auto *handle = reinterpret_cast<HWND>(window.winId());
    if (handle == nullptr) {
        return;
    }

    APPBARDATA bar{};
    bar.cbSize = sizeof(bar);
    bar.hWnd = handle;
    ::SHAppBarMessage(ABM_REMOVE, &bar);

    if (reservedFor == handle) {
        reservedFor = nullptr;
    }
}

void setTaskbarEntry(QWindow &window, bool shown)
{
    const auto list = taskbar();
    if (list == nullptr) {
        core::Logger::instance().log(core::LogLevel::Debug, "ui",
                                     "the shell would not say whether a window is in the task bar");
        return;
    }

    const auto handle = reinterpret_cast<HWND>(window.winId());

    const HRESULT told = shown ? list->AddTab(handle) : list->DeleteTab(handle);
    if (FAILED(told)) {
        core::Logger::instance().log(core::LogLevel::Debug, "ui",
                                     "the shell refused to change a window's task bar button");
    }
}

bool pointerHeld()
{

    constexpr int kButtons[] = {VK_LBUTTON, VK_RBUTTON, VK_MBUTTON};

    for (const int button : kButtons) {

        if ((::GetAsyncKeyState(button) & 0x8000) != 0) {
            return true;
        }
    }
    return false;
}

namespace {

[[nodiscard]] std::string titleOf(HWND window)
{
    const int length = ::GetWindowTextLengthW(window);
    if (length <= 0) {
        return {};
    }

    std::wstring wide(static_cast<std::size_t>(length) + 1, L'\0');
    const int wrote = ::GetWindowTextW(window, wide.data(), length + 1);
    if (wrote <= 0) {
        return {};
    }
    wide.resize(static_cast<std::size_t>(wrote));

    const int needed =
        ::WideCharToMultiByte(CP_UTF8, 0, wide.data(), wrote, nullptr, 0, nullptr, nullptr);
    if (needed <= 0) {
        return {};
    }
    std::string narrowed(static_cast<std::size_t>(needed), '\0');
    ::WideCharToMultiByte(CP_UTF8, 0, wide.data(), wrote, narrowed.data(), needed, nullptr,
                          nullptr);
    return narrowed;
}

[[nodiscard]] std::string coveredBy(HWND window)
{
    for (HWND above = ::GetTopWindow(nullptr); above != nullptr;
         above = ::GetWindow(above, GW_HWNDNEXT)) {
        if (above == window) {
            return {};
        }
        if (::IsWindowVisible(above) == FALSE || ::IsIconic(above) != FALSE) {
            continue;
        }

        auto title = titleOf(above);
        if (title.empty()) {
            continue;
        }

        const auto style = static_cast<DWORD>(::GetWindowLongPtrW(above, GWL_EXSTYLE));
        if ((style & WS_EX_TOPMOST) != 0U) {
            title += " (always on top)";
        }
        return title;
    }
    return {};
}

[[nodiscard]] std::string whereAndWhether(HWND window)
{
    RECT rect{};
    if (::GetWindowRect(window, &rect) == FALSE) {
        return "(no rectangle)";
    }

    auto where = "at " + std::to_string(rect.left) + "," + std::to_string(rect.top) + " " +
                 std::to_string(rect.right - rect.left) + "x" +
                 std::to_string(rect.bottom - rect.top);

    if (::IsWindowVisible(window) == FALSE) {
        where += ", HIDDEN";
    }
    if (::MonitorFromWindow(window, MONITOR_DEFAULTTONULL) == nullptr) {
        where += ", on no monitor";
    }
    return where;
}

[[nodiscard]] DWORD cloaking(HWND window)
{
    DWORD cloaked = 0;
    if (::DwmGetWindowAttribute(window, DWMWA_CLOAKED, &cloaked, sizeof(cloaked)) != S_OK) {
        return 0;
    }
    return cloaked;
}

[[nodiscard]] std::string whatIsThereInstead(HWND window)
{
    RECT box{};
    if (::GetWindowRect(window, &box) == FALSE) {
        return {};
    }

    const POINT middle{(box.left + box.right) / 2, (box.top + box.bottom) / 2};
    HWND there = ::WindowFromPoint(middle);
    if (there == nullptr) {
        return "nothing at all";
    }

    there = ::GetAncestor(there, GA_ROOT);
    if (there == window) {
        return {};
    }

    auto title = titleOf(there);
    return title.empty() ? std::string{"an untitled window"} : title;
}

void said(const char *how, HWND window)
{

    auto message = std::string{"brought \""} + titleOf(window) + "\" forward: " + how + " " +
                   whereAndWhether(window);
    if (const auto over = coveredBy(window); !over.empty()) {
        message += " — but it is still under: " + over;
    }
    if (const auto cloaked = cloaking(window); cloaked != 0) {
        message += cloaked == DWM_CLOAKED_SHELL
                       ? " — and the shell is not drawing it: another virtual desktop"
                       : " — and it is cloaked (" + std::to_string(cloaked) + ")";
    }
    if (const auto instead = whatIsThereInstead(window); !instead.empty()) {
        message += " — and what is on the screen where it is: " + instead;
    }
    core::Logger::instance().log(core::LogLevel::Debug, "ui", message);

    QTimer::singleShot(500, [window]() {
        const auto over = coveredBy(window);
        core::Logger::instance().log(core::LogLevel::Debug, "ui",
                                     over.empty()
                                         ? std::string{"half a second later: still in front"}
                                         : "half a second later: now under: " + over);
    });
}

}

void bringToFront(QWindow &window)
{
    const auto handle = reinterpret_cast<HWND>(window.winId());

    if (::IsIconic(handle) != FALSE) {
        ::ShowWindow(handle, SW_RESTORE);
    }

    static_cast<void>(
        ::SetWindowPos(handle, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE));

    static_cast<void>(::SetForegroundWindow(handle));

    const HWND inFront = ::GetForegroundWindow();
    const DWORD theirs = ::GetWindowThreadProcessId(inFront, nullptr);
    const DWORD ours = ::GetCurrentThreadId();

    if (inFront != nullptr && theirs != ours && ::AttachThreadInput(theirs, ours, TRUE) != FALSE) {
        ::BringWindowToTop(handle);
        static_cast<void>(::SetForegroundWindow(handle));
        static_cast<void>(::AttachThreadInput(theirs, ours, FALSE));
    }

    DWORD lock = 0;
    if (::SystemParametersInfoW(SPI_GETFOREGROUNDLOCKTIMEOUT, 0, &lock, 0) != FALSE && lock != 0) {
        const auto set = [](DWORD value) {
            static_cast<void>(::SystemParametersInfoW(
                SPI_SETFOREGROUNDLOCKTIMEOUT, 0,
                reinterpret_cast<void *>(static_cast<std::uintptr_t>(value)), 0));
        };

        set(0);
        static_cast<void>(::SetForegroundWindow(handle));
        set(lock);
    }

    if (!whatIsThereInstead(handle).empty()) {
        static_cast<void>(::SetWindowPos(handle, HWND_TOPMOST, 0, 0, 0, 0,
                                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE));
        static_cast<void>(::SetWindowPos(handle, HWND_NOTOPMOST, 0, 0, 0, 0,
                                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE));
    }

    const bool inFrontNow = ::GetForegroundWindow() == handle;

    if (!inFrontNow) {
        FLASHWINFO flash{};
        flash.cbSize = sizeof(flash);
        flash.hwnd = handle;
        flash.dwFlags = FLASHW_ALL | FLASHW_TIMERNOFG;
        flash.uCount = 3;
        static_cast<void>(::FlashWindowEx(&flash));
    }

    said(inFrontNow ? "every step, and it is in front" : "every step, and it still is not", handle);
}

void allowAnotherProcessToRaise()
{

    static_cast<void>(::AllowSetForegroundWindow(ASFW_ANY));
}

std::optional<QPoint> whereOnScreen(const QWindow &window)
{

    static_cast<void>(window);
    return std::nullopt;
}

void askForFrameOffset(QWindow & )
{

}

std::optional<QPoint> frameOffset(const QWindow &window)
{

    const QMargins frame = window.frameMargins();
    return QPoint{frame.left(), frame.top()};
}

}
