#include "ScriptInterceptionService.h"

#include <atomic>
#include <cmath>
#include <functional>

#include "core/SceneInstanceBuilder.h"
#include "core/SimulationSession.h"
#include "kernel/run/InstanceNode.h"
#include "kernel/run/RayTracer.h"
#include "kernel/scene/TSceneKit.h"
#include "kernel/scene/TShapeKit.h"

namespace
{
bool fail(QString* error, const QString& message)
{
    if (error)
        *error = message;
    return false;
}
}

bool ScriptInterceptionService::findPower(TSceneKit* scene,
                                         const QString& surfaceUrl,
                                         ulong rays,
                                         std::uint64_t masterSeed,
                                         double* power,
                                         QString* error)
{
    if (error)
        error->clear();
    if (!power)
        return fail(error, "tn.FindInterception: result destination is unavailable.");
    *power = 0.;
    if (!scene)
        return fail(error, "tn.FindInterception requires an active scene.");
    if (surfaceUrl.trimmed().isEmpty())
        return fail(error, "tn.FindInterception requires a non-empty surface URL.");
    if (rays == 0)
        return fail(error, "tn.FindInterception requires a positive integer ray count.");

    // Check the complete instance URL, not only the underlying Coin node.
    // A shared node can have several placements with different transforms.
    // This is validation only; SimulationSession owns its actual trace tree.
    SceneInstanceTree inspection = SceneInstanceBuilder::build(scene);
    if (!inspection.sceneRoot || !inspection.layoutRoot)
        return fail(error, "tn.FindInterception: scene has no layout.");
    int targets = 0;
    const std::function<void(const InstanceNode*)> visit = [&](const InstanceNode* instance) {
        if (!instance)
            return;
        if (instance->getURL() == surfaceUrl) {
            if (dynamic_cast<TShapeKit*>(instance->getNode()))
                ++targets;
        }
        for (const InstanceNode* child : instance->children)
            visit(child);
    };
    visit(inspection.sceneRoot.get());
    if (targets == 0)
        return fail(error, QString("tn.FindInterception: no shape at '%1'.").arg(surfaceUrl));
    if (targets != 1)
        return fail(error, QString("tn.FindInterception: ambiguous shape URL '%1'.").arg(surfaceUrl));

    // The callback is observation-only: no change to ray propagation, random
    // sampling, or photon exporter behavior. Atomic accumulation is independent
    // of worker execution order and uses no unbounded photon-history buffer.
    std::atomic<qulonglong> frontHits{0};
    HeadlessTracePreparationInput input;
    input.scene = scene;
    input.updateTrackers = true;
    input.configuration.rays = rays;
    input.configuration.masterSeed = masterSeed;
    input.hitCallback = [&frontHits, &surfaceUrl](const RayTracerHit& hit) {
        if (hit.isFront && hit.surface && hit.surface->getURL() == surfaceUrl)
            frontHits.fetch_add(1, std::memory_order_relaxed);
    };

    SimulationSession session;
    QString sessionError;
    if (!session.prepareHeadless(input, &sessionError))
        return fail(error, QString("tn.FindInterception preparation failed: %1").arg(sessionError));
    if (!session.start(&sessionError) || !session.waitForFinished(&sessionError))
        return fail(error, QString("tn.FindInterception trace failed: %1").arg(sessionError));
    if (session.state() != SimulationSession::State::Completed
        || session.result().raysTraced != rays)
        return fail(error, "tn.FindInterception: trace was incomplete.");

    const double result = static_cast<double>(frontHits.load(std::memory_order_relaxed))
        * session.result().powerPerRay;
    if (!std::isfinite(result) || result < 0.)
        return fail(error, "tn.FindInterception: non-finite intercepted power.");
    *power = result;
    return true;
}
