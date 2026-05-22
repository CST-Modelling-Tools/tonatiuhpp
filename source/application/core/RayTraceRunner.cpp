#include "RayTraceRunner.h"

#include <QElapsedTimer>
#include <QMutex>
#include <QVector>

#include "core/SceneInstanceBuilder.h"
#include "kernel/air/AirTransmission.h"
#include "kernel/air/AirVacuum.h"
#include "kernel/node/TonatiuhFunctions.h"
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
}

bool RayTraceRunner::trace(TSceneKit* scene,
                           const RayTraceOptions& options,
                           RayTraceResult* result,
                           QString* errorMessage,
                           const ProgressCallback& progress,
                           const HitCallback& hitCallback) const
{
    if (result)
        *result = RayTraceResult();

    if (!scene)
        return fail(errorMessage, "Scene is not loaded.");
    if (options.rays == 0)
        return fail(errorMessage, "Ray count must be greater than zero.");
    if (options.sunWidthDivisions <= 0 || options.sunHeightDivisions <= 0)
        return fail(errorMessage, "Sun grid dimensions must be greater than zero.");

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
        result->sunApertureArea = sunAperture->getArea();
        result->irradiance = sunPosition->irradiance.getValue();
        result->powerPerRay = options.rays > 0 ? result->sunApertureArea * result->irradiance / options.rays : 0.;
    }

    AirTransmission* air = static_cast<AirTransmission*>(scene->getPart("world.air.transmission", false));
    AirTransmission* tracingAir = nullptr;
    if (air && air->getTypeId() != AirVacuum::getClassTypeId())
        tracingAir = air;

    QVector<InstanceNode*> exportSurfaceList;
    RandomSTL random(options.seed, 1);
    QMutex mutexRandom;

    QElapsedTimer timer;
    timer.start();

    reportProgress(progress, "Starting ray loop.");
    ulong traced = 0;
    const ulong progressStep = qMax<ulong>(1, options.rays / 10);
    while (traced < options.rays) {
        const ulong raysThisStep = qMin(progressStep, options.rays - traced);
        RayTracer tracer(
            instanceLayout,
            &instanceSun,
            sunAperture,
            sunShape,
            tracingAir,
            &random,
            &mutexRandom,
            nullptr,
            nullptr,
            exportSurfaceList,
            nullptr,
            hitCallback
        );
        tracer(raysThisStep);
        traced += raysThisStep;
        reportProgress(progress, formatRayProgress(traced, options.rays));
    }

    const double elapsedSeconds = static_cast<double>(timer.elapsed()) / 1000.;
    if (result) {
        result->elapsedSeconds = elapsedSeconds;
        result->raysPerSecond = elapsedSeconds > 0. ? static_cast<double>(options.rays) / elapsedSeconds : 0.;
    }

    return true;
}
