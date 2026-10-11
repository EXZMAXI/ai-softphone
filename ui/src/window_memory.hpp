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
#include <QRect>
#include <QSize>
#include <QString>
#include <QtQml/qqmlregistration.h>

namespace softphone::ui {

class WindowMemory : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Created by the application and exposed as `windows`.")

    Q_PROPERTY(int noticeRevision READ noticeRevision NOTIFY noticeRevisionChanged)

public:

    explicit WindowMemory(QString shape = {}, QObject *parent = nullptr);

    Q_INVOKABLE [[nodiscard]] QRect restore(const QString &key, int minimumWidth,
                                            int minimumHeight) const;

    Q_INVOKABLE void remember(const QString &key, int x, int y, int width, int height);

    Q_INVOKABLE [[nodiscard]] QSize restoreSize(const QString &key, int minimumWidth,
                                                int minimumHeight) const;
    Q_INVOKABLE void rememberSize(const QString &key, int width, int height);

    Q_INVOKABLE [[nodiscard]] bool noticeGiven(const QString &key) const;
    Q_INVOKABLE void rememberNotice(const QString &key);

    [[nodiscard]] int noticeRevision() const noexcept
    {
        return noticeRevision_;
    }

    Q_INVOKABLE [[nodiscard]] QString chose(const QString &key, const QString &fallback) const;
    Q_INVOKABLE void remembers(const QString &key, const QString &value);

    [[nodiscard]] QString filed(const QString &subtree, const QString &key) const;

Q_SIGNALS:
    void noticeRevisionChanged();

private:
    QString shape_;

    int noticeRevision_{0};
};

}
