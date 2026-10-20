#include "SimulationSession.h"

#include <utility>

#include "kernel/photons/PhotonsBuffer.h"

SimulationSession::~SimulationSession()
{
    // A future alone does not join its workers. Never release the executor,
    // callbacks or retained resources while a ray-trace worker can use them.
    if (m_execution && m_execution->started) {
        m_execution->future.cancel();
        m_executor.waitForFinished(m_execution.get());
    }
}

bool SimulationSession::fail(const QString& message, QString* error)
{
    m_error = message;
    m_state = State::Failed;
    if (error)
        *error = message;
    return false;
}

bool SimulationSession::beginPreparation(QString* error)
{
    if (m_state == State::Running) {
        if (error)
            *error = "Cannot prepare a simulation while another run is active.";
        return false;
    }
    m_prepared.reset();
    m_execution.reset();
    m_result = RayTraceExecutorResult();
    m_completedRays = 0;
    m_completedChunks = 0;
    m_exportFailed = false;
    m_error.clear();
    m_state = State::Idle;
    m_timer.start();
    return true;
}

bool SimulationSession::prepareGui(const GuiTracePreparationInput& input, QString* error)
{
    if (!beginPreparation(error))
        return false;
    PreparedTraceContext prepared;
    QString message;
    if (!TracePreparation::prepareGuiTrace(input, &prepared, &message))
        return fail(message, error);
    TracePreparation::initializeResult(prepared, &m_result);
    m_prepared.emplace(std::move(prepared));
    m_state = State::Prepared;
    if (error)
        error->clear();
    return true;
}

bool SimulationSession::prepareHeadless(const HeadlessTracePreparationInput& input, QString* error)
{
    if (!beginPreparation(error))
        return false;
    PreparedTraceContext prepared;
    QString message;
    if (!TracePreparation::prepareHeadlessTrace(input, &prepared, &message))
        return fail(message, error);
    TracePreparation::initializeResult(prepared, &m_result);
    m_prepared.emplace(std::move(prepared));
    m_state = State::Prepared;
    if (error)
        error->clear();
    return true;
}

bool SimulationSession::start(QString* error)
{
    if (m_state != State::Prepared || !m_prepared) {
        if (error)
            *error = "Simulation must be prepared before starting, and cannot be started twice.";
        return false;
    }
    m_execution = std::make_unique<RayTraceExecution>(m_executor.start(std::move(*m_prepared)));
    m_prepared.reset();
    if (!m_execution->started)
        return fail(m_execution->errorMessage, error);
    m_state = State::Running;
    if (error)
        error->clear();
    return true;
}

void SimulationSession::cancel()
{
    if (m_state == State::Running && m_execution)
        m_execution->future.cancel();
}

bool SimulationSession::waitForFinished(QString* error)
{
    if (m_state != State::Running || !m_execution) {
        if (error)
            *error = "There is no running simulation to finish.";
        return false;
    }

    QString workerError;
    const bool workersSucceeded = m_executor.waitForFinished(m_execution.get(), &workerError);
    m_completedRays = m_execution->fullyCompletedRays;
    m_completedChunks = m_execution->fullyCompletedChunks;
    const bool cancelled = m_execution->future.isCanceled();
    const PreparedTraceContext* context = m_execution->context();
    m_exportFailed = m_executor.exportFailed()
        || (context && context->photonBuffer() && context->photonBuffer()->hasExportFailed());

    m_result.elapsedSeconds = m_timer.elapsed() / 1000.;
    // On incomplete runs these are conservative, *fully finished* chunks
    // only. Partially processed chunks are never presented as completed rays.
    m_result.raysTraced = static_cast<ulong>(m_completedRays);
    m_result.raysPerSecond = m_result.elapsedSeconds > 0.
        ? static_cast<double>(m_completedRays) / m_result.elapsedSeconds : 0.;
    m_result.exportFailed = m_exportFailed;

    if (!workersSucceeded)
        return fail(workerError.isEmpty() ? "Ray tracing worker failed." : workerError, error);
    if (m_exportFailed)
        return fail("Photon buffer/export failed during ray tracing.", error);
    if (cancelled) {
        m_error = "Ray tracing was cancelled; partial results are not accepted.";
        m_state = State::Cancelled;
        if (error)
            *error = m_error;
        return false;
    }
    if (!context || m_completedRays != context->rays()
        || m_completedChunks != m_result.chunkCount)
        return fail("Ray tracing finished without completing every requested chunk.", error);

    QString finalizeError;
    if (!TracePreparation::finalizeResult(*context, false, m_result.elapsedSeconds,
                                          &m_result, &finalizeError))
        return fail(finalizeError, error);
    m_state = State::Completed;
    m_error.clear();
    if (error)
        error->clear();
    return true;
}

QFuture<void> SimulationSession::future() const
{
    return m_execution ? m_execution->future : QFuture<void>();
}

const PreparedTraceContext* SimulationSession::preparedContext() const
{
    if (m_prepared)
        return &*m_prepared;
    return m_execution ? m_execution->context() : nullptr;
}

void SimulationSession::retainResource(std::shared_ptr<void> resource)
{
    if (resource)
        m_resourcePins.push_back(std::move(resource));
}
