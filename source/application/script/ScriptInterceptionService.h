#pragma once

#include <cstdint>

#include <QString>
#include <qglobal.h>

class TSceneKit;

// Widget-free, no-export interception power based on the existing canonical
// SimulationSession and RayTracer hit stream. This is NOT a second ray tracer.
class ScriptInterceptionService final
{
public:
    static bool findPower(TSceneKit* scene,
                          const QString& surfaceUrl,
                          ulong rays,
                          std::uint64_t masterSeed,
                          double* power,
                          QString* error = nullptr);
};
