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
    return m_configuration.rays > 0
        ? sunApertureArea() * irradiance() / m_configuration.rays : 0.;
}

// Adapt an already available GUI instance tree or a freshly built headless
// tree to the same numerical preparation pipeline. Ownership remains in
// PreparedTraceContext; this method never creates an extra instance tree.
bool TracePreparation::prepareResolvedTrace(PreparedTraceContext&& prepared,
                                            SunKit* sunKit,
                                            const SimulationConfig& configuration,
                                            const TracePreparationProgress& progress,
                                            const QString& invalidBoundsMessage,
                                            bool sunTransformBeforeSampling,
                                            PreparedTraceContext* context,
                                            QString* errorMessage)
{
    // The adapters validate the configuration and resolve the sun before
    // scene synchronization, preserving their original initialization order.
    reportProgress(progress, "Updating ray-tracing instance tree.");
    prepared.m_layoutRoot->updateTree(Transform::Identity);
    const Box3D& layoutBox = prepared.m_layoutRoot->getBox();
    if (!layoutBox.isValid())
        return fail(errorMessage, invalidBoundsMessage);

    reportProgress(progress, "Sizing sun aperture.");
    sunKit->setBox(layoutBox);

    // Preserve historical observable ordering: headless assigns the sun
    // transform before findTexture; GUI assigns it only after success.
    if (sunTransformBeforeSampling) {
        prepared.m_sunInstance->setNode(sunKit);
        prepared.m_sunInstance->setTransform(tgf::makeTransform(sunKit->m_transform));
    }

    reportProgress(progress, "Finding sun aperture cells.");
    if (!sunKit->findTexture(configuration.sunWidthDivisions,
                             configuration.sunHeightDivisions, prepared.m_layoutRoot))
        return fail(errorMessage, "There are no surfaces defined for ray tracing.");

    if (!sunTransformBeforeSampling) {
        prepared.m_sunInstance->setNode(sunKit);
        prepared.m_sunInstance->setTransform(tgf::makeTransform(sunKit->m_transform));
    }

    // Store the entire validated effective configuration once: propagation,
    // result normalization and diagnostics must agree on rays, seed and grid.
    prepared.m_configuration = configuration;
    *context = std::move(prepared);
    return true;
}

bool TracePreparation::prepareGuiTrace(const GuiTracePreparationInput& input,
                                       PreparedTraceContext* context,
                                       QString* errorMessage)
{
    if (!context)
        return fail(errorMessage, "Prepared trace context is null.");
    if (!input.configuration.validate(errorMessage))
        return false;
    if (!input.layoutRoot || !input.sunInstance)
        return fail(errorMessage, "GUI trace preparation is missing its layout or sun instance.");

    PreparedTraceContext prepared;
    SunKit* sunKit = nullptr;
    if (!resolveSun(input.scene, &sunKit, &prepared.m_sunShape, &prepared.m_sunAperture,
                    &prepared.m_sunPosition, errorMessage))
        return false;

    // GUI owns the tree and sun instance; synchronization updates its scene
    // before the one shared analytical preparation pipeline runs.
    if (input.synchronizeScene)
        input.synchronizeScene();

    prepared.m_scene = input.scene;
    prepared.m_layoutRoot = input.layoutRoot;
    prepared.m_sunInstance = input.sunInstance;
    prepared.m_tracingAir = input.tracingAir;
    prepared.m_photonBuffer = input.photonBuffer;
    prepared.m_exportSurfaceList = input.exportSurfaceList;
    prepared.m_hitCallback = input.hitCallback;
    prepared.m_historyCallback = input.historyCallback;

    return prepareResolvedTrace(std::move(prepared), sunKit, input.configuration,
                                TracePreparationProgress(),
                                "GUI scene has no valid analytical ray-tracing bounds.",
                                false, context, errorMessage);
}

bool TracePreparation::prepareHeadlessTrace(const HeadlessTracePreparationInput& input,
                                            PreparedTraceContext* context,
                                            QString* errorMessage)
{
    if (!context)
        return fail(errorMessage, "Prepared trace context is null.");
    if (!input.configuration.validate(errorMessage))
        return false;

    PreparedTraceContext prepared;
    SunKit* sunKit = nullptr;
    if (!resolveSun(input.scene, &sunKit, &prepared.m_sunShape, &prepared.m_sunAperture,
                    &prepared.m_sunPosition, errorMessage))
        return false;

    reportProgress(input.progress, "Preparing scene.");
    // Keep historical opt-in: callers controlling the headless scene choose
    // whether to initialize trackers before building the instance tree.
    if (input.updateTrackers) {
        reportProgress(input.progress, "Updating solar trackers.");
        input.scene->updateParents();
        input.scene->updateTrackers();
    }

    reportProgress(input.progress, "Building ray-tracing instance tree.");
    prepared.m_ownedInstanceTree = SceneInstanceBuilder::build(input.scene);
    if (!prepared.m_ownedInstanceTree.sceneRoot)
        return fail(errorMessage, "Could not create ray-tracing scene root.");
    if (!prepared.m_ownedInstanceTree.layoutRoot)
        return fail(errorMessage, "Scene has no layout at group.");

    prepared.m_scene = input.scene;
    prepared.m_layoutRoot = prepared.m_ownedInstanceTree.layoutRoot;
    prepared.m_ownedSunInstance = std::make_unique<InstanceNode>(sunKit);
    prepared.m_sunInstance = prepared.m_ownedSunInstance.get();

    AirTransmission* air = static_cast<AirTransmission*>(
        input.scene->getPart("world.air.transmission", false));
    // Preserve the headless vacuum policy; GUI adapters continue to supply
    // their own effective non-vacuum air object.
    if (air && air->getTypeId() != AirVacuum::getClassTypeId())
        prepared.m_tracingAir = air;

    prepared.m_photonBuffer = input.photonBuffer;
    prepared.m_hitCallback = input.hitCallback;
    prepared.m_historyCallback = input.historyCallback;

    return prepareResolvedTrace(std::move(prepared), sunKit, input.configuration,
                                input.progress,
                                "Scene layout has no valid ray-tracing bounds. Check that it contains ray-traceable surfaces with shape and profile data.",
                                true, context, errorMessage);
}

void TracePreparation::initializeResult(const PreparedTraceContext& context, RayTraceExecutorResult* result)
{
    if (!result)
        return;
    *result = RayTraceExecutorResult();
    const QVector<ulong> workItems = RayTraceExecutor::guiRaysPerThread(context.m_configuration.rays);
    result->sunApertureArea = context.sunApertureArea();
    result->irradiance = context.irradiance();
    result->powerPerRay = context.powerPerRay();
    result->workerCount = RayTraceExecutor::workerCountForRays(context.m_configuration.rays);
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
        result->raysTraced = exportFailed ? 0 : context.m_configuration.rays;
        result->raysPerSecond = elapsedSeconds > 0.
            ? static_cast<double>(result->raysTraced) / elapsedSeconds
            : 0.;
        result->exportFailed = exportFailed || (context.m_photonBuffer && context.m_photonBuffer->hasExportFailed());
    }
    if (exportFailed)
        return fail(errorMessage, "Photon buffer/export failed during ray tracing.");
    return true;
}
