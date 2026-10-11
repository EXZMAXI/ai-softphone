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
#include <QQuickWindow>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

namespace softphone::ui {

inline constexpr auto kScreenshotOption = "--screenshot";
inline constexpr auto kScreenshotIntoOption = "--screenshot-into";

inline constexpr auto kScreenshotCapturingOption = "--screenshot-capturing";

inline constexpr auto kScreenshotContentOption = "--screenshot-content";

[[nodiscard]] QStringList screenshotsWanted(const QStringList &arguments);

[[nodiscard]] QString screenshotsInto(const QStringList &arguments);

[[nodiscard]] QString screenshotsCapturing(const QStringList &arguments);

[[nodiscard]] QString screenshotsContent(const QStringList &arguments);

class Screenshots : public QObject {
    Q_OBJECT

    QML_ELEMENT
    QML_UNCREATABLE("Created by the application and exposed as `shots`.")

    Q_PROPERTY(QStringList wanted READ wanted CONSTANT)

    Q_PROPERTY(QString capturing READ capturing CONSTANT)

    Q_PROPERTY(QVariantMap content READ content CONSTANT)

public:
    Screenshots(QStringList wanted, QString into, QString capturing = {}, QString content = {},
                QObject *parent = nullptr);

    [[nodiscard]] QStringList wanted() const
    {
        return wanted_;
    }

    [[nodiscard]] QString capturing() const
    {
        return capturing_;
    }

    [[nodiscard]] QVariantMap content() const
    {
        return content_;
    }

    Q_INVOKABLE bool take(const QString &name, QQuickWindow *window);

private:
    QStringList wanted_;
    QString into_;
    QString capturing_;
    QVariantMap content_;
};

}
