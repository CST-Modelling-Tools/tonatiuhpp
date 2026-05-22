#pragma once

#include <functional>

#include <QString>
#include <qglobal.h>

class TSceneKit;
struct RayTracerHit;

struct RayTraceOptions
{
    ulong rays = 0;
    ulong seed = 0;
    int sunWidthDivisions = 100;
    int sunHeightDivisions = 100;
};

struct RayTraceResult
{
    double elapsedSeconds = 0.;
    double raysPerSecond = 0.;
    double sunApertureArea = 0.;
    double irradiance = 0.;
    double powerPerRay = 0.;
};

class RayTraceRunner
{
public:
    using ProgressCallback = std::function<void(const QString&)>;
    using HitCallback = std::function<void(const RayTracerHit&)>;

    bool trace(TSceneKit* scene,
               const RayTraceOptions& options,
               RayTraceResult* result,
               QString* errorMessage,
               const ProgressCallback& progress = ProgressCallback(),
               const HitCallback& hitCallback = HitCallback()) const;
};
