// SPDX-License-Identifier: GPL-3.0-only
// Moss modification, 2026-10-03: own exactly one login attempt at a time.
#pragma once

#include <QTimer>
#include "tasks/Task.h"

class MossLoginAttempt final : public QObject {
   public:
    ~MossLoginAttempt() override { cancel(); }

    quint64 replace(const Task::Ptr& task)
    {
        cancel();
        m_task = task;
        const auto generation = m_generation;
        QTimer::singleShot(0, this, [this, generation] {
            if (isCurrent(generation))
                m_task->start();
        });
        return generation;
    }

    void cancel()
    {
        ++m_generation; // Invalidate queued signals before abort can emit anything.
        if (m_task) {
            QObject::disconnect(m_task.get(), nullptr, this, nullptr);
            if (m_task->isRunning())
                m_task->abort();
            m_task.reset();
        }
    }

    bool isCurrent(quint64 generation) const { return m_task && generation == m_generation; }
    Task* task() const { return m_task.get(); }

   private:
    Task::Ptr m_task;
    quint64 m_generation = 0;
};
