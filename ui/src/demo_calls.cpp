// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "demo_calls.hpp"

#include <softphone/core/call_backend.hpp>

#include <utility>

namespace softphone::ui {

DemoCalls::DemoCalls(core::CallController &calls, std::string accountId,
                     std::function<void(std::function<void()>)> post, QObject *parent)
    : QObject{parent}, calls_{calls}, accountId_{std::move(accountId)}, post_{std::move(post)}
{}

void DemoCalls::arrives(const QString &callId, const QString &number)
{
    core::IncomingCall call;
    call.callId = callId.toStdString();
    call.accountId = accountId_;
    call.remoteUri = "sip:" + number.toStdString() + "@example.test";

    post_([this, call]() { calls_.onIncomingCall(call); });
}

void DemoCalls::connected(const QString &callId)
{
    core::CallProgress progress;
    progress.callId = callId.toStdString();
    progress.state = core::BackendCallState::Connected;

    post_([this, progress]() { calls_.onCallProgress(progress); });
}

void DemoCalls::held(const QString &callId)
{
    post_([this, id = callId.toStdString()]() { calls_.hold(id); });
}

void DemoCalls::heldByThem(const QString &callId)
{

    post_([this, id = callId.toStdString()]() { calls_.onRemoteHoldChanged(id, true); });
}

void DemoCalls::activated(const QString &callId)
{
    post_([this, id = callId.toStdString()]() { calls_.activate(id); });
}

void DemoCalls::quiet()
{
    post_([this]() {
        for (const auto &call : calls_.calls()) {
            core::CallProgress progress;
            progress.callId = call.id;
            progress.state = core::BackendCallState::Disconnected;
            progress.endedLocally = true;
            calls_.onCallProgress(progress);
        }
    });
}

}
