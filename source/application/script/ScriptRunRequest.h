#pragma once

#include <optional>

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
    int sunWidthDivisions = 200;
    int sunHeightDivisions = 200;
    ScriptPhotonExportState exportOptions;
};
