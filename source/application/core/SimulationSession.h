#pragma once

#include <memory>
#include <optional>
#include <vector>

#include <QElapsedTimer>
#include <QFuture>
#include <QString>

#include "core/RayTraceExecutor.h"
#include "core/TracePreparation.h"

// The GUI and headless adapters use one session lifecycle. The optical
// implementation and the GUI/headless preparation policies remain unchanged.
class SimulationSession final
{
public:
    enum class State { Idle, Prepared, Running, Completed, Cancelled, Failed };

    SimulationSession() = default;
    ~SimulationSession();
    SimulationSession(const SimulationSession&) = delete;
    SimulationSession& operator=(const SimulationSession&) = delete;

    bool prepareGui(const GuiTracePreparationInput& input, QString* error = nullptr);
    bool prepareHeadless(const HeadlessTracePreparationInput& input, QString* error = nullptr);
    bool start(QString* error = nullptr);
    void cancel();
    bool waitForFinished(QString* error = nullptr);

    State state() const { return m_state; }
    bool isRunning() const { return m_state == State::Running; }
    const QString& errorMessage() const { return m_error; }
    const RayTraceExecutorResult& result() const { return m_result; }
    qulonglong fullyCompletedRays() const { return m_completedRays; }
    qulonglong fullyCompletedChunks() const { return m_completedChunks; }
    bool exportFailed() const { return m_exportFailed; }
    QFuture<void> future() const;
    const PreparedTraceContext* preparedContext() const;

    // Pin optional owned scene/plugin/export resources until this session is
    // destroyed. Borrowed GUI scene/tree/buffer inputs still require their
    // adapter to remain alive throughout the synchronous wait and export.
    void retainResource(std::shared_ptr<void> resource);

private:
    bool beginPreparation(QString* error);
    bool fail(const QString& message, QString* error);

    // Resource pins are destroyed last, after both execution and executor.
    std::vector<std::shared_ptr<void>> m_resourcePins;
    RayTraceExecutor m_executor;
    std::unique_ptr<RayTraceExecution> m_execution;
    std::optional<PreparedTraceContext> m_prepared;
    QElapsedTimer m_timer;
    RayTraceExecutorResult m_result;
    qulonglong m_completedRays = 0;
    qulonglong m_completedChunks = 0;
    bool m_exportFailed = false;
    State m_state = State::Idle;
    QString m_error;
};
