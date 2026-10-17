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
#include <QString>
#include <QtQml/qqmlregistration.h>

namespace softphone::ui {

class PrompterViewModel;

class DemoPrompter final : public QObject {
    Q_OBJECT

    QML_ELEMENT
    QML_UNCREATABLE("Created by the application and exposed as `demoPrompter`.")

public:
    explicit DemoPrompter(PrompterViewModel &prompter, QObject *parent = nullptr);

    Q_INVOKABLE void begins();

    Q_INVOKABLE void said(bool near, const QString &text, int atMs);

    Q_INVOKABLE void answered(const QString &text, int covers, int atMs);

    Q_INVOKABLE void forget();

private:
    PrompterViewModel &prompter_;

    qulonglong said_{0};
};

}
