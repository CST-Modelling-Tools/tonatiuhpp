#include "RayTraceExecutor.h"

#include <exception>
#include <limits>
#include <memory>
#include <vector>

#include <QFuture>
#include <QMutex>
#include <QMutexLocker>
#include <QPromise>
#include <QRunnable>
#include <QThread>
#include <QThreadPool>

#include "core/TracePreparation.h"
#include "kernel/random/StandardRandom.h"
#include "kernel/run/InstanceNode.h"
#include "kernel/run/RayTracer.h"

namespace
{
constexpr ulong kRayChunkSize = 10'000;
}

struct RayTraceWorkerState
{
    std::shared_ptr<QPromise<void>> promise;
    std::atomic_bool* exportFailed = nullptr;
    std::function<RayEnergyBudget(qulonglong, ulong)> traceChunk;
    qulonglong chunkCount = 0;
    ulong chunkSize = 0;
    ulong totalRays = 0;
    std::atomic<qulonglong> nextChunk{0};
    std::atomic<qulonglong> completedChunks{0};
    std::atomic<qulonglong> completedRays{0};
    // Aggregated once per completed chunk, never in the per-ray hot loop.
    std::atomic<qulonglong> budgetLaunched{0};
    std::atomic<qulonglong> surfaceAbsorbed{0};
    std::atomic<qulonglong> airAbsorbed{0};
    std::atomic<qulonglong> escaped{0};
    std::atomic<int> remainingWorkers{0};
    std::atomic_bool failed{false};
    QMutex errorMutex;
    QMutex progressMutex;
    QString errorMessage;

    void recordFailure(const QString& message)
    {
        QMutexLocker lock(&errorMutex);
        if (errorMessage.isEmpty())
            errorMessage = message;
        failed.store(true);
    }

    void runWorker() noexcept
    {
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
        class WorkerServiceLevel
        {
        public:
            WorkerServiceLevel():
                m_thread(QThread::currentThread()),
                m_previous(m_thread->serviceLevel())
            {
                m_thread->setServiceLevel(QThread::QualityOfService::High);
            }

            ~WorkerServiceLevel()
            {
                m_thread->setServiceLevel(m_previous);
            }

        private:
            QThread* m_thread;
            QThread::QualityOfService m_previous;
        } workerServiceLevel;
#endif

        try {
            while (!failed.load() && !exportFailed->load() && !promise->isCanceled()) {
                const qulonglong chunkIndex = nextChunk.fetch_add(1);
                if (chunkIndex >= chunkCount)
                    break;

                const qulonglong chunkStart = chunkIndex * static_cast<qulonglong>(chunkSize);
                const ulong raysThisChunk = static_cast<ulong>(qMin<qulonglong>(
                    chunkSize,
                    static_cast<qulonglong>(totalRays) - chunkStart
                ));
                const RayEnergyBudget budget = traceChunk(chunkIndex, raysThisChunk);

                // addPhotons can stop partway through a chunk on export
                // failure. Never count such a chunk as fully traced.
                if (!exportFailed->load()) {
                    if (!budget.matches(raysThisChunk)) {
                        recordFailure(QString("Scientific integrity failure: chunk %1 requested %2 rays, "
                                              "but launched %3 and absorbed %4 at surfaces, %5 in air, %6 escaped.")
                                          .arg(chunkIndex).arg(raysThisChunk)
                                          .arg(budget.launched).arg(budget.surfaceAbsorbed)
                                          .arg(budget.airAbsorbed).arg(budget.escaped));
                        break;
                    }
                    budgetLaunched.fetch_add(budget.launched, std::memory_order_relaxed);
                    surfaceAbsorbed.fetch_add(budget.surfaceAbsorbed, std::memory_order_relaxed);
                    airAbsorbed.fetch_add(budget.airAbsorbed, std::memory_order_relaxed);
                    escaped.fetch_add(budget.escaped, std::memory_order_relaxed);
                    completedRays.fetch_add(raysThisChunk);
                    const qulonglong completed = completedChunks.fetch_add(1) + 1;
                    QMutexLocker progressLock(&progressMutex);
                    promise->setProgressValue(static_cast<int>(qMin<qulonglong>(
                        completed,
                        static_cast<qulonglong>(std::numeric_limits<int>::max())
                    )));
                }
            }
        } catch (const std::exception& error) {
            recordFailure(QString("Ray tracing worker failed: %1").arg(error.what()));
        } catch (...) {
            recordFailure("Ray tracing worker failed with an unknown exception.");
        }

        if (remainingWorkers.fetch_sub(1) == 1)
            promise->finish();
    }
};

RayTraceExecutor::~RayTraceExecutor() noexcept
{
    // Future handles alone do not join their workers. Protect the mutex,
    // callback captures and active scene context if a caller forgets wait.
    if (!m_active.load() || !m_workerState || !m_workerState->promise)
        return;
    QFuture<void> running = m_workerState->promise->future();
    running.cancel();
    running.waitForFinished();
}

void RayTraceExecutor::abortPartialStart(int submittedWorkers, int plannedWorkers) noexcept
{
    if (!m_workerState || !m_workerState->promise)
        return;
    // Never complete the promise while any submitted worker still uses
    // this executor. Account for workers that were never submitted.
    m_workerState->failed.store(true);
    QFuture<void> pending = m_workerState->promise->future();
    pending.cancel();
    const int unsubmitted = plannedWorkers - submittedWorkers;
    if (unsubmitted > 0
        && m_workerState->remainingWorkers.fetch_sub(unsubmitted) == unsubmitted)
        m_workerState->promise->finish();
    pending.waitForFinished();
}

QVector<ulong> RayTraceExecutor::guiRaysPerThread(ulong rays)
{
    QVector<ulong> chunks;
    if (rays == 0)
        return chunks;

    const qulonglong rayCount = static_cast<qulonglong>(rays);
    const qulonglong chunkCount = rayCount / kRayChunkSize + (rayCount % kRayChunkSize != 0 ? 1 : 0);
    chunks.reserve(static_cast<qsizetype>(chunkCount));

    ulong remaining = rays;
    while (remaining > 0) {
        const ulong raysThisChunk = qMin(kRayChunkSize, remaining);
        chunks << raysThisChunk;
        remaining -= raysThisChunk;
    }
    return chunks;
}

qulonglong RayTraceExecutor::taskCountForRays(ulong rays)
{
    if (rays == 0)
        return 0;
    const qulonglong rayCount = static_cast<qulonglong>(rays);
    return rayCount / kRayChunkSize + (rayCount % kRayChunkSize != 0 ? 1 : 0);
}

int RayTraceExecutor::workerCountForRays(ulong rays)
{
    const qulonglong chunkCount = taskCountForRays(rays);
    return qMax(1, qMin<int>(
        QThreadPool::globalInstance()->maxThreadCount(),
        static_cast<int>(qMin<qulonglong>(chunkCount, static_cast<qulonglong>(std::numeric_limits<int>::max())))
    ));
}

RayTraceExecution RayTraceExecutor::start(PreparedTraceContext&& context)
{
    RayTraceExecution execution;
    if (!context.m_layoutRoot || !context.m_sunInstance || !context.m_sunAperture
        || !context.m_sunShape || context.m_configuration.rays == 0) {
        execution.errorMessage = "RayTraceExecutor requires a valid prepared trace context.";
        return execution;
    }

    bool expected = false;
    if (!m_active.compare_exchange_strong(expected, true)) {
        execution.errorMessage = "RayTraceExecutor already has an active execution.";
        return execution;
    }

    m_exportFailed.store(false);
    m_workerState.reset();
    int plannedWorkers = 0;
    int submittedWorkers = 0;
    try {
        m_activeContext = std::make_shared<PreparedTraceContext>(std::move(context));
        execution.preparedContext = m_activeContext;
        PreparedTraceContext& prepared = *m_activeContext;
        const qulonglong chunkCount = taskCountForRays(prepared.m_configuration.rays);
        const int workerCount = workerCountForRays(prepared.m_configuration.rays);
        plannedWorkers = workerCount;

        auto promise = std::make_shared<QPromise<void>>();
        promise->start();
        promise->setProgressRange(0, static_cast<int>(qMin<qulonglong>(
            chunkCount,
            static_cast<qulonglong>(std::numeric_limits<int>::max())
        )));
        execution.future = promise->future();
        if (!execution.future.isValid()) {
            promise->finish();
            execution.errorMessage = "Could not start ray tracing because the worker-loop future is invalid.";
            m_activeContext.reset();
            m_active.store(false);
            return execution;
        }

        m_workerState = std::make_shared<RayTraceWorkerState>();
        m_workerState->promise = std::move(promise);
        m_workerState->exportFailed = &m_exportFailed;
        m_workerState->chunkCount = chunkCount;
        m_workerState->chunkSize = kRayChunkSize;
        m_workerState->totalRays = prepared.m_configuration.rays;
        m_workerState->remainingWorkers.store(workerCount);
        struct TraceChunkRunner
        {
            RayTraceExecutor* executor;
            std::shared_ptr<PreparedTraceContext> workerContext;

            RayEnergyBudget operator()(qulonglong chunkIndex, ulong raysThisChunk) const
            {
                StandardRandom random(StandardRandom::deriveChunkSeed(workerContext->m_configuration.masterSeed, chunkIndex));
                RayTracer tracer(workerContext->m_layoutRoot,
                                 workerContext->m_sunInstance,
                                 workerContext->m_sunAperture,
                                 workerContext->m_sunShape,
                                 workerContext->m_tracingAir,
                                 &random,
                                 workerContext->m_photonBuffer,
                                 workerContext->m_photonBuffer ? &executor->m_photonBufferMutex : nullptr,
                                 workerContext->m_exportSurfaceList,
                                 &executor->m_exportFailed,
                                 workerContext->m_hitCallback,
                                 workerContext->m_historyCallback,
                                 chunkIndex * static_cast<qulonglong>(kRayChunkSize));
                tracer(raysThisChunk);
                return tracer.energyBudget();
            }
        };

        struct RayTraceWorkerTask
        {
            std::shared_ptr<RayTraceWorkerState> workerState;

            void operator()() const
            {
                workerState->runWorker();
            }
        };

        const std::shared_ptr<PreparedTraceContext> workerContext = m_activeContext;
        m_workerState->traceChunk = TraceChunkRunner{this, workerContext};

        std::vector<std::unique_ptr<QRunnable>> workerTasks;
        workerTasks.reserve(static_cast<size_t>(workerCount));
        for (int worker = 0; worker < workerCount; ++worker) {
            const std::shared_ptr<RayTraceWorkerState> workerState = m_workerState;
            workerTasks.emplace_back(
                QRunnable::create(RayTraceWorkerTask{workerState}));
        }
        for (std::unique_ptr<QRunnable>& workerTask : workerTasks) {
            QThreadPool::globalInstance()->start(workerTask.get());
            workerTask.release();
            ++submittedWorkers;
        }

        execution.started = true;
        execution.owner = this;
    } catch (const std::exception& error) {
        abortPartialStart(submittedWorkers, plannedWorkers);
        execution.errorMessage = QString("Could not start ray tracing: %1").arg(error.what());
        m_workerState.reset();
        m_activeContext.reset();
        m_active.store(false);
    } catch (...) {
        abortPartialStart(submittedWorkers, plannedWorkers);
        execution.errorMessage = "Could not start ray tracing because an unknown exception was raised.";
        m_workerState.reset();
        m_activeContext.reset();
        m_active.store(false);
    }
    return execution;
}

bool RayTraceExecutor::waitForFinished(RayTraceExecution* execution, QString* errorMessage) noexcept
{
    if (!execution || !execution->started || execution->owner != this) {
        if (errorMessage)
            *errorMessage = "Ray-trace execution was not started.";
        return false;
    }

    struct ActiveRunReset
    {
        std::atomic_bool* active;
        std::shared_ptr<RayTraceWorkerState>* workerState;
        std::shared_ptr<PreparedTraceContext>* context;
        ~ActiveRunReset()
        {
            workerState->reset();
            context->reset();
            active->store(false);
        }
    } reset{&m_active, &m_workerState, &m_activeContext};

    try {
        execution->future.waitForFinished();
        if (m_workerState) {
            execution->fullyCompletedRays = m_workerState->completedRays.load();
            execution->fullyCompletedChunks = m_workerState->completedChunks.load();
            execution->energyBudget.launched = m_workerState->budgetLaunched.load(std::memory_order_relaxed);
            execution->energyBudget.surfaceAbsorbed = m_workerState->surfaceAbsorbed.load(std::memory_order_relaxed);
            execution->energyBudget.airAbsorbed = m_workerState->airAbsorbed.load(std::memory_order_relaxed);
            execution->energyBudget.escaped = m_workerState->escaped.load(std::memory_order_relaxed);
        }
        const bool workerFailed = m_workerState && m_workerState->failed.load();
        QString workerError;
        if (m_workerState) {
            QMutexLocker errorLock(&m_workerState->errorMutex);
            workerError = m_workerState->errorMessage;
        }
        execution->started = false;
        execution->owner = nullptr;
        if (workerFailed) {
            if (errorMessage)
                *errorMessage = workerError.isEmpty() ? "Ray tracing worker failed." : workerError;
            return false;
        }
        if (!execution->energyBudget.matches(execution->fullyCompletedRays)) {
            if (errorMessage)
                *errorMessage = "Scientific integrity failure: completed ray chunks have inconsistent terminal accounting.";
            return false;
        }
        return true;
    } catch (const std::exception& error) {
        if (errorMessage)
            *errorMessage = QString("Ray tracing worker failed: %1").arg(error.what());
    } catch (...) {
        if (errorMessage)
            *errorMessage = "Ray tracing worker failed with an unknown exception.";
    }
    execution->started = false;
    execution->owner = nullptr;
    return false;
}
