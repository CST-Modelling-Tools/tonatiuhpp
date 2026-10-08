#pragma once

#include <cstdint>
#include <QString>
#include <QtGlobal>

// Shared scientific ray-tracing parameters. GUI/export-specific settings
// intentionally remain in their owning workflows.
struct SimulationConfig
{
    static constexpr int kDefaultSunGridDivisions = 200;

    ulong rays = 0;
    std::uint64_t masterSeed = 0;
    int sunWidthDivisions = kDefaultSunGridDivisions;
    int sunHeightDivisions = kDefaultSunGridDivisions;

    bool validate(QString* errorMessage = nullptr) const
    {
        if (rays == 0) {
            if (errorMessage)
                *errorMessage = QStringLiteral("Ray count must be greater than zero.");
            return false;
        }
        if (sunWidthDivisions <= 0 || sunHeightDivisions <= 0) {
            if (errorMessage)
                *errorMessage = QStringLiteral("Sun grid dimensions must be greater than zero.");
            return false;
        }
        if (errorMessage)
            errorMessage->clear();
        return true;
    }
};
