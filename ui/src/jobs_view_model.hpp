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

#include <softphone/core/jobs.hpp>

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

#include <cstdint>
#include <functional>
#include <vector>

namespace softphone::ui {

class JobsViewModel : public QObject {
    Q_OBJECT

    QML_ELEMENT
    QML_UNCREATABLE("Created by the application and exposed as `jobs`.")

    Q_PROPERTY(QVariantList rows READ rows NOTIFY rowsChanged)

    Q_PROPERTY(int count READ count NOTIFY rowsChanged)

    Q_PROPERTY(int working READ working NOTIFY rowsChanged)

    Q_PROPERTY(int failed READ failed NOTIFY rowsChanged)

    Q_PROPERTY(QString summary READ summary NOTIFY rowsChanged)

    Q_PROPERTY(qint64 workingSinceMs READ workingSinceMs NOTIFY rowsChanged)

public:

    struct Entry {
        QString id;
        QString recordingId;

        QString party;
        qint64 startedAtMs{0};

        qint64 sinceMs{0};

        qint64 queuedAtMs{0};

        core::JobState state{core::JobState::Queued};

        core::JobKind kind{core::JobKind::Transcribe};

        unsigned attempts{0};
        QString error;
    };

    using Action = std::function<void(const QString &jobId)>;
    using Plain = std::function<void()>;

    explicit JobsViewModel(QObject *parent = nullptr);

    [[nodiscard]] QVariantList rows() const;

    [[nodiscard]] Q_INVOKABLE QVariantList rowsNow() const;

    [[nodiscard]] int count() const noexcept
    {
        return static_cast<int>(rows_.size());
    }

    [[nodiscard]] int working() const noexcept
    {
        return working_;
    }

    [[nodiscard]] int failed() const noexcept
    {
        return failed_;
    }

    [[nodiscard]] qint64 workingSinceMs() const noexcept
    {
        return workingSince_;
    }

    [[nodiscard]] bool transcribing(const QString &recordingId) const;

    [[nodiscard]] QString summary() const;

    Q_INVOKABLE void cancel(const QString &jobId);

    Q_INVOKABLE void dismiss(const QString &jobId);

    Q_INVOKABLE void dismissAll();

    void setJobs(std::vector<Entry> rows);
    void setActions(Action cancel, Action dismiss, Plain refresh);

Q_SIGNALS:
    void rowsChanged();

private:
    std::vector<Entry> rows_;
    int working_{0};
    int transcribing_{0};
    int generating_{0};
    int failed_{0};
    qint64 workingSince_{0};

    Action cancel_;
    Action dismiss_;
    Plain refresh_;
};

}
