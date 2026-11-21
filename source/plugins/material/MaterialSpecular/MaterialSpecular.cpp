#include "MaterialSpecular.h"

#include <cmath>
#include <stdexcept>

#include "libraries/math/gcf.h"
#include "libraries/math/3D/Ray.h"
#include "libraries/math/3D/Transform.h"
#include "kernel/shape/DifferentialGeometry.h"
#include "kernel/random/Random.h"
#include "kernel/node/TonatiuhFunctions.h"


SO_NODE_SOURCE(MaterialSpecular)

void MaterialSpecular::initClass()
{
    SO_NODE_INIT_CLASS(MaterialSpecular, MaterialRT, "MaterialRT");
}

MaterialSpecular::MaterialSpecular()
{
    SO_NODE_CONSTRUCTOR(MaterialSpecular);
    isBuiltIn = TRUE;

    SO_NODE_ADD_FIELD(reflectivity, (0.95) ); // 1 may give infinite reflections

    SO_NODE_DEFINE_ENUM_VALUE(Distribution, pillbox);
    SO_NODE_DEFINE_ENUM_VALUE(Distribution, Gaussian);
    SO_NODE_SET_SF_ENUM_TYPE(distribution, Distribution);
    SO_NODE_ADD_FIELD(distribution, (Gaussian) );

    SO_NODE_ADD_FIELD(slope, (0.002) ); // in radians

    // Invalid coefficients are rejected, never silently clamped.
}

MaterialSpecular::~MaterialSpecular() = default;

MaterialProbabilities MaterialSpecular::probabilities(
    const Ray&, const DifferentialGeometry&, Random&, MaterialInteractionState&) const
{
    const double sigma = slope.getValue();
    if (!std::isfinite(sigma) || sigma < 0.)
        throw std::domain_error("Scientific integrity failure: invalid specular slope.");
    return {reflectivity.getValue(), 0., 0.};
}

void MaterialSpecular::scatter(MaterialInteraction event, const Ray& incoming,
    const DifferentialGeometry& dg, Random& rand,
    const MaterialInteractionState&, Ray& outgoing) const
{
    if (event != MaterialInteraction::SpecularReflection)
        throw std::domain_error("Scientific integrity failure: invalid specular event.");
    vec3d normal;
    const double sigma = slope.getValue();
    if (sigma > 0.) {
        if (distribution.getValue() == Distribution::pillbox) {
            const double phi = gcf::TwoPi*rand.RandomDouble();
            const double sinTheta = std::sin(sigma)*std::sqrt(rand.RandomDouble());
            const double cosTheta = std::sqrt(1. - sinTheta*sinTheta);
            normal = vec3d(sinTheta*std::cos(phi), sinTheta*std::sin(phi), cosTheta);
        } else if (distribution.getValue() == Distribution::Gaussian) {
            double u, v, s;
            do {
                u = 2.*rand.RandomDouble() - 1.;
                v = 2.*rand.RandomDouble() - 1.;
                s = u*u + v*v;
            } while (s > 1. || s == 0.);
            const double scale = sigma*std::sqrt(-2.*std::log(s)/s);
            normal = vec3d(scale*u, scale*v, 1.);
        } else {
            throw std::domain_error(
                "Scientific integrity failure: unsupported specular distribution.");
        }
        const vec3d vx = dg.dpdu.normalized();
        const vec3d vy = dg.dpdv.normalized();
        const vec3d vz = dg.normal.normalized();
        normal = vx*normal.x + vy*normal.y + vz*normal.z;
        normal.normalize();
    } else {
        normal = dg.normal.normalized();
    }
    const vec3d reflected = incoming.direction().reflected(normal);
    const bool incomingBelow = dot(incoming.direction(), dg.normal) < 0.;
    const bool outgoingBelow = dot(reflected, dg.normal) < 0.;
    if (incomingBelow == outgoingBelow)
        throw std::domain_error(
            "Scientific integrity failure: specular reflection exits wrong hemisphere.");
    outgoing.setDirection(reflected);
}
