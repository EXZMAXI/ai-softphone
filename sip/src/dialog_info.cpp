// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/sip/dialog_info.hpp>

#include <pjsua2.hpp>

#include <array>
#include <cstddef>
#include <string_view>
#include <utility>

namespace softphone::sip {
namespace {

[[nodiscard]] std::string text(const pj_str_t &value)
{
    return value.ptr == nullptr ? std::string{}
                                : std::string{value.ptr, static_cast<std::size_t>(value.slen)};
}

[[nodiscard]] core::LineState dialogStateOf(std::string_view state)
{
    if (state == "confirmed") {
        return core::LineState::Busy;
    }
    if (state == "trying" || state == "proceeding" || state == "early") {
        return core::LineState::Ringing;
    }
    return core::LineState::Idle;
}

}

std::optional<core::DialogInfoNotice> dialogInfoIn(const std::string &message)
{
    const auto blank = message.find("\r\n\r\n");
    const auto at = blank == std::string::npos ? message.find("\n\n") : blank;
    if (at == std::string::npos) {
        return std::nullopt;
    }

    auto body = message.substr(at + (blank == std::string::npos ? 2 : 4));
    if (body.find("dialog-info") == std::string::npos) {
        return std::nullopt;
    }

    std::array<char, 8192> arena{};
    auto *pool = pj_pool_create_on_buf("dlg-info", arena.data(), arena.size());
    if (pool == nullptr) {
        return std::nullopt;
    }

    auto *root = pj_xml_parse(pool, body.data(), body.size());
    if (root == nullptr || text(root->name) != "dialog-info") {
        return std::nullopt;
    }

    core::DialogInfoNotice notice;

    pj_str_t stateName = {const_cast<char *>("state"), 5};
    if (const auto *attribute = pj_xml_find_attr(root, &stateName, nullptr)) {
        notice.complete = text(attribute->value) == "full";
    }

    pj_str_t idName = {const_cast<char *>("id"), 2};
    for (auto *node = root->node_head.next; node != static_cast<void *>(&root->node_head);
         node = node->next) {
        if (text(node->name) != "dialog") {
            continue;
        }

        core::WatchedDialog dialog;
        if (const auto *id = pj_xml_find_attr(node, &idName, nullptr)) {
            dialog.id = text(id->value);
        }

        if (auto *state = pj_xml_find_node(node, &stateName)) {
            dialog.state = dialogStateOf(text(state->content));
        }

        notice.dialogs.push_back(std::move(dialog));
    }

    return notice;
}

}
