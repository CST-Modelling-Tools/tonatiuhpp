#pragma once

#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>

#include <QDir>
#include <QFileInfo>
#include <QString>

#include "core/FluxGridComputation.h"
#include "core/SceneInstanceBuilder.h"
#include "core/SimulationSession.h"
#include "kernel/photons/PhotonsBuffer.h"
#include "kernel/run/InstanceNode.h"
#include "kernel/scene/TSceneKit.h"
#include "kernel/scene/TShapeKit.h"
#include "kernel/shape/ShapeRT.h"
#include "run/FluxAnalysisTsvWriter.h"
#include "script/ScriptFluxAnalysisRequest.h"

// M3 integrated candidate: one widget-free, synchronous Flux Analysis backend
// for true-headless scripts and BOTH seven/eight-argument GUI scripts.
// Interactive FluxAnalysisDialog remains a GUI presentation adapter, not a
// scientific JS execution dependency.
// No ray tracer, RNG algorithm, photon format, or flux formulas are duplicated.
//
// The caller owns the scene for the entire call. Every worker is joined before
// the locally owned photon buffer and instance tree are destroyed. The bounded
// ray limit prevents a script from requesting unlimited retained photon data.
class HeadlessScriptFluxBackend final
{
public:
    static constexpr uint kMaximumRecordedRays = 200000u;
    static constexpr qint64 kMaximumGridCells = 1000000;

    static bool run(TSceneKit* scene, const ScriptFluxAnalysisRequest& request,
                    QString* error = nullptr)
    {
        if (error)
            error->clear();
        if (!scene)
            return fail(error, "active scene is unavailable.");
        if (request.rays == 0 || request.rays > kMaximumRecordedRays)
            return fail(error, "nonvisual flux tracing supports 1..200000 rays per call.");
        if (request.heightDivisions < 2 || request.widthDivisions < 2
            || static_cast<qint64>(request.heightDivisions) * request.widthDivisions
                > kMaximumGridCells)
            return fail(error, "invalid flux grid dimensions.");
        if (request.surfaceURL.trimmed().isEmpty()
            || (request.surfaceSide != "front" && request.surfaceSide != "back"))
            return fail(error, "invalid selected surface URL or side.");
        if (request.sunWidthDivisions <= 0 || request.sunHeightDivisions <= 0)
            return fail(error, "invalid sun grid dimensions.");
        if (request.fileName.trimmed().isEmpty())
            return fail(error, "output path is empty.");

        const QFileInfo destination(request.fileName);
        if (!destination.isAbsolute())
            return fail(error, "output path must be absolute after script-host resolution.");
        if (!destination.dir().exists())
            return fail(error, QString("Cannot open flux output '%1': parent directory does not exist.")
                                   .arg(request.fileName));
        // One domain policy for both hosts: unseeded seven-argument calls
        // preserve historical file replacement, seeded calls fail closed.
        // The shared TSV writer uses QSaveFile so a failed write cannot
        // truncate the existing scientific artifact.
        if (destination.exists() && !request.replaceExistingOutput)
            return fail(error, QString("flux output '%1' already exists; refusing to overwrite it.")
                                   .arg(request.fileName));

        // Build one borrowed tree and retain it until after the joined session
        // AND the shared numerical flux calculation. All photon.surface pointers
        // refer to this tree, not to a different validation-only tree.
        SceneInstanceTree tree = SceneInstanceBuilder::build(scene);
        if (!tree.sceneRoot || !tree.layoutRoot)
            return fail(error, "scene has no ray-traceable layout.");
        InstanceNode* target = nullptr;
        int matches = 0;
        const std::function<void(InstanceNode*)> findTarget = [&](InstanceNode* node) {
            if (!node) return;
            if (node->getURL() == request.surfaceURL
                && dynamic_cast<TShapeKit*>(node->getNode())) {
                target = node;
                ++matches;
            }
            for (InstanceNode* child : node->children)
                findTarget(child);
        };
        findTarget(tree.sceneRoot.get());
        if (matches != 1)
            return fail(error, QString("selected surface '%1' has %2 shape matches (expected 1).")
                                   .arg(request.surfaceURL).arg(matches));

        // size=0 means retain photons without mid-trace flush. A positive
        // maximum without an exporter would flush/drop old photons instead.
        // No enormous legacy GUI reserve is made for short headless runs.
        PhotonsBuffer photonBuffer(0);
        HeadlessTracePreparationInput input;
        input.scene = scene;
        input.photonBuffer = &photonBuffer;
        input.updateTrackers = true;
        input.borrowedInstanceTree = &tree;
        input.exportSurfaceUrls = {request.surfaceURL};
        input.configuration.rays = request.rays;
        input.configuration.masterSeed = request.masterSeed.value_or(0);
        input.configuration.sunWidthDivisions = request.sunWidthDivisions;
        input.configuration.sunHeightDivisions = request.sunHeightDivisions;

        SimulationSession session;
        QString sessionError;
        if (!session.prepareHeadless(input, &sessionError)
            || !session.start(&sessionError)
            || !session.waitForFinished(&sessionError))
            return fail(error, "trace was not completed: " + sessionError);
        if (session.state() != SimulationSession::State::Completed
            || session.exportFailed() || session.result().raysTraced != request.rays)
            return fail(error, "trace was incomplete; partial photons are not accepted.");

        const double powerPerRay = session.result().powerPerRay;
        FluxGridComputationResult grid;
        QString gridError;
        if (!FluxGridComputation::calculate(photonBuffer.getPhotons(), target,
                                             request.surfaceSide,
                                             request.heightDivisions,
                                             request.widthDivisions,
                                             powerPerRay, &grid, &gridError))
            return fail(error, "flux grid could not be computed: " + gridError);

        auto* kit = dynamic_cast<TShapeKit*>(target->getNode());
        auto* shape = kit ? dynamic_cast<ShapeRT*>(kit->shapeRT.getValue()) : nullptr;
        if (!shape)
            return fail(error, "selected flux surface lost its shape.");
        // Do not replace the legacy TSV normalization with the in-memory
        // findArea() map here. Keep the accepted derivative-based file format
        // until a separate scientifically reviewed M4 normalization change.
        const vec3d scales = target->getTransform().getScales();
        const double uStep = grid.box.size().x / grid.binsPhotons.rows();
        const double vStep = grid.box.size().y / grid.binsPhotons.cols();
        double areaCell = dot(scales, shape->getDerivativeU(0., 0.)) * uStep;
        areaCell *= dot(scales, shape->getDerivativeV(0., 0.)) * vStep;
        if (!std::isfinite(areaCell) || areaCell == 0.)
            return fail(error, "legacy flux TSV cell area is zero or non-finite.");

        // Recheck after ray execution: do not silently replace an output
        // that appeared while a seeded no-overwrite trace was running.
        if (!request.replaceExistingOutput && QFileInfo(request.fileName).exists())
            return fail(error, QString("flux output '%1' already exists; refusing to overwrite it.")
                                   .arg(request.fileName));
        QString outputError;
        if (!FluxAnalysisTsvWriter::write(request.fileName, grid.binsPhotons, grid.box,
                                          powerPerRay / areaCell,
                                          request.saveCoordinates, &outputError))
            return fail(error, outputError);
        return true;
    }

private:
    static bool fail(QString* error, const QString& message)
    {
        if (error)
            *error = "tn.RunFluxAnalysis: " + message;
        return false;
    }
};
