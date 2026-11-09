#pragma once

#include <cstdint>

#include <QString>
#include <qglobal.h>

#include "core/SimulationConfig.h"

class TSceneKit;

// Widget-free, no-export interception power based on the existing canonical
// SimulationSession and RayTracer hit stream. This is NOT a second ray tracer.
class ScriptInterceptionService final
{
public:
    // One numerical interception implementation for GUI, legacy CLI and
    // true-headless scripts. Each host supplies its effective scientific inputs.
    static bool findPower(TSceneKit* scene,
                          const QString& surfaceUrl,
                          const SimulationConfig& configuration,
                          double* power,
                          QString* error = nullptr);

    // Preserve existing C++ call sites that supplied ray count and seed.
    static bool findPower(TSceneKit* scene,
                          const QString& surfaceUrl,
                          ulong rays,
                          std::uint64_t masterSeed,
                          double* power,
                          QString* error = nullptr);
};
