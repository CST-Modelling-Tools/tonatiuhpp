#pragma once

#include <cstdint>

// Exact counts of physical terminal outcomes in the supported material
// model. Invalid optical interactions throw and fail the run instead of
// masquerading as absorbed energy.
enum class RayTerminalOutcome : std::uint8_t {
    SurfaceAbsorbed,
    Air,
    Escaped
};

struct RayEnergyBudget
{
    std::uint64_t launched = 0;
    std::uint64_t surfaceAbsorbed = 0;
    std::uint64_t airAbsorbed = 0;
    std::uint64_t escaped = 0;

    void record(RayTerminalOutcome outcome) noexcept
    {
        switch (outcome) {
        case RayTerminalOutcome::SurfaceAbsorbed: ++surfaceAbsorbed; break;
        case RayTerminalOutcome::Air: ++airAbsorbed; break;
        case RayTerminalOutcome::Escaped: ++escaped; break;
        }
    }

    // Subtraction rather than unchecked addition also detects overflowing or
    // duplicated terminal counts, including in adversarial unit tests.
    bool matches(std::uint64_t requestedRays) const noexcept
    {
        if (launched != requestedRays || surfaceAbsorbed > launched)
            return false;
        const std::uint64_t afterSurface = launched - surfaceAbsorbed;
        return airAbsorbed <= afterSurface
            && escaped == afterSurface - airAbsorbed;
    }
};
