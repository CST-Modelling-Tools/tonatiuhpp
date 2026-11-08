#pragma once

#include <QtGlobal>
#include <cstdint>
#include <optional>

#include "core/SimulationConfig.h"
#include <QString>

// Shared, widget-free Flux Analysis request for GUI and true-headless scripts.
// A fixed seed chooses the common nonvisual runner in both hosts.
struct ScriptFluxAnalysisRequest
{
    QString surfaceURL;
    QString surfaceSide;
    uint rays = 0;
    int heightDivisions = 0;
    int widthDivisions = 0;
    QString fileName;
    bool saveCoordinates = false;
    // Both signatures use the shared nonvisual backend. A fixed seed
    // makes GUI and headless optical results directly comparable.
    std::optional<std::uint64_t> masterSeed;
    // Historical seven-argument scripts replace an existing TSV.
    // Seeded eight-argument scripts keep fail-closed no-overwrite.
    // The same explicit file policy is applied by both JS hosts.
    bool replaceExistingOutput = false;
    int sunWidthDivisions = SimulationConfig::kDefaultSunGridDivisions;
    int sunHeightDivisions = SimulationConfig::kDefaultSunGridDivisions;
};
