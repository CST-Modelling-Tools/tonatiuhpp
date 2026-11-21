#pragma once

#include "kernel/random/Random.h"

#include <cmath>
#include <cstdint>
#include <stdexcept>

// Shared categorical selection for surface optics and survival in air.
// Three explicit continuation channels; absorption is the remainder.
namespace RussianRoulette
{
enum class Outcome : std::uint8_t { First, Second, Third, Absorbed };

inline Outcome sample(Random& random, double first,
                      double second = 0., double third = 0.)
{
    if (!std::isfinite(first) || !std::isfinite(second) || !std::isfinite(third)
        || first < 0. || second < 0. || third < 0.
        || first > 1. || second > 1. || third > 1.
        || first + second + third > 1.)
        throw std::domain_error(
            "Scientific integrity failure: invalid optical probabilities.");
    const double u = random.RandomDouble();
    if (!std::isfinite(u) || u < 0. || u >= 1.)
        throw std::domain_error("Scientific integrity failure: invalid RNG sample.");
    if (u < first)
        return Outcome::First;
    if (u < first + second)
        return Outcome::Second;
    if (u < first + second + third)
        return Outcome::Third;
    return Outcome::Absorbed;
}
} // namespace RussianRoulette
