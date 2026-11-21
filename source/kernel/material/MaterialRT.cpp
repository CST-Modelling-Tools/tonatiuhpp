#include "MaterialRT.h"

#include "kernel/random/Random.h"
#include "kernel/random/RussianRoulette.h"
#include "kernel/shape/DifferentialGeometry.h"
#include "libraries/math/3D/Ray.h"

#include <cmath>
#include <stdexcept>

SO_NODE_ABSTRACT_SOURCE(MaterialRT)

void MaterialRT::initClass()
{
    SO_NODE_INIT_ABSTRACT_CLASS(MaterialRT, TNode, "TNode");
}

MaterialInteraction MaterialRT::interact(
    const Ray& incoming, const DifferentialGeometry& geometry,
    Random& random, Ray& outgoing) const
{
    MaterialInteractionState state;
    const MaterialProbabilities p = probabilities(incoming, geometry, random, state);
    const auto chosen = RussianRoulette::sample(
        random, p.specular, p.diffuse, p.transmission);
    if (chosen == RussianRoulette::Outcome::Absorbed)
        return MaterialInteraction::Absorbed;

    MaterialInteraction event = MaterialInteraction::Transmission;
    if (chosen == RussianRoulette::Outcome::First)
        event = MaterialInteraction::SpecularReflection;
    else if (chosen == RussianRoulette::Outcome::Second)
        event = MaterialInteraction::DiffuseReflection;

    // The physical surface hit sets origin, not a plugin-specific convention.
    outgoing.origin = geometry.point;
    scatter(event, incoming, geometry, random, state, outgoing);
    outgoing.origin = geometry.point;

    const vec3d& d = outgoing.direction();
    const vec3d& o = outgoing.origin;
    const double normSquared = dot(d, d);
    if (!std::isfinite(o.x) || !std::isfinite(o.y) || !std::isfinite(o.z)
        || !std::isfinite(d.x) || !std::isfinite(d.y) || !std::isfinite(d.z)
        || !std::isfinite(normSquared) || normSquared <= 0.)
        throw std::domain_error(
            "Scientific integrity failure: material produced an invalid outgoing ray.");
    return event;
}
