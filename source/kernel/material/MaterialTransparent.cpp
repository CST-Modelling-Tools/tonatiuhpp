#include "MaterialTransparent.h"

#include "libraries/math/3D/Ray.h"
#include "kernel/shape/DifferentialGeometry.h"
#include "kernel/random/Random.h"

#include <stdexcept>


SO_NODE_SOURCE(MaterialTransparent)

void MaterialTransparent::initClass()
{
     SO_NODE_INIT_CLASS(MaterialTransparent, MaterialRT, "MaterialRT");
}

MaterialTransparent::MaterialTransparent()
{
    SO_NODE_CONSTRUCTOR(MaterialTransparent);
    isBuiltIn = TRUE;
}

MaterialProbabilities MaterialTransparent::probabilities(
    const Ray&, const DifferentialGeometry&, Random&, MaterialInteractionState&) const
{
    return {0., 0., 1.};
}

void MaterialTransparent::scatter(MaterialInteraction event, const Ray& incoming,
    const DifferentialGeometry&, Random&, const MaterialInteractionState&, Ray& outgoing) const
{
    if (event != MaterialInteraction::Transmission)
        throw std::logic_error("Scientific integrity failure: invalid transparent material event.");
    outgoing.setDirection(incoming.direction());
}
