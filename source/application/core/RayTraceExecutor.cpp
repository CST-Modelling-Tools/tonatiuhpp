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
    std::function<void(qulonglong, ulong)> traceChunk;
    qulonglong chunkCount = 0;
    ulong chunkSize = 0;
    ulong totalRays = 0;
    std::atomic<qulonglong> nextChunk{0};
    std::atomic<qulonglong> completedChunks{0};
    std::atomic<qulonglong> completedRays{0};
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
                traceChunk(chunkIndex, raysThisChunk);

                // addPhotons can stop partway through a chunk on export
                // failure. Never count such a chunk as fully traced.
                if (!exportFailed->load()) {
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
    try {
        m_activeContext = std::make_shared<PreparedTraceContext>(std::move(context));
        execution.preparedContext = m_activeContext;
        PreparedTraceContext& prepared = *m_activeContext;
        const qulonglong chunkCount = taskCountForRays(prepared.m_configuration.rays);
        const int workerCount = workerCountForRays(prepared.m_configuration.rays);

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
        const std::shared_ptr<PreparedTraceContext> workerContext = m_activeContext;
        m_workerState->traceChunk = [this, workerContext](qulonglong chunkIndex, ulong raysThisChunk) {
            StandardRandom random(StandardRandom::deriveChunkSeed(workerContext->m_configuration.masterSeed, chunkIndex));
            RayTracer tracer(workerContext->m_layoutRoot,
                             workerContext->m_sunInstance,
                             workerContext->m_sunAperture,
                             workerContext->m_sunShape,
                             workerContext->m_tracingAir,
                             &random,
                             workerContext->m_photonBuffer,
                             workerContext->m_photonBuffer ? &m_photonBufferMutex : nullptr,
                             workerContext->m_exportSurfaceList,
                             &m_exportFailed,
                             workerContext->m_hitCallback,
                             workerContext->m_historyCallback,
                             chunkIndex * static_cast<qulonglong>(kRayChunkSize));
            tracer(raysThisChunk);
        };

        std::vector<std::unique_ptr<QRunnable>> workerTasks;
        workerTasks.reserve(static_cast<size_t>(workerCount));
        for (int worker = 0; worker < workerCount; ++worker) {
            const std::shared_ptr<RayTraceWorkerState> workerState = m_workerState;
            workerTasks.emplace_back(QRunnable::create([workerState]() {
                workerState->runWorker();
            }));
        }
        for (std::unique_ptr<QRunnable>& workerTask : workerTasks)
            QThreadPool::globalInstance()->start(workerTask.release());

        execution.started = true;
        execution.owner = this;
    } catch (const std::exception& error) {
        if (m_workerState && m_workerState->promise)
            m_workerState->promise->finish();
        execution.errorMessage = QString("Could not start ray tracing: %1").arg(error.what());
        m_workerState.reset();
        m_activeContext.reset();
        m_active.store(false);
    } catch (...) {
        if (m_workerState && m_workerState->promise)
            m_workerState->promise->finish();
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
