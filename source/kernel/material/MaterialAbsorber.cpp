#include "MaterialAbsorber.h"

#include "libraries/math/3D/Ray.h"
#include "kernel/shape/DifferentialGeometry.h"
#include "kernel/random/Random.h"

#include <stdexcept>


SO_NODE_SOURCE(MaterialAbsorber)

void MaterialAbsorber::initClass()
{
     SO_NODE_INIT_CLASS(MaterialAbsorber, MaterialRT, "MaterialRT");
}

MaterialAbsorber::MaterialAbsorber()
{
    SO_NODE_CONSTRUCTOR(MaterialAbsorber);
    isBuiltIn = TRUE;
}

MaterialProbabilities MaterialAbsorber::probabilities(
    const Ray&, const DifferentialGeometry&, Random&, MaterialInteractionState&) const
{
    return {};
}

void MaterialAbsorber::scatter(MaterialInteraction, const Ray&,
    const DifferentialGeometry&, Random&, const MaterialInteractionState&, Ray&) const
{
    // Shared selection can never reach scattering for an ideal absorber.
    throw std::logic_error("Scientific integrity failure: absorber cannot scatter.");
}
