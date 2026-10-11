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

#include <softphone/core/progress.hpp>

#include <QObject>
#include <QString>
#include <QStringList>

#include <memory>

namespace softphone::ui {

class DashboardViewModel : public QObject {
    Q_OBJECT

    Q_PROPERTY(int revision READ revision NOTIFY revisionChanged)

public:

    explicit DashboardViewModel(std::unique_ptr<core::ProgressStore> store,
                                QObject *parent = nullptr);
    ~DashboardViewModel() override;

    [[nodiscard]] int revision() const noexcept
    {
        return revision_;
    }

    [[nodiscard]] Q_INVOKABLE bool wasDone(const QString &code) const;

    Q_INVOKABLE void note(const QString &code);

    void noteAll(const QStringList &codes);

Q_SIGNALS:
    void revisionChanged();

private:
    std::unique_ptr<core::ProgressStore> store_;
    int revision_{0};
};

}
