#include "MaterialFresnelUnpolarized.h"

#include "libraries/math/3D/Ray.h"
#include "kernel/shape/DifferentialGeometry.h"
#include "kernel/random/Random.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>


SO_NODE_SOURCE(MaterialFresnelUnpolarized)

void MaterialFresnelUnpolarized::initClass()
{
     SO_NODE_INIT_CLASS(MaterialFresnelUnpolarized, MaterialRT, "MaterialRT");
}

MaterialFresnelUnpolarized::MaterialFresnelUnpolarized()
{
    SO_NODE_CONSTRUCTOR(MaterialFresnelUnpolarized);
//    isBuiltIn = TRUE;
    SO_NODE_ADD_FIELD( nFront, (1.) );// air
    SO_NODE_ADD_FIELD( nBack, (1.5) );// glass

    SO_NODE_DEFINE_ENUM_VALUE(Distribution, pillbox);
    SO_NODE_DEFINE_ENUM_VALUE(Distribution, Gaussian);
    SO_NODE_SET_SF_ENUM_TYPE(distribution, Distribution);
    SO_NODE_ADD_FIELD(distribution, (Gaussian) );

    SO_NODE_ADD_FIELD(slope, (0.002) ); // in radians
}

MaterialProbabilities MaterialFresnelUnpolarized::probabilities(
    const Ray& incoming, const DifferentialGeometry& dg, Random& rand,
    MaterialInteractionState& state) const
{
    const double frontIndex = nFront.getValue();
    const double backIndex = nBack.getValue();
    const double sigma = slope.getValue();
    if (!std::isfinite(frontIndex) || !std::isfinite(backIndex)
        || frontIndex <= 0. || backIndex <= 0.
        || !std::isfinite(sigma) || sigma < 0.)
        throw std::domain_error(
            "Scientific integrity failure: invalid Fresnel refractive index or slope.");

    vec3d normal;
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
                "Scientific integrity failure: unsupported Fresnel distribution.");
        }
        const vec3d vx = dg.dpdu.normalized();
        const vec3d vy = dg.dpdv.normalized();
        const vec3d vz = dg.normal.normalized();
        normal = vx*normal.x + vy*normal.y + vz*normal.z;
        normal.normalize();
    } else {
        normal = dg.normal.normalized();
    }
    const vec3d& direction = incoming.direction();
    double cosine = dot(direction, normal);
    double incidentIndex = frontIndex;
    double transmittedIndex = backIndex;
    if (cosine > 0.) {
        std::swap(incidentIndex, transmittedIndex);
        normal = -normal;
    }
    cosine = std::fabs(cosine);
    if (!std::isfinite(cosine) || cosine > 1. + 1e-10)
        throw std::domain_error(
            "Scientific integrity failure: invalid Fresnel incident cosine.");
    cosine = std::min(1., cosine);
    const double ratio = incidentIndex/transmittedIndex;
    const double sinTransmittedSquared = ratio*ratio*(1. - cosine*cosine);
    double reflectedFraction = 1.;
    double cosineTransmitted = 0.;

    if (incidentIndex == transmittedIndex) {
        // Index-matched media do not reflect even at exact grazing incidence.
        reflectedFraction = 0.;
        cosineTransmitted = cosine;
    } else if (sinTransmittedSquared < 1.) {
        cosineTransmitted = std::sqrt(1. - sinTransmittedSquared);
        const double rs =
            (incidentIndex*cosine - transmittedIndex*cosineTransmitted)
            /(incidentIndex*cosine + transmittedIndex*cosineTransmitted);
        const double rp =
            (transmittedIndex*cosine - incidentIndex*cosineTransmitted)
            /(transmittedIndex*cosine + incidentIndex*cosineTransmitted);
        reflectedFraction = (rs*rs + rp*rp)*0.5;
    }
    state.microfacetNormal = normal;
    state.incidentIndex = incidentIndex;
    state.transmittedIndex = transmittedIndex;
    state.cosineTransmitted = cosineTransmitted;
    return {reflectedFraction, 0., 1. - reflectedFraction};
}

void MaterialFresnelUnpolarized::scatter(MaterialInteraction event,
    const Ray& incoming, const DifferentialGeometry&, Random&,
    const MaterialInteractionState& state, Ray& outgoing) const
{
    if (event == MaterialInteraction::SpecularReflection) {
        outgoing.setDirection(incoming.direction().reflected(state.microfacetNormal));
    } else if (event == MaterialInteraction::Transmission) {
        const double ratio = state.incidentIndex/state.transmittedIndex;
        outgoing.setDirection(ratio*incoming.direction().projected(state.microfacetNormal)
            - state.cosineTransmitted*state.microfacetNormal);
    } else {
        throw std::domain_error("Scientific integrity failure: invalid Fresnel event.");
    }
}
