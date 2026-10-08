#include "TracePreparation.h"

#include <type_traits>

#include "core/RayTraceExecutor.h"
#include "kernel/air/AirTransmission.h"
#include "kernel/air/AirVacuum.h"
#include "kernel/photons/PhotonsBuffer.h"
#include "kernel/run/InstanceNode.h"
#include "kernel/scene/TSceneKit.h"
#include "kernel/sun/SunAperture.h"
#include "kernel/sun/SunKit.h"
#include "kernel/sun/SunPosition.h"
#include "kernel/sun/SunShape.h"
#include "libraries/math/3D/Transform.h"

namespace
{
static_assert(!std::is_copy_constructible_v<PreparedTraceContext>);
static_assert(!std::is_copy_assignable_v<PreparedTraceContext>);
static_assert(std::is_move_constructible_v<PreparedTraceContext>);

bool fail(QString* errorMessage, const QString& message)
{
    if (errorMessage)
        *errorMessage = message;
    return false;
}

void reportProgress(const TracePreparationProgress& progress, const QString& message)
{
    if (progress)
        progress(message);
}

bool resolveSun(TSceneKit* scene,
                SunKit** sunKit,
                SunShape** sunShape,
                SunAperture** sunAperture,
                SunPosition** sunPosition,
                QString* errorMessage)
{
    if (!scene)
        return fail(errorMessage, "Scene is not loaded.");

    *sunKit = static_cast<SunKit*>(scene->getPart("world.sun", false));
    if (!*sunKit)
        return fail(errorMessage, "Scene has no sun at world.sun.");

    *sunShape = static_cast<SunShape*>((*sunKit)->getPart("shape", false));
    if (!*sunShape)
        return fail(errorMessage, "Scene sun is missing shape data.");

    *sunAperture = static_cast<SunAperture*>((*sunKit)->getPart("aperture", false));
    if (!*sunAperture)
        return fail(errorMessage, "Scene sun is missing aperture data.");

    *sunPosition = static_cast<SunPosition*>((*sunKit)->getPart("position", false));
    if (!*sunPosition)
        return fail(errorMessage, "Scene sun is missing position data.");
    return true;
}

bool validateCommon(ulong rays, int sunWidthDivisions, int sunHeightDivisions, QString* errorMessage)
{
    if (rays == 0)
        return fail(errorMessage, "Ray count must be greater than zero.");
    if (sunWidthDivisions <= 0 || sunHeightDivisions <= 0)
        return fail(errorMessage, "Sun grid dimensions must be greater than zero.");
    return true;
}
}

PreparedTraceContext::PreparedTraceContext() = default;
PreparedTraceContext::~PreparedTraceContext() = default;
PreparedTraceContext::PreparedTraceContext(PreparedTraceContext&&) noexcept = default;
PreparedTraceContext& PreparedTraceContext::operator=(PreparedTraceContext&&) noexcept = default;

double PreparedTraceContext::sunApertureArea() const
{
    return m_sunAperture ? m_sunAperture->getArea() : 0.;
}

double PreparedTraceContext::irradiance() const
{
    return m_sunPosition ? m_sunPosition->irradiance.getValue() : 0.;
}

double PreparedTraceContext::powerPerRay() const
{
    return m_rays > 0 ? sunApertureArea() * irradiance() / m_rays : 0.;
}

bool TracePreparation::prepareGuiTrace(const GuiTracePreparationInput& input,
                                       PreparedTraceContext* context,
                                       QString* errorMessage)
{
    if (!context)
        return fail(errorMessage, "Prepared trace context is null.");
    if (!validateCommon(input.rays, input.sunWidthDivisions, input.sunHeightDivisions, errorMessage))
        return false;
    if (!input.layoutRoot || !input.sunInstance)
        return fail(errorMessage, "GUI trace preparation is missing its layout or sun instance.");

    SunKit* sunKit = nullptr;
    SunShape* sunShape = nullptr;
    SunAperture* sunAperture = nullptr;
    SunPosition* sunPosition = nullptr;
    if (!resolveSun(input.scene, &sunKit, &sunShape, &sunAperture, &sunPosition, errorMessage))
        return false;

    if (input.synchronizeScene)
        input.synchronizeScene();
    input.layoutRoot->updateTree(Transform::Identity);
    const Box3D& layoutBox = input.layoutRoot->getBox();
    if (!layoutBox.isValid())
        return fail(errorMessage, "GUI scene has no valid analytical ray-tracing bounds.");
    // Both the borrowed GUI and owned headless preparation use analytical bounds.
    sunKit->setBox(layoutBox);
    if (!sunKit->findTexture(input.sunWidthDivisions, input.sunHeightDivisions, input.layoutRoot))
        return fail(errorMessage, "There are no surfaces defined for ray tracing.");

    input.sunInstance->setNode(sunKit);
    input.sunInstance->setTransform(tgf::makeTransform(sunKit->m_transform));

    PreparedTraceContext prepared;
    prepared.m_scene = input.scene;
    prepared.m_layoutRoot = input.layoutRoot;
    prepared.m_sunInstance = input.sunInstance;
    prepared.m_sunPosition = sunPosition;
    prepared.m_sunAperture = sunAperture;
    prepared.m_sunShape = sunShape;
    prepared.m_tracingAir = input.tracingAir;
    prepared.m_masterSeed = input.masterSeed;
    prepared.m_photonBuffer = input.photonBuffer;
    prepared.m_exportSurfaceList = input.exportSurfaceList;
    prepared.m_hitCallback = input.hitCallback;
    prepared.m_rays = input.rays;
    *context = std::move(prepared);
    return true;
}

bool TracePreparation::prepareHeadlessTrace(const HeadlessTracePreparationInput& input,
                                            PreparedTraceContext* context,
                                            QString* errorMessage)
{
    if (!context)
        return fail(errorMessage, "Prepared trace context is null.");
    if (!validateCommon(input.rays, input.sunWidthDivisions, input.sunHeightDivisions, errorMessage))
        return false;

    SunKit* sunKit = nullptr;
    SunShape* sunShape = nullptr;
    SunAperture* sunAperture = nullptr;
    SunPosition* sunPosition = nullptr;
    if (!resolveSun(input.scene, &sunKit, &sunShape, &sunAperture, &sunPosition, errorMessage))
        return false;

    reportProgress(input.progress, "Preparing scene.");
    reportProgress(input.progress, "Building ray-tracing instance tree.");
    SceneInstanceTree instanceTree = SceneInstanceBuilder::build(input.scene);
    if (!instanceTree.sceneRoot)
        return fail(errorMessage, "Could not create ray-tracing scene root.");
    if (!instanceTree.layoutRoot)
        return fail(errorMessage, "Scene has no layout at group.");

    reportProgress(input.progress, "Updating ray-tracing instance tree.");
    instanceTree.layoutRoot->updateTree(Transform::Identity);
    const Box3D& layoutBox = instanceTree.layoutRoot->getBox();
    if (!layoutBox.isValid())
        return fail(errorMessage, "Scene layout has no valid ray-tracing bounds. Check that it contains ray-traceable surfaces with shape and profile data.");

    reportProgress(input.progress, "Sizing sun aperture.");
    sunKit->setBox(layoutBox);
    auto sunInstance = std::make_unique<InstanceNode>(sunKit);
    sunInstance->setTransform(tgf::makeTransform(sunKit->m_transform));

    reportProgress(input.progress, "Finding sun aperture cells.");
    if (!sunKit->findTexture(input.sunWidthDivisions, input.sunHeightDivisions, instanceTree.layoutRoot))
        return fail(errorMessage, "There are no surfaces defined for ray tracing.");

    AirTransmission* air = static_cast<AirTransmission*>(input.scene->getPart("world.air.transmission", false));
    AirTransmission* tracingAir = nullptr;
    if (air && air->getTypeId() != AirVacuum::getClassTypeId())
        tracingAir = air;

    PreparedTraceContext prepared;
    prepared.m_scene = input.scene;
    prepared.m_ownedInstanceTree = std::move(instanceTree);
    prepared.m_ownedSunInstance = std::move(sunInstance);
    prepared.m_layoutRoot = prepared.m_ownedInstanceTree.layoutRoot;
    prepared.m_sunInstance = prepared.m_ownedSunInstance.get();
    prepared.m_sunPosition = sunPosition;
    prepared.m_sunAperture = sunAperture;
    prepared.m_sunShape = sunShape;
    prepared.m_tracingAir = tracingAir;
    prepared.m_masterSeed = input.seed;
    prepared.m_hitCallback = input.hitCallback;
    prepared.m_rays = input.rays;
    *context = std::move(prepared);
    return true;
}

void TracePreparation::initializeResult(const PreparedTraceContext& context, RayTraceExecutorResult* result)
{
    if (!result)
        return;
    *result = RayTraceExecutorResult();
    const QVector<ulong> workItems = RayTraceExecutor::guiRaysPerThread(context.m_rays);
    result->sunApertureArea = context.sunApertureArea();
    result->irradiance = context.irradiance();
    result->powerPerRay = context.powerPerRay();
    result->workerCount = RayTraceExecutor::workerCountForRays(context.m_rays);
    result->chunkSize = workItems.isEmpty() ? 0 : workItems.first();
    result->chunkCount = static_cast<qulonglong>(workItems.size());
}

bool TracePreparation::finalizeResult(const PreparedTraceContext& context,
                                      bool exportFailed,
                                      double elapsedSeconds,
                                      RayTraceExecutorResult* result,
                                      QString* errorMessage)
{
    if (result) {
        result->elapsedSeconds = elapsedSeconds;
        result->raysTraced = exportFailed ? 0 : context.m_rays;
        result->raysPerSecond = elapsedSeconds > 0.
            ? static_cast<double>(result->raysTraced) / elapsedSeconds
            : 0.;
        result->exportFailed = exportFailed || (context.m_photonBuffer && context.m_photonBuffer->hasExportFailed());
    }
    if (exportFailed)
        return fail(errorMessage, "Photon buffer/export failed during ray tracing.");
    return true;
}
