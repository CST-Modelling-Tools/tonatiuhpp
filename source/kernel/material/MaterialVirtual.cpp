#include "MaterialVirtual.h"

#include "libraries/math/3D/Ray.h"
#include "kernel/shape/DifferentialGeometry.h"
#include "kernel/random/Random.h"

#include <stdexcept>


SO_NODE_SOURCE(MaterialVirtual)

void MaterialVirtual::initClass()
{
     SO_NODE_INIT_CLASS(MaterialVirtual, MaterialRT, "MaterialRT");
}

MaterialVirtual::MaterialVirtual()
{
    SO_NODE_CONSTRUCTOR(MaterialVirtual);
    isBuiltIn = TRUE;
}

MaterialProbabilities MaterialVirtual::probabilities(
    const Ray&, const DifferentialGeometry&, Random&, MaterialInteractionState&) const
{
    return {0., 0., 1.};
}

void MaterialVirtual::scatter(MaterialInteraction event, const Ray& incoming,
    const DifferentialGeometry&, Random&, const MaterialInteractionState&, Ray& outgoing) const
{
    if (event != MaterialInteraction::Transmission)
        throw std::logic_error("Scientific integrity failure: invalid virtual material event.");
    outgoing.setDirection(incoming.direction());
}
