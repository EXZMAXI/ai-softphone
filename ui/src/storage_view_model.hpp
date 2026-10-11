// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#pragma once

#include <softphone/core/store.hpp>

#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>

#include <functional>

namespace softphone::ui {

class StorageViewModel : public QObject {
    Q_OBJECT

    QML_ELEMENT
    QML_UNCREATABLE("Created by the application and exposed as `storage`.")

    Q_PROPERTY(Problem problem READ problem NOTIFY problemChanged)

    Q_PROPERTY(bool working READ working NOTIFY problemChanged)

public:
    enum class Problem {

        None,

        Unreadable,

        Unusable,
    };
    Q_ENUM(Problem)

    using Recreator = std::function<void(std::function<void(core::StoreStatus)>)>;

    explicit StorageViewModel(QObject *parent = nullptr);

    void setRecreator(Recreator recreate);

    void applyStatus(core::StoreStatus status);

    [[nodiscard]] Problem problem() const noexcept
    {
        return problem_;
    }

    [[nodiscard]] bool working() const noexcept
    {
        return working_;
    }

public Q_SLOTS:

    void startAgain();

Q_SIGNALS:
    void problemChanged();

    void startedAgain();

private:
    void apply(Problem problem);

    Recreator recreate_;
    Problem problem_{Problem::None};
    bool working_{false};
};

}
