#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <initializer_list>
#include <vector>

#include <gtest/gtest.h>

#include "kernel/material/MaterialAbsorber.h"
#include "kernel/material/MaterialFresnelUnpolarized.h"
#include "kernel/material/MaterialRough.h"
#include "kernel/material/MaterialTransparent.h"
#include "kernel/material/MaterialVirtual.h"
#include "kernel/random/Random.h"
#include "kernel/random/StandardRandom.h"
#include "kernel/random/RussianRoulette.h"
#include "kernel/shape/DifferentialGeometry.h"
#include "libraries/math/3D/Ray.h"

namespace
{
class FixedRandom final : public Random
{
public:
    explicit FixedRandom(double value): m_value(value) {}
    double RandomDouble() override { ++draws; return m_value; }
    int draws = 0;
private:
    double m_value;
};

class SequenceRandom final : public Random
{
public:
    explicit SequenceRandom(std::initializer_list<double> values): m_values(values) {}
    double RandomDouble() override
    {
        if (m_position >= m_values.size())
            throw std::logic_error("Unexpected extra random draw.");
        return m_values[m_position++];
    }
    std::size_t draws() const { return m_position; }
private:
    std::vector<double> m_values;
    std::size_t m_position = 0;
};

struct MaterialHandle
{
    explicit MaterialHandle(MaterialRT* material): m_material(material) { m_material->ref(); }
    ~MaterialHandle() { m_material->unref(); }
    MaterialRT* m_material;
};

DifferentialGeometry planarHit()
{
    DifferentialGeometry dg;
    dg.point = vec3d(2., 3., 0.);
    dg.dpdu = vec3d(1., 0., 0.);
    dg.dpdv = vec3d(0., 1., 0.);
    dg.normal = vec3d(0., 0., 1.);
    dg.isFront = true;
    return dg;
}

Ray normalIncoming()
{
    return Ray(vec3d(2., 3., 1.), vec3d(0., 0., -1.));
}
}

TEST(RussianRouletteScience, CategoricalIntervalsUseOneDraw)
{
    FixedRandom a(0.199999), b(0.2), c(0.499999), d(0.5);
    FixedRandom e(0.599999), f(0.6);
    EXPECT_EQ(RussianRoulette::sample(a, 0.2, 0.3, 0.1), RussianRoulette::Outcome::First);
    EXPECT_EQ(RussianRoulette::sample(b, 0.2, 0.3, 0.1), RussianRoulette::Outcome::Second);
    EXPECT_EQ(RussianRoulette::sample(c, 0.2, 0.3, 0.1), RussianRoulette::Outcome::Second);
    EXPECT_EQ(RussianRoulette::sample(d, 0.2, 0.3, 0.1), RussianRoulette::Outcome::Third);
    EXPECT_EQ(RussianRoulette::sample(e, 0.2, 0.3, 0.1), RussianRoulette::Outcome::Third);
    EXPECT_EQ(RussianRoulette::sample(f, 0.2, 0.3, 0.1), RussianRoulette::Outcome::Absorbed);
    EXPECT_EQ(a.draws+b.draws+c.draws+d.draws+e.draws+f.draws, 6);
}

TEST(RussianRouletteScience, ZeroAndUnitProbabilitiesRemainExact)
{
    FixedRandom zero(0.), half(0.5);
    EXPECT_EQ(RussianRoulette::sample(zero, 0.), RussianRoulette::Outcome::Absorbed);
    EXPECT_EQ(RussianRoulette::sample(half, 1.), RussianRoulette::Outcome::First);
    EXPECT_EQ(RussianRoulette::sample(half, 0., 1.), RussianRoulette::Outcome::Second);
    EXPECT_EQ(RussianRoulette::sample(half, 0., 0., 1.), RussianRoulette::Outcome::Third);
    EXPECT_EQ(zero.draws, 1);
    EXPECT_EQ(half.draws, 3);
}

TEST(RussianRouletteScience, InvalidProbabilitiesRejectedBeforeRngUse)
{
    FixedRandom rng(0.5);
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    EXPECT_THROW(RussianRoulette::sample(rng, -0.1), std::domain_error);
    EXPECT_THROW(RussianRoulette::sample(rng, 1.1), std::domain_error);
    EXPECT_THROW(RussianRoulette::sample(rng, nan), std::domain_error);
    EXPECT_THROW(RussianRoulette::sample(rng, inf), std::domain_error);
    EXPECT_THROW(RussianRoulette::sample(rng, 0.6, 0.5), std::domain_error);
    EXPECT_THROW(RussianRoulette::sample(rng, 0.1, 0.3, 0.7), std::domain_error);
    EXPECT_EQ(rng.draws, 0);
}

TEST(RussianRouletteScience, InvalidRandomDrawIsRejected)
{
    FixedRandom neg(-0.01), one(1.), nan(std::numeric_limits<double>::quiet_NaN());
    EXPECT_THROW(RussianRoulette::sample(neg, 0.9), std::domain_error);
    EXPECT_THROW(RussianRoulette::sample(one, 0.9), std::domain_error);
    EXPECT_THROW(RussianRoulette::sample(nan, 0.9), std::domain_error);
}

TEST(RussianRouletteScience, NinetyPercentReflectionMatchesBernoulliStatistics)
{
    StandardRandom rng(123456789ULL);
    constexpr int n = 200000;
    int reflections = 0;
    for (int k = 0; k < n; ++k)
        if (RussianRoulette::sample(rng, 0.9) == RussianRoulette::Outcome::First)
            ++reflections;
    EXPECT_NEAR(static_cast<double>(reflections)/n, 0.9, 0.004);
}

TEST(RussianRouletteScience, ThreeWayFrequenciesMatchPrescribedProbabilities)
{
    StandardRandom rng(987654321ULL);
    constexpr int n = 200000;
    std::array<int, 4> events{};
    for (int k = 0; k < n; ++k)
        ++events[static_cast<int>(RussianRoulette::sample(rng, 0.3, 0.4, 0.2))];
    EXPECT_NEAR(static_cast<double>(events[0])/n, 0.3, 0.005);
    EXPECT_NEAR(static_cast<double>(events[1])/n, 0.4, 0.005);
    EXPECT_NEAR(static_cast<double>(events[2])/n, 0.2, 0.004);
    EXPECT_NEAR(static_cast<double>(events[3])/n, 0.1, 0.004);
    EXPECT_EQ(events[0]+events[1]+events[2]+events[3], n);
}

TEST(RussianRouletteScience, RoughSpecularEventUsesOneSelectionAndCorrectOrigin)
{
    auto* rough = new MaterialRough;
    MaterialHandle hold(rough);
    rough->diffuse = 0.6;
    rough->specular = 0.2;
    rough->roughness = 0.;
    FixedRandom rng(0.1);
    Ray outgoing;
    EXPECT_EQ(rough->interact(normalIncoming(), planarHit(), rng, outgoing),
              MaterialInteraction::SpecularReflection);
    EXPECT_EQ(rng.draws, 1);
    EXPECT_EQ(outgoing.origin, planarHit().point);
    EXPECT_EQ(outgoing.direction(), vec3d(0., 0., 1.));
}

TEST(RussianRouletteScience, RoughDiffuseEventHasCorrectOriginAndHemisphere)
{
    auto* rough = new MaterialRough;
    MaterialHandle hold(rough);
    rough->diffuse = 0.6;
    rough->specular = 0.2;
    rough->roughness = 0.;
    FixedRandom rng(0.5);
    Ray outgoing;
    EXPECT_EQ(rough->interact(normalIncoming(), planarHit(), rng, outgoing),
              MaterialInteraction::DiffuseReflection);
    EXPECT_EQ(rng.draws, 3);
    EXPECT_EQ(outgoing.origin, planarHit().point);
    EXPECT_GT(dot(outgoing.direction(), planarHit().normal), 0.);
}

TEST(RussianRouletteScience, RoughAbsorptionIsAnExplicitEvent)
{
    auto* rough = new MaterialRough;
    MaterialHandle hold(rough);
    rough->diffuse = 0.6;
    rough->specular = 0.2;
    FixedRandom rng(0.95);
    Ray outgoing;
    EXPECT_EQ(rough->interact(normalIncoming(), planarHit(), rng, outgoing),
              MaterialInteraction::Absorbed);
    EXPECT_EQ(rng.draws, 1);
}

TEST(RussianRouletteScience, RoughGeometricRejectionResamplesWithoutExtraRoulette)
{
    auto* rough = new MaterialRough;
    MaterialHandle hold(rough);
    rough->diffuse = 0.;
    rough->specular = 1.;
    rough->roughness = 0.2;
    rough->distribution = MaterialRough::Trowbridge;
    // Categorical draw, then an inadmissible and an admissible Trowbridge
    // microfacet. The optical event remains specular throughout.
    SequenceRandom rng({0.5, 0., 0.99, 0., 0.1});
    Ray outgoing;
    EXPECT_EQ(rough->interact(normalIncoming(), planarHit(), rng, outgoing),
              MaterialInteraction::SpecularReflection);
    EXPECT_GT(dot(outgoing.direction(), planarHit().normal), 0.);
    EXPECT_EQ(outgoing.origin, planarHit().point);
    EXPECT_EQ(rng.draws(), 5U);
}

TEST(RussianRouletteScience, InvalidRoughParametersRejectBeforeRngUse)
{
    auto* rough = new MaterialRough;
    MaterialHandle hold(rough);
    FixedRandom rng(0.5);
    Ray outgoing;
    rough->diffuse = 0.8;
    rough->specular = 0.3;
    EXPECT_THROW(rough->interact(normalIncoming(), planarHit(), rng, outgoing),
                 std::domain_error);
    rough->diffuse = 0.5;
    rough->specular = 0.5;
    rough->roughness = -0.1;
    EXPECT_THROW(rough->interact(normalIncoming(), planarHit(), rng, outgoing),
                 std::domain_error);
    EXPECT_EQ(rng.draws, 0);
}

TEST(RussianRouletteScience, FresnelNormalIncidenceFourPercentAndTransmit)
{
    auto* f = new MaterialFresnelUnpolarized;
    MaterialHandle hold(f);
    f->nFront = 1.;
    f->nBack = 1.5;
    f->slope = 0.;
    FixedRandom reflect(0.03), transmit(0.05);
    Ray reflected, transmitted;
    EXPECT_EQ(f->interact(normalIncoming(), planarHit(), reflect, reflected),
              MaterialInteraction::SpecularReflection);
    EXPECT_EQ(f->interact(normalIncoming(), planarHit(), transmit, transmitted),
              MaterialInteraction::Transmission);
    EXPECT_EQ(reflect.draws, 1);
    EXPECT_EQ(transmit.draws, 1);
    EXPECT_EQ(reflected.direction(), vec3d(0., 0., 1.));
    EXPECT_EQ(transmitted.direction(), vec3d(0., 0., -1.));
}

TEST(RussianRouletteScience, FresnelTotalInternalReflectionIsCertain)
{
    auto* f = new MaterialFresnelUnpolarized;
    MaterialHandle hold(f);
    f->nFront = 1.5;
    f->nBack = 1.;
    f->slope = 0.;
    FixedRandom rng(0.99);
    Ray outgoing;
    const Ray incoming(vec3d(2., 3., 1.), vec3d(0.8, 0., -0.6));
    EXPECT_EQ(f->interact(incoming, planarHit(), rng, outgoing),
              MaterialInteraction::SpecularReflection);
    EXPECT_NEAR(outgoing.direction().x, 0.8, 1e-12);
    EXPECT_NEAR(outgoing.direction().z, 0.6, 1e-12);
}

TEST(RussianRouletteScience, IndexMatchedFresnelHasZeroReflection)
{
    auto* f = new MaterialFresnelUnpolarized;
    MaterialHandle hold(f);
    f->nFront = 1.;
    f->nBack = 1.;
    f->slope = 0.;
    FixedRandom rng(0.);
    Ray outgoing;
    EXPECT_EQ(f->interact(normalIncoming(), planarHit(), rng, outgoing),
              MaterialInteraction::Transmission);
}

TEST(RussianRouletteScience, InvalidFresnelIndexRejectsBeforeSelection)
{
    auto* f = new MaterialFresnelUnpolarized;
    MaterialHandle hold(f);
    f->nBack = 0.;
    FixedRandom rng(0.5);
    Ray outgoing;
    EXPECT_THROW(f->interact(normalIncoming(), planarHit(), rng, outgoing),
                 std::domain_error);
    EXPECT_EQ(rng.draws, 0);
}

TEST(RussianRouletteScience, AbsorberIsAlwaysPhysicallyAbsorbed)
{
    auto* absorber = new MaterialAbsorber;
    MaterialHandle hold(absorber);
    FixedRandom rng(0.5);
    Ray outgoing;
    EXPECT_EQ(absorber->interact(normalIncoming(), planarHit(), rng, outgoing),
              MaterialInteraction::Absorbed);
    EXPECT_EQ(rng.draws, 1);
}

TEST(RussianRouletteScience, VirtualMaterialExplicitlyTransmits)
{
    auto* material = new MaterialVirtual;
    MaterialHandle hold(material);
    FixedRandom rng(0.5);
    Ray outgoing;
    EXPECT_EQ(material->interact(normalIncoming(), planarHit(), rng, outgoing),
              MaterialInteraction::Transmission);
    EXPECT_EQ(outgoing.origin, planarHit().point);
    EXPECT_EQ(outgoing.direction(), normalIncoming().direction());
}

TEST(RussianRouletteScience, TransparentMaterialExplicitlyTransmitsWhenCalled)
{
    auto* material = new MaterialTransparent;
    MaterialHandle hold(material);
    FixedRandom rng(0.5);
    Ray outgoing;
    EXPECT_EQ(material->interact(normalIncoming(), planarHit(), rng, outgoing),
              MaterialInteraction::Transmission);
    EXPECT_EQ(outgoing.origin, planarHit().point);
    EXPECT_EQ(outgoing.direction(), normalIncoming().direction());
    // Production geometry intentionally ignores transparent surfaces.
}
