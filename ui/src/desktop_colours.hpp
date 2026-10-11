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

#include <QObject>
#include <Qt>

namespace softphone::ui {

class DesktopColours : public QObject {
    Q_OBJECT

public:
    explicit DesktopColours(QObject *parent = nullptr);
    ~DesktopColours() override;

    DesktopColours(const DesktopColours &) = delete;
    DesktopColours &operator=(const DesktopColours &) = delete;
    DesktopColours(DesktopColours &&) = delete;
    DesktopColours &operator=(DesktopColours &&) = delete;

    [[nodiscard]] Qt::ColorScheme scheme() const;

Q_SIGNALS:

    void changed();

private Q_SLOTS:

    void portalSaidSomething();

private:

    Qt::ColorScheme fromThePortal_{Qt::ColorScheme::Unknown};

    void askThePortal();
    void watchThePortal();
};

}
