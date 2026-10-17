// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "demo_prompter.hpp"

#include "prompter_view_model.hpp"

#include <chrono>

namespace softphone::ui {

DemoPrompter::DemoPrompter(PrompterViewModel &prompter, QObject *parent)
    : QObject{parent}, prompter_{prompter}
{}

void DemoPrompter::begins()
{
    prompter_.setRunning(true);
}

void DemoPrompter::said(bool near, const QString &text, int atMs)
{

    prompter_.heardTurn(near, text, std::chrono::milliseconds{atMs}, said_++);
}

void DemoPrompter::answered(const QString &text, int covers, int atMs)
{
    prompter_.heardAnswer(text, covers, std::chrono::milliseconds{atMs});
}

void DemoPrompter::forget()
{
    prompter_.setRunning(false);
    prompter_.clear();
    said_ = 0;
}

}
