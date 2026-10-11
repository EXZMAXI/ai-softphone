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

#include <QGuiApplication>
#include <QScreen>
#include <QTimer>
#include <QWindow>
#include <QtGui/qguiapplication_platform.h>

#include <xcb/xcb.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <memory>
#include <optional>
#include <vector>

namespace softphone::ui::platform {
namespace {

using X11Application = QNativeInterface::QX11Application;

[[nodiscard]] xcb_connection_t *connection()
{
    auto *x11 = qGuiApp != nullptr ? qGuiApp->nativeInterface<X11Application>() : nullptr;
    return x11 != nullptr ? x11->connection() : nullptr;
}

[[nodiscard]] xcb_atom_t atom(xcb_connection_t *connection, const char *name)
{
    const auto cookie =
        xcb_intern_atom(connection, 0, static_cast<std::uint16_t>(std::strlen(name)), name);

    std::unique_ptr<xcb_intern_atom_reply_t, decltype(&free)> reply{
        xcb_intern_atom_reply(connection, cookie, nullptr), &free};

    return reply ? reply->atom : static_cast<xcb_atom_t>(XCB_ATOM_NONE);
}

[[nodiscard]] std::vector<xcb_atom_t> statesOf(xcb_connection_t *x11, xcb_window_t id)
{
    const auto state = atom(x11, "_NET_WM_STATE");
    if (state == XCB_ATOM_NONE) {
        return {};
    }

    const auto cookie = xcb_get_property(x11, 0, id, state, XCB_ATOM_ATOM, 0, 32);

    std::unique_ptr<xcb_get_property_reply_t, decltype(&free)> reply{
        xcb_get_property_reply(x11, cookie, nullptr), &free};

    if (!reply) {
        return {};
    }

    const auto *atoms = static_cast<const xcb_atom_t *>(xcb_get_property_value(reply.get()));
    const auto count =
        static_cast<std::size_t>(xcb_get_property_value_length(reply.get())) / sizeof(xcb_atom_t);

    return std::vector<xcb_atom_t>{atoms, atoms + count};
}

void applyOnX11(QWindow &window, bool shown)
{
    auto *x11 = connection();
    if (x11 == nullptr) {
        return;
    }

    const auto state = atom(x11, "_NET_WM_STATE");
    const auto skipTaskbar = atom(x11, "_NET_WM_STATE_SKIP_TASKBAR");
    const auto skipPager = atom(x11, "_NET_WM_STATE_SKIP_PAGER");

    if (state == XCB_ATOM_NONE || skipTaskbar == XCB_ATOM_NONE) {
        return;
    }

    const auto id = static_cast<xcb_window_t>(window.winId());
    const auto root = xcb_setup_roots_iterator(xcb_get_setup(x11)).data->root;

    if (!window.isVisible()) {
        auto atoms = statesOf(x11, id);

        std::erase(atoms, skipTaskbar);
        std::erase(atoms, skipPager);

        if (!shown) {
            atoms.push_back(skipTaskbar);
            atoms.push_back(skipPager);
        }

        xcb_change_property(x11, XCB_PROP_MODE_REPLACE, id, state, XCB_ATOM_ATOM, 32,
                            static_cast<std::uint32_t>(atoms.size()), atoms.data());
        xcb_flush(x11);
        return;
    }

    {

        xcb_client_message_event_t message{};
        message.response_type = XCB_CLIENT_MESSAGE;
        message.format = 32;
        message.window = id;
        message.type = state;
        message.data.data32[0] = shown ? 0U : 1U;
        message.data.data32[1] = skipTaskbar;
        message.data.data32[2] = skipPager;
        message.data.data32[3] = 1U;
        message.data.data32[4] = 0U;

        xcb_send_event(x11, 0, root,
                       XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT,
                       reinterpret_cast<const char *>(&message));
    }

    xcb_flush(x11);
}

constexpr int kMapMs = 300;

[[nodiscard]] std::optional<bool> skippingTaskbar(const QWindow &window)
{
    auto *x11 = connection();
    if (x11 == nullptr) {
        return std::nullopt;
    }

    const auto skipTaskbar = atom(x11, "_NET_WM_STATE_SKIP_TASKBAR");
    if (skipTaskbar == XCB_ATOM_NONE) {
        return std::nullopt;
    }

    const auto atoms = statesOf(x11, static_cast<xcb_window_t>(window.winId()));
    return std::ranges::find(atoms, skipTaskbar) != atoms.end();
}

void standAsADock(xcb_connection_t *x11, xcb_window_t id, bool dock)
{
    const auto kind = atom(x11, "_NET_WM_WINDOW_TYPE");
    const auto asDock = atom(x11, "_NET_WM_WINDOW_TYPE_DOCK");
    const auto asNormal = atom(x11, "_NET_WM_WINDOW_TYPE_NORMAL");
    if (kind == XCB_ATOM_NONE || asDock == XCB_ATOM_NONE || asNormal == XCB_ATOM_NONE) {
        return;
    }

    const auto wanted = dock ? asDock : asNormal;

    const auto cookie = xcb_get_property(x11, 0, id, kind, XCB_ATOM_ATOM, 0, 32);
    std::unique_ptr<xcb_get_property_reply_t, decltype(&free)> reply{
        xcb_get_property_reply(x11, cookie, nullptr), &free};

    if (reply) {
        const auto *kinds = static_cast<const xcb_atom_t *>(xcb_get_property_value(reply.get()));
        const auto count = static_cast<std::size_t>(xcb_get_property_value_length(reply.get())) /
                           sizeof(xcb_atom_t);
        if (count > 0 && kinds[0] == wanted) {
            return;
        }
    }

    const std::array<xcb_atom_t, 1> kinds{wanted};
    xcb_change_property(x11, XCB_PROP_MODE_REPLACE, id, kind, XCB_ATOM_ATOM, 32,
                        static_cast<std::uint32_t>(kinds.size()), kinds.data());
}

}

namespace {

xcb_window_t struttedWindow = XCB_WINDOW_NONE;

}

void giveBackTheRoom()
{
    auto *x11 = connection();
    if (x11 == nullptr || struttedWindow == XCB_WINDOW_NONE) {
        return;
    }

    const auto partial = atom(x11, "_NET_WM_STRUT_PARTIAL");
    const auto plain = atom(x11, "_NET_WM_STRUT");
    if (partial != XCB_ATOM_NONE && plain != XCB_ATOM_NONE) {
        standAsADock(x11, struttedWindow, false);
        xcb_delete_property(x11, struttedWindow, partial);
        xcb_delete_property(x11, struttedWindow, plain);
        xcb_flush(x11);
    }
    struttedWindow = XCB_WINDOW_NONE;
}

bool edgesCanBeReserved()
{

    return connection() != nullptr;
}

bool reserveEdge(QWindow &window, ScreenEdge edge, int thickness)
{
    auto *x11 = connection();
    if (x11 == nullptr) {
        return false;
    }

    const auto partial = atom(x11, "_NET_WM_STRUT_PARTIAL");
    const auto plain = atom(x11, "_NET_WM_STRUT");
    if (partial == XCB_ATOM_NONE || plain == XCB_ATOM_NONE) {
        return false;
    }

    const auto id = static_cast<xcb_window_t>(window.winId());

    const auto *screen = window.screen();
    const auto ratio = screen != nullptr ? screen->devicePixelRatio() : 1.0;
    const auto deep = static_cast<std::uint32_t>(std::lround(thickness * ratio));

    if (deep == 0) {
        standAsADock(x11, id, false);
        xcb_delete_property(x11, id, partial);
        xcb_delete_property(x11, id, plain);
        xcb_flush(x11);
        struttedWindow = XCB_WINDOW_NONE;
        return true;
    }

    standAsADock(x11, id, true);
    struttedWindow = id;

    const auto geometry = screen != nullptr ? screen->geometry() : QRect{};
    const auto from = static_cast<std::uint32_t>(std::lround(geometry.left() * ratio));
    const auto to = static_cast<std::uint32_t>(std::lround(geometry.right() * ratio));

    const auto rootHeight = static_cast<std::int32_t>(
        xcb_setup_roots_iterator(xcb_get_setup(x11)).data->height_in_pixels);
    const auto top = static_cast<std::int32_t>(std::lround(geometry.top() * ratio));
    const auto below =
        static_cast<std::int32_t>(std::lround((geometry.top() + geometry.height()) * ratio));

    std::array<std::uint32_t, 12> strut{};
    if (edge == ScreenEdge::Top) {
        strut[2] = static_cast<std::uint32_t>(top + static_cast<std::int32_t>(deep));
        strut[8] = from;
        strut[9] = to;
    } else {
        strut[3] = static_cast<std::uint32_t>(std::max(0, rootHeight - below) +
                                              static_cast<std::int32_t>(deep));
        strut[10] = from;
        strut[11] = to;
    }

    xcb_change_property(x11, XCB_PROP_MODE_REPLACE, id, partial, XCB_ATOM_CARDINAL, 32,
                        static_cast<std::uint32_t>(strut.size()), strut.data());

    const std::array<std::uint32_t, 4> plainStrut{strut[0], strut[1], strut[2], strut[3]};
    xcb_change_property(x11, XCB_PROP_MODE_REPLACE, id, plain, XCB_ATOM_CARDINAL, 32,
                        static_cast<std::uint32_t>(plainStrut.size()), plainStrut.data());

    xcb_flush(x11);
    return true;
}

void releaseEdge(QWindow &window)
{
    auto *x11 = connection();
    if (x11 == nullptr) {
        return;
    }

    const auto id = static_cast<xcb_window_t>(window.winId());

    standAsADock(x11, id, false);

    const auto partial = atom(x11, "_NET_WM_STRUT_PARTIAL");
    const auto plain = atom(x11, "_NET_WM_STRUT");
    if (partial != XCB_ATOM_NONE) {
        xcb_delete_property(x11, id, partial);
    }
    if (plain != XCB_ATOM_NONE) {
        xcb_delete_property(x11, id, plain);
    }

    if (struttedWindow == id) {
        struttedWindow = XCB_WINDOW_NONE;
    }

    const auto state = atom(x11, "_NET_WM_STATE");
    const auto above = atom(x11, "_NET_WM_STATE_ABOVE");
    if (state != XCB_ATOM_NONE && above != XCB_ATOM_NONE) {
        if (!window.isVisible()) {
            auto atoms = statesOf(x11, id);
            std::erase(atoms, above);
            xcb_change_property(x11, XCB_PROP_MODE_REPLACE, id, state, XCB_ATOM_ATOM, 32,
                                static_cast<std::uint32_t>(atoms.size()), atoms.data());
        } else {
            const auto root = xcb_setup_roots_iterator(xcb_get_setup(x11)).data->root;

            xcb_client_message_event_t message{};
            message.response_type = XCB_CLIENT_MESSAGE;
            message.format = 32;
            message.window = id;
            message.type = state;
            message.data.data32[0] = 0U;
            message.data.data32[1] = above;
            message.data.data32[2] = 0U;
            message.data.data32[3] = 1U;
            message.data.data32[4] = 0U;

            xcb_send_event(x11, 0, root,
                           XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY |
                               XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT,
                           reinterpret_cast<const char *>(&message));
        }
    }

    xcb_flush(x11);
}

bool controlsTaskbarEntry()
{
    return connection() != nullptr;
}

void setTaskbarEntry(QWindow &window, bool shown)
{
    applyOnX11(window, shown);

    for (const int after : {kMapMs, 2 * kMapMs}) {
        QTimer::singleShot(after, &window, [asWindow = &window, shown]() {
            const auto skipping = skippingTaskbar(*asWindow);
            if (!skipping.has_value() || *skipping == !shown) {
                return;
            }

            core::Logger::instance().log(core::LogLevel::Debug, "ui",
                                         "the desktop put a window back in the task bar; "
                                         "saying otherwise again");
            applyOnX11(*asWindow, shown);
        });
    }
}

bool pointerHeld()
{
    auto *x11 = connection();
    if (x11 == nullptr) {
        return false;
    }

    const auto root = xcb_setup_roots_iterator(xcb_get_setup(x11)).data->root;
    const auto cookie = xcb_query_pointer(x11, root);

    std::unique_ptr<xcb_query_pointer_reply_t, decltype(&free)> reply{
        xcb_query_pointer_reply(x11, cookie, nullptr), &free};

    if (!reply) {
        return false;
    }

    constexpr std::uint16_t kButtons = XCB_BUTTON_MASK_1 | XCB_BUTTON_MASK_2 | XCB_BUTTON_MASK_3;
    return (reply->mask & kButtons) != 0;
}

void bringToFront(QWindow &window)
{

    auto *x11 = connection();
    if (x11 == nullptr) {
        return;
    }

    const auto active = atom(x11, "_NET_ACTIVE_WINDOW");
    if (active == XCB_ATOM_NONE) {
        return;
    }

    const auto id = static_cast<xcb_window_t>(window.winId());
    const auto root = xcb_setup_roots_iterator(xcb_get_setup(x11)).data->root;

    xcb_client_message_event_t message{};
    message.response_type = XCB_CLIENT_MESSAGE;
    message.format = 32;
    message.window = id;
    message.type = active;
    message.data.data32[0] = 2;
    message.data.data32[1] = XCB_CURRENT_TIME;
    message.data.data32[2] = 0;

    xcb_send_event(x11, 0, root,
                   XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT,
                   reinterpret_cast<const char *>(&message));
    xcb_flush(x11);
}

void allowAnotherProcessToRaise()
{

}

std::optional<QPoint> whereOnScreen(const QWindow &window)
{
    auto *x11 = connection();
    if (x11 == nullptr || !window.isVisible()) {
        return std::nullopt;
    }

    const auto id = static_cast<xcb_window_t>(window.winId());
    const auto root = xcb_setup_roots_iterator(xcb_get_setup(x11)).data->root;

    const auto cookie = xcb_translate_coordinates(x11, id, root, 0, 0);

    std::unique_ptr<xcb_translate_coordinates_reply_t, decltype(&free)> reply{
        xcb_translate_coordinates_reply(x11, cookie, nullptr), &free};

    if (!reply) {
        return std::nullopt;
    }

    return QPoint{reply->dst_x, reply->dst_y};
}

void askForFrameOffset(QWindow &window)
{
    auto *x11 = connection();
    if (x11 == nullptr) {
        return;
    }

    const auto request = atom(x11, "_NET_REQUEST_FRAME_EXTENTS");
    if (request == XCB_ATOM_NONE) {
        return;
    }

    const auto id = static_cast<xcb_window_t>(window.winId());
    const auto root = xcb_setup_roots_iterator(xcb_get_setup(x11)).data->root;

    xcb_client_message_event_t message{};
    message.response_type = XCB_CLIENT_MESSAGE;
    message.format = 32;
    message.window = id;
    message.type = request;

    xcb_send_event(x11, 0, root,
                   XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT,
                   reinterpret_cast<const char *>(&message));
    xcb_flush(x11);
}

std::optional<QPoint> frameOffset(const QWindow &window)
{
    auto *x11 = connection();
    if (x11 == nullptr) {
        return std::nullopt;
    }

    const auto extents = atom(x11, "_NET_FRAME_EXTENTS");
    if (extents == XCB_ATOM_NONE) {
        return std::nullopt;
    }

    const auto id = static_cast<xcb_window_t>(window.winId());

    const auto cookie = xcb_get_property(x11, 0, id, extents, XCB_ATOM_CARDINAL, 0, 4);

    std::unique_ptr<xcb_get_property_reply_t, decltype(&free)> reply{
        xcb_get_property_reply(x11, cookie, nullptr), &free};

    if (!reply || reply->type != XCB_ATOM_CARDINAL || reply->format != 32 ||
        xcb_get_property_value_length(reply.get()) < 4 * static_cast<int>(sizeof(std::uint32_t))) {
        return std::nullopt;
    }

    const auto *values = static_cast<const std::uint32_t *>(xcb_get_property_value(reply.get()));

    return QPoint{static_cast<int>(values[0]), static_cast<int>(values[2])};
}

}
