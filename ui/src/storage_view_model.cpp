// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "storage_view_model.hpp"

#include <softphone/core/logging.hpp>

#include <utility>

namespace softphone::ui {

StorageViewModel::StorageViewModel(QObject *parent) : QObject{parent} {}

void StorageViewModel::setRecreator(Recreator recreate)
{
    recreate_ = std::move(recreate);
}

void StorageViewModel::applyStatus(core::StoreStatus status)
{
    switch (status) {
    case core::StoreStatus::Ok:
    case core::StoreStatus::Missing:
        apply(Problem::None);
        return;

    case core::StoreStatus::UnsupportedSchema:
    case core::StoreStatus::Malformed:

        apply(Problem::Unreadable);
        return;

    case core::StoreStatus::Unavailable:
    case core::StoreStatus::Unwritable:
    case core::StoreStatus::Busy:
    case core::StoreStatus::Failed:
        apply(Problem::Unusable);
        return;
    }

    apply(Problem::Unusable);
}

void StorageViewModel::startAgain()
{
    if (working_ || !recreate_) {
        return;
    }

    working_ = true;
    Q_EMIT problemChanged();

    core::Logger::instance().log(core::LogLevel::Warning, "ui",
                                 "the user asked for the database to be made again");

    recreate_([this](core::StoreStatus status) {

        working_ = false;
        applyStatus(status);

        if (status == core::StoreStatus::Ok) {
            Q_EMIT startedAgain();
        }
    });
}

void StorageViewModel::apply(Problem problem)
{
    if (problem == problem_ && !working_) {
        return;
    }

    problem_ = problem;
    Q_EMIT problemChanged();
}

}
