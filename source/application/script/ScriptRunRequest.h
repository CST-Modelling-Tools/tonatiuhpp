#pragma once

#include <optional>
#include <cstdint>

#include "script/ScriptPhotonExportConfig.h"

class TSceneKit;

// Shared, immutable-at-dispatch script run request. The GUI retains its
// current dialog/effective settings; a true-headless adapter must explicitly
// validate each requested capability before opening any exporter.
struct ScriptRunRequest
{
    TSceneKit* scene = nullptr;
    std::optional<uint> explicitRays;
    uint rays = 10000; // Historical MainWindow constructor default.
    // Optional non-breaking tn.Run(seed) input, valid for GUI and CLI.
    // Unspecified: GUI resolves its current seed, CLI keeps zero.
    std::optional<std::uint64_t> masterSeed;
    int sunWidthDivisions = 200;
    int sunHeightDivisions = 200;
    ScriptPhotonExportState exportOptions;
};
