#include "MaterialRough.h"

#include "libraries/math/3D/Ray.h"
#include "kernel/shape/DifferentialGeometry.h"
#include "kernel/random/Random.h"

#include <cmath>
#include <stdexcept>


SO_NODE_SOURCE(MaterialRough)

void MaterialRough::initClass()
{
     SO_NODE_INIT_CLASS(MaterialRough, MaterialRT, "MaterialRT");
}

MaterialRough::MaterialRough()
{
    SO_NODE_CONSTRUCTOR(MaterialRough);
    isBuiltIn = TRUE;

    SO_NODE_ADD_FIELD(diffuse, (1.) );
    SO_NODE_ADD_FIELD(specular, (0.) );
    SO_NODE_DEFINE_ENUM_VALUE(Distribution, Beckmann);
    SO_NODE_DEFINE_ENUM_VALUE(Distribution, Trowbridge);
    SO_NODE_SET_SF_ENUM_TYPE(distribution, Distribution);
    SO_NODE_ADD_FIELD(distribution, (Beckmann) );
    SO_NODE_ADD_FIELD(roughness, (0.) );

}


// Beckmann and Trowbridge-Reitz are sampled only after the centralized
// categorical event has selected a specular reflection.
MaterialProbabilities MaterialRough::probabilities(
    const Ray&, const DifferentialGeometry&, Random&, MaterialInteractionState&) const
{
    const double alpha = roughness.getValue();
    if (!std::isfinite(alpha) || alpha < 0.)
        throw std::domain_error(
            "Scientific integrity failure: invalid rough-material roughness.");
    // The kernel validates diffuse + specular <= 1. The remainder absorbs.
    return {specular.getValue(), diffuse.getValue(), 0.};
}

void MaterialRough::scatter(MaterialInteraction event, const Ray& rayIn,
    const DifferentialGeometry& dg, Random& rand,
    const MaterialInteractionState&, Ray& rayOut) const
{
    if (event == MaterialInteraction::DiffuseReflection) {
        const double phi = gcf::TwoPi * rand.RandomDouble();
        const double sinTheta = std::sqrt(rand.RandomDouble());
        const double cosTheta = std::sqrt(1. - sinTheta*sinTheta);
        vec3d diffuseDirection(
            sinTheta*std::cos(phi), sinTheta*std::sin(phi), cosTheta);
        const vec3d vx = dg.dpdu.normalized();
        const vec3d vy = dg.dpdv.normalized();
        const vec3d vz = dg.normal.normalized();
        diffuseDirection = vx*diffuseDirection.x
            + vy*diffuseDirection.y + vz*diffuseDirection.z;
        diffuseDirection.normalize();
        if (dot(rayIn.direction(), vz) > 0.)
            diffuseDirection = -diffuseDirection;
        rayOut.setDirection(diffuseDirection);
        return;
    }
    if (event != MaterialInteraction::SpecularReflection)
        throw std::domain_error("Scientific integrity failure: invalid rough material event.");

    const double alpha = roughness.getValue();
    const bool incomingBelow = dot(rayIn.direction(), dg.normal) < 0.;
    // A rough-surface angular sample can point into the solid. Rejection
    // sampling conditions the scattering distribution on geometrically
    // admissible directions without changing the roulette event or R.
    // This is a phenomenological rough-scatter model, not a microfacet BRDF.
    for (int attempt = 0; attempt < 256; ++attempt) {
        vec3d normal;
        if (alpha > 0.) {
            const double phi = gcf::TwoPi * rand.RandomDouble();
            double tan2Theta = 0.;
            if (distribution.getValue() == Distribution::Beckmann) {
                tan2Theta = -gcf::pow2(alpha) * std::log(rand.RandomDouble());
            } else if (distribution.getValue() == Distribution::Trowbridge) {
                const double u = rand.RandomDouble();
                tan2Theta = gcf::pow2(alpha)*u/(1. - u);
            } else {
                throw std::domain_error(
                    "Scientific integrity failure: unsupported rough distribution.");
            }
            if (!std::isfinite(tan2Theta) || tan2Theta < 0.)
                throw std::domain_error(
                    "Scientific integrity failure: invalid rough microfacet normal.");
            const double cosTheta = 1./std::sqrt(1. + tan2Theta);
            const double sinTheta = std::sqrt(1. - cosTheta*cosTheta);
            normal = vec3d(sinTheta*std::cos(phi), sinTheta*std::sin(phi), cosTheta);
            const vec3d vx = dg.dpdu.normalized();
            const vec3d vy = dg.dpdv.normalized();
            const vec3d vz = dg.normal.normalized();
            normal = vx*normal.x + vy*normal.y + vz*normal.z;
            normal.normalize();
        } else {
            normal = dg.normal.normalized();
        }
        const vec3d reflected = rayIn.direction().reflected(normal);
        const bool outgoingBelow = dot(reflected, dg.normal) < 0.;
        if (incomingBelow != outgoingBelow) {
            rayOut.setDirection(reflected);
            return;
        }
        if (alpha == 0.)
            break;
    }
    throw std::domain_error(
        "Scientific integrity failure: no admissible rough reflection direction.");
}
