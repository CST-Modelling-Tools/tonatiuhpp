#pragma once

#include <cmath>
#include <limits>
#include <utility>
#include <vector>

#include <QString>

#include "kernel/photons/Photon.h"
#include "kernel/run/InstanceNode.h"
#include "kernel/scene/TShapeKit.h"
#include "kernel/shape/ShapeRT.h"
#include "kernel/profiles/ProfileRT.h"
#include "libraries/math/2D/Box2D.h"
#include "libraries/math/2D/Matrix2D.h"
#include "libraries/math/2D/vec2i.h"
#include "libraries/math/3D/Transform.h"

// M3: nonvisual, deterministic legacy flux-grid calculation. No widgets,
// SceneTreeModel, exporter, global script host, or separate ray tracer.
// Both GUI and future true-headless recording-mode sessions can pass their
// selected-surface photon records through this identical computation.
struct FluxGridComputationResult
{
    Matrix2D<int> binsPhotons;
    Matrix2D<double> binsFlux;
    Box2D box;
    int photonsMax = 0;
    vec2i photonsMaxPos;
    int photonsError = 0;
    double powerTotal = 0.;
};

namespace FluxGridComputation
{
inline bool fail(QString* error, const QString& message)
{
    if (error)
        *error = message;
    return false;
}

inline bool calculate(const std::vector<Photon>& photons,
                      InstanceNode* surface,
                      const QString& surfaceSide,
                      int rows, int cols,
                      double powerPhoton,
                      FluxGridComputationResult* output,
                      QString* error = nullptr)
{
    if (error)
        error->clear();
    if (!output || !surface)
        return fail(error, "Flux grid has no result destination or selected surface.");
    if (rows < 2 || cols < 2 || static_cast<qint64>(rows) * cols > std::numeric_limits<int>::max())
        return fail(error, "Flux grid dimensions must be at least 2 and fit the matrix index range.");
    if ((surfaceSide != "front" && surfaceSide != "back") ||
        !std::isfinite(powerPhoton) || powerPhoton < 0.)
        return fail(error, "Flux grid has an invalid side or photon power.");

    auto* shapeKit = dynamic_cast<TShapeKit*>(surface->getNode());
    if (!shapeKit)
        return fail(error, "Flux grid selected surface is not a shape.");
    auto* shape = dynamic_cast<ShapeRT*>(shapeKit->shapeRT.getValue());
    auto* profile = dynamic_cast<ProfileRT*>(shapeKit->profileRT.getValue());
    if (!shape || !profile)
        return fail(error, "Flux grid selected surface has no shape or profile.");

    FluxGridComputationResult result;
    result.box = profile->getBox();
    const vec2d boxSize = result.box.size();
    if (!result.box.isValid() || !std::isfinite(boxSize.x) ||
        !std::isfinite(boxSize.y) || boxSize.x <= 0. || boxSize.y <= 0.)
        return fail(error, "Flux grid profile bounds are invalid.");
    result.binsPhotons.resize(rows, cols);
    result.binsPhotons.fill(0);
    Matrix2D<int> binErrors(rows - 1, cols - 1);
    binErrors.fill(0);

    // Preserve the legacy transform, UV mapping, side selection and bin
    // boundaries. The range checks only reject data that would previously
    // index beyond the QVector, rather than silently miscount it.
    const bool activeIsFront = surfaceSide == "front";
    const Transform toWorld = surface->getTransform();
    const Transform toObject = toWorld.inversed();
    int photonsTotal = 0;
    for (const Photon& photon : photons) {
        if (photon.isFront != activeIsFront)
            continue;
        const vec3d p = toObject.transformPoint(photon.pos);
        const vec2d uv = shape->getUV(p);
        const vec2d q = (uv - result.box.min()) / boxSize;
        if (!std::isfinite(q.x) || !std::isfinite(q.y))
            return fail(error, "Flux grid photon has non-finite UV coordinates.");
        int r = static_cast<int>(std::floor(q.x * rows));
        int c = static_cast<int>(std::floor(q.y * cols));
        if (r == rows) --r;
        if (c == cols) --c;
        if (r < 0 || r >= rows || c < 0 || c >= cols)
            return fail(error, "Flux grid photon lies outside selected profile bounds.");
        if (photonsTotal == std::numeric_limits<int>::max())
            return fail(error, "Flux grid photon count exceeds integer range.");
        ++photonsTotal;
        int& bin = result.binsPhotons(r, c);
        ++bin;
        if (result.photonsMax < bin) {
            result.photonsMax = bin;
            result.photonsMaxPos = vec2i(r, c);
        }

        int rE = static_cast<int>(std::floor(q.x * binErrors.rows()));
        int cE = static_cast<int>(std::floor(q.y * binErrors.cols()));
        if (rE == binErrors.rows()) --rE;
        if (cE == binErrors.cols()) --cE;
        if (rE < 0 || rE >= binErrors.rows() || cE < 0 || cE >= binErrors.cols())
            return fail(error, "Flux grid reduced-grid coordinate is invalid.");
        int& binE = binErrors(rE, cE);
        ++binE;
        if (result.photonsError < binE)
            result.photonsError = binE;
    }

    result.powerTotal = photonsTotal * powerPhoton;
    result.binsFlux.resize(rows, cols);
    const double uStep = boxSize.x / rows;
    const double vStep = boxSize.y / cols;
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            const double u0 = result.box.min().x + r * uStep;
            const double v0 = result.box.min().y + c * vStep;
            const double area = shape->findArea(u0, v0, u0 + uStep, v0 + vStep, toWorld);
            if (!std::isfinite(area) || area == 0.)
                return fail(error, "Flux grid cell area is zero or non-finite.");
            result.binsFlux(r, c) = result.binsPhotons(r, c) * powerPhoton / area;
        }
    }
    *output = std::move(result);
    return true;
}
}
