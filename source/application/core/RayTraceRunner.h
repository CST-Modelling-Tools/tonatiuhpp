#pragma once

#include <functional>

#include <QVector>
#include <QString>
#include <qglobal.h>

class InstanceNode;
class PhotonsBuffer;
class TSceneKit;
struct RayTracerHit;

enum class RayTraceOutputMode
{
    NoOutput,
    PhotonBuffer
};

struct RayTraceOptions
{
    ulong rays = 0;
    ulong seed = 0;
    int sunWidthDivisions = 100;
    int sunHeightDivisions = 100;
    int workerCount = 1;
    ulong chunkSize = 10000;
    RayTraceOutputMode outputMode = RayTraceOutputMode::NoOutput;
    PhotonsBuffer* photonBuffer = nullptr;
    QVector<InstanceNode*> exportSurfaceList;
};

struct RayTraceResult
{
    RayTraceOutputMode outputMode = RayTraceOutputMode::NoOutput;
    double elapsedSeconds = 0.;
    double raysPerSecond = 0.;
    double sunApertureArea = 0.;
    double irradiance = 0.;
    double powerPerRay = 0.;
    ulong raysTraced = 0;
    int workerCount = 1;
    ulong chunkSize = 0;
    qulonglong chunkCount = 0;
    bool canceled = false;
    bool exportFailed = false;
};

class RayTraceRunner
{
public:
    using ProgressCallback = std::function<void(const QString&)>;
    using CancellationCallback = std::function<bool()>;
    using HitCallback = std::function<void(const RayTracerHit&)>;
    using WorkerHitCallbackFactory = std::function<HitCallback(int)>;

    // Migration boundary: GUI code still owns exporter startup, append/non-append
    // buffer lifecycle, retained-photon safeguards, endExport(power), and ray
    // display. RayTraceRunner only executes tracing against an already-prepared
    // photon buffer when outputMode is PhotonBuffer.
    bool trace(TSceneKit* scene,
               const RayTraceOptions& options,
               RayTraceResult* result,
               QString* errorMessage,
               const ProgressCallback& progress = ProgressCallback(),
               const HitCallback& hitCallback = HitCallback(),
               const WorkerHitCallbackFactory& workerHitCallbackFactory = WorkerHitCallbackFactory(),
               const CancellationCallback& cancellation = CancellationCallback()) const;
};
