// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "dashboard_view_model.hpp"

#include <softphone/core/logging.hpp>

#include <array>
#include <string>
#include <utility>

namespace softphone::ui {

DashboardViewModel::DashboardViewModel(std::unique_ptr<core::ProgressStore> store, QObject *parent)
    : QObject{parent}, store_{std::move(store)}
{}

DashboardViewModel::~DashboardViewModel() = default;

bool DashboardViewModel::wasDone(const QString &code) const
{
    return store_ != nullptr && store_->done(code.toStdString());
}

void DashboardViewModel::note(const QString &code)
{
    noteAll({code});
}

void DashboardViewModel::noteAll(const QStringList &codes)
{
    if (store_ == nullptr) {
        return;
    }

    QStringList noted;
    for (const auto &code : codes) {
        if (code.isEmpty() || store_->done(code.toStdString())) {
            continue;
        }
        store_->note(code.toStdString());
        noted << code;
    }

    if (noted.isEmpty()) {
        return;
    }

    ++revision_;

    const auto which = noted.join(QStringLiteral(", ")).toStdString();
    const auto revision = std::to_string(revision_);
    const std::array attributes{core::LogAttribute{"deeds", which},
                                core::LogAttribute{"revision", revision}};
    core::Logger::instance().log(core::LogLevel::Info, "ui",
                                 "the summary noted something done for the first time", attributes);

    Q_EMIT revisionChanged();
}

}
