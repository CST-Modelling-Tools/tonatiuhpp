#include "RayTraceRunner.h"

#include <atomic>
#include <exception>
#include <limits>
#include <thread>
#include <vector>

#include <QElapsedTimer>
#include <QMutex>
#include <QMutexLocker>
#include <QVector>

#include "core/SceneInstanceBuilder.h"
#include "kernel/air/AirTransmission.h"
#include "kernel/air/AirVacuum.h"
#include "kernel/node/TonatiuhFunctions.h"
#include "kernel/photons/PhotonsBuffer.h"
#include "kernel/random/RandomSTL.h"
#include "kernel/run/InstanceNode.h"
#include "kernel/run/RayTracer.h"
#include "kernel/scene/TSceneKit.h"
#include "kernel/sun/SunAperture.h"
#include "kernel/sun/SunKit.h"
#include "kernel/sun/SunPosition.h"
#include "kernel/sun/SunShape.h"
#include "libraries/math/3D/Transform.h"

namespace
{
bool fail(QString* errorMessage, const QString& message)
{
    if (errorMessage)
        *errorMessage = message;
    return false;
}

void reportProgress(const RayTraceRunner::ProgressCallback& progress, const QString& message)
{
    if (progress)
        progress(message);
}

QString formatRayProgress(ulong traced, ulong total)
{
    return QString("Traced %1/%2 rays.")
        .arg(QString::number(static_cast<qulonglong>(traced)))
        .arg(QString::number(static_cast<qulonglong>(total)));
}

ulong chunkSeed(ulong seed, qulonglong chunkIndex)
{
    quint64 value = static_cast<quint64>(seed);
    value += 0x9e3779b97f4a7c15ull + (chunkIndex << 6) + (chunkIndex >> 2);
    value ^= value >> 30;
    value *= 0xbf58476d1ce4e5b9ull;
    value ^= value >> 27;
    value *= 0x94d049bb133111ebull;
    value ^= value >> 31;
    return static_cast<ulong>(value);
}
}

bool RayTraceRunner::trace(TSceneKit* scene,
                           const RayTraceOptions& options,
                           RayTraceResult* result,
                           QString* errorMessage,
                           const ProgressCallback& progress,
                           const HitCallback& hitCallback,
                           const WorkerHitCallbackFactory& workerHitCallbackFactory,
                           const CancellationCallback& cancellation) const
{
    if (result)
        *result = RayTraceResult();

    if (!scene)
        return fail(errorMessage, "Scene is not loaded.");
    if (options.rays == 0)
        return fail(errorMessage, "Ray count must be greater than zero.");
    if (options.sunWidthDivisions <= 0 || options.sunHeightDivisions <= 0)
        return fail(errorMessage, "Sun grid dimensions must be greater than zero.");
    if (options.outputMode == RayTraceOutputMode::PhotonBuffer && !options.photonBuffer)
        return fail(errorMessage, "PhotonBuffer output mode requires a photon buffer.");
    if (options.outputMode != RayTraceOutputMode::NoOutput && options.outputMode != RayTraceOutputMode::PhotonBuffer)
        return fail(errorMessage, "Unsupported ray trace output mode.");

    auto isCanceled = [&cancellation]() {
        return cancellation && cancellation();
    };

    SunKit* sunKit = static_cast<SunKit*>(scene->getPart("world.sun", false));
    if (!sunKit)
        return fail(errorMessage, "Scene has no sun.");

    reportProgress(progress, "Preparing scene.");
    reportProgress(progress, "Building ray-tracing instance tree.");
    SceneInstanceTree instanceTree = SceneInstanceBuilder::build(scene);
    InstanceNode* instanceLayout = instanceTree.layoutRoot;
    if (!instanceLayout)
        return fail(errorMessage, "Scene has no layout.");

    instanceLayout->updateTree(Transform::Identity);

    reportProgress(progress, "Sizing sun aperture.");
    sunKit->setBox(instanceLayout->getBox());

    InstanceNode instanceSun(sunKit);
    instanceSun.setTransform(tgf::makeTransform(sunKit->m_transform));

    SunShape* sunShape = static_cast<SunShape*>(sunKit->getPart("shape", false));
    SunAperture* sunAperture = static_cast<SunAperture*>(sunKit->getPart("aperture", false));
    SunPosition* sunPosition = static_cast<SunPosition*>(sunKit->getPart("position", false));
    if (!sunShape || !sunAperture || !sunPosition)
        return fail(errorMessage, "Scene sun is missing position, shape, or aperture data.");

    reportProgress(progress, "Finding sun aperture cells.");
    if (!sunKit->findTexture(options.sunWidthDivisions, options.sunHeightDivisions, instanceLayout))
        return fail(errorMessage, "There are no surfaces defined for ray tracing.");

    if (result) {
        result->outputMode = options.outputMode;
        result->sunApertureArea = sunAperture->getArea();
        result->irradiance = sunPosition->irradiance.getValue();
        result->powerPerRay = options.rays > 0 ? result->sunApertureArea * result->irradiance / options.rays : 0.;
    }

    AirTransmission* air = static_cast<AirTransmission*>(scene->getPart("world.air.transmission", false));
    AirTransmission* tracingAir = nullptr;
    if (air && air->getTypeId() != AirVacuum::getClassTypeId())
        tracingAir = air;

    QVector<InstanceNode*> exportSurfaceList = options.exportSurfaceList;
    PhotonsBuffer* photonBuffer = options.outputMode == RayTraceOutputMode::PhotonBuffer ? options.photonBuffer : nullptr;
    QMutex mutexPhotonBuffer;
    std::atomic_bool exportFailed(false);
    RandomSTL random(options.seed, 1);
    QMutex mutexRandom;

    QElapsedTimer timer;
    timer.start();

    reportProgress(progress, "Starting ray loop.");
    const ulong progressStep = qMax<ulong>(1, options.rays / 10);

    ulong raysTraced = 0;
    bool canceled = false;
    const int requestedWorkers = qMax(1, options.workerCount);
    if (requestedWorkers == 1) {
        if (result) {
            result->workerCount = 1;
            result->chunkSize = progressStep;
            result->chunkCount = (static_cast<qulonglong>(options.rays) + progressStep - 1) / progressStep;
        }
        ulong traced = 0;
        while (traced < options.rays) {
            if (isCanceled()) {
                canceled = true;
                break;
            }

            const ulong raysThisStep = qMin(progressStep, options.rays - traced);
            RayTracer tracer(
                instanceLayout,
                &instanceSun,
                sunAperture,
                sunShape,
                tracingAir,
                &random,
                &mutexRandom,
                photonBuffer,
                photonBuffer ? &mutexPhotonBuffer : nullptr,
                exportSurfaceList,
                photonBuffer ? &exportFailed : nullptr,
                hitCallback
            );
            tracer(raysThisStep);
            if (exportFailed.load())
                break;

            traced += raysThisStep;
            raysTraced = traced;
            reportProgress(progress, formatRayProgress(traced, options.rays));
        }
    } else {
        const ulong chunkSize = qMax<ulong>(1, options.chunkSize);
        const qulonglong chunkCount = (static_cast<qulonglong>(options.rays) + chunkSize - 1) / chunkSize;
        const int workerCount = qMin<int>(requestedWorkers, static_cast<int>(qMin<qulonglong>(chunkCount, static_cast<qulonglong>(std::numeric_limits<int>::max()))));
        if (result) {
            result->workerCount = workerCount;
            result->chunkSize = chunkSize;
            result->chunkCount = chunkCount;
        }
        std::atomic<qulonglong> nextChunk(0);
        std::atomic<ulong> traced(0);
        std::atomic_bool failed(false);
        std::atomic_bool canceledByUser(false);
        std::vector<std::thread> workers;
        workers.reserve(static_cast<size_t>(workerCount));
        QMutex progressMutex;
        QMutex errorMutex;
        ulong nextProgress = progressStep;
        QString workerError;

        auto setWorkerError = [&](const QString& message) {
            QMutexLocker lock(&errorMutex);
            if (workerError.isEmpty())
                workerError = message;
            failed.store(true);
        };

        for (int workerIndex = 0; workerIndex < workerCount; ++workerIndex) {
            workers.emplace_back([&, workerIndex]() {
                try {
                    const HitCallback workerHitCallback = workerHitCallbackFactory ? workerHitCallbackFactory(workerIndex) : hitCallback;
                    QMutex workerRandomMutex;

                    while (!failed.load()) {
                        if (isCanceled()) {
                            canceledByUser.store(true);
                            failed.store(true);
                            break;
                        }

                        const qulonglong chunkIndex = nextChunk.fetch_add(1);
                        if (chunkIndex >= chunkCount)
                            break;

                        const qulonglong chunkStart = chunkIndex * static_cast<qulonglong>(chunkSize);
                        const ulong raysThisChunk = static_cast<ulong>(qMin<qulonglong>(chunkSize, static_cast<qulonglong>(options.rays) - chunkStart));
                        RandomSTL workerRandom(chunkSeed(options.seed, chunkIndex), 1);
                        RayTracer tracer(
                            instanceLayout,
                            &instanceSun,
                            sunAperture,
                            sunShape,
                            tracingAir,
                            &workerRandom,
                            &workerRandomMutex,
                            photonBuffer,
                            photonBuffer ? &mutexPhotonBuffer : nullptr,
                            exportSurfaceList,
                            photonBuffer ? &exportFailed : nullptr,
                            workerHitCallback
                        );
                        tracer(raysThisChunk);
                        if (exportFailed.load()) {
                            failed.store(true);
                            break;
                        }

                        const ulong tracedNow = traced.fetch_add(raysThisChunk) + raysThisChunk;
                        if (progress) {
                            QMutexLocker lock(&progressMutex);
                            while (nextProgress <= options.rays && tracedNow >= nextProgress) {
                                reportProgress(progress, formatRayProgress(nextProgress, options.rays));
                                if (options.rays - nextProgress < progressStep) {
                                    nextProgress = options.rays + 1;
                                } else {
                                    nextProgress += progressStep;
                                }
                            }
                            if (tracedNow == options.rays && nextProgress <= options.rays)
                                reportProgress(progress, formatRayProgress(options.rays, options.rays));
                        }
                    }
                } catch (const std::exception& e) {
                    setWorkerError(QString("Ray tracing worker failed: %1").arg(e.what()));
                } catch (...) {
                    setWorkerError("Ray tracing worker failed with an unknown exception.");
                }
            });
        }

        for (std::thread& worker : workers)
            worker.join();

        canceled = canceledByUser.load();
        raysTraced = traced.load();
        if (failed.load() && !canceled && !exportFailed.load())
            return fail(errorMessage, workerError.isEmpty() ? "Ray tracing worker failed." : workerError);
        if (!canceled && !exportFailed.load() && raysTraced != options.rays)
            return fail(errorMessage, "Ray tracing did not complete all requested rays.");
    }

    const double elapsedSeconds = static_cast<double>(timer.elapsed()) / 1000.;
    if (result) {
        result->elapsedSeconds = elapsedSeconds;
        result->raysTraced = raysTraced;
        result->raysPerSecond = elapsedSeconds > 0. ? static_cast<double>(raysTraced) / elapsedSeconds : 0.;
        result->canceled = canceled;
        result->exportFailed = exportFailed.load() || (photonBuffer && photonBuffer->hasExportFailed());
    }

    return true;
}
