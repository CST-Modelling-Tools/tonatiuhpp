#pragma once

#include <cstdint>

// Exact accounting of completed analog ray histories, not a proof of physical
// energy absorption. MaterialRT::OutputRay() currently conflates legitimate
// surface absorption with failed continuation; keep the classification honest.
enum class RayTerminalOutcome : std::uint8_t {
    Surface,
    Air,
    Escaped
};

struct RayEnergyBudget
{
    std::uint64_t launched = 0;
    std::uint64_t surfaceTerminated = 0;
    std::uint64_t airAbsorbed = 0;
    std::uint64_t escaped = 0;

    void record(RayTerminalOutcome outcome) noexcept
    {
        switch (outcome) {
        case RayTerminalOutcome::Surface: ++surfaceTerminated; break;
        case RayTerminalOutcome::Air: ++airAbsorbed; break;
        case RayTerminalOutcome::Escaped: ++escaped; break;
        }
    }

    // Subtraction rather than unchecked addition also detects overflowing or
    // duplicated terminal counts, including in adversarial unit tests.
    bool matches(std::uint64_t requestedRays) const noexcept
    {
        if (launched != requestedRays || surfaceTerminated > launched)
            return false;
        const std::uint64_t afterSurface = launched - surfaceTerminated;
        return airAbsorbed <= afterSurface
            && escaped == afterSurface - airAbsorbed;
    }
};
