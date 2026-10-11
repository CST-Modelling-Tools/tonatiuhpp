#pragma once

#include "kernel/TonatiuhKernel.h"
#include <atomic>
#include <cstdint>
#include <functional>
#include <vector>

#include <QVector>
#include <QMutex>
#include <QPair>
#include <QSet>
#include <QObject>

#include "libraries/math/3D/Transform.h"
#include "libraries/math/3D/vec3d.h"

class InstanceNode;
struct Photon;
class Random;
struct RayTracerPhoton;
class QMutex;
class QPoint;
class PhotonsBuffer;
class SunAperture;
class SunShape;
class AirTransmission;

struct TONATIUH_KERNEL RayTracerHit
{
    vec3d position;
    InstanceNode* surface = nullptr;
    bool isFront = false;
};

// A0-only observation data. Each entry describes one launched ray, including
// misses, sequential interactions and the way propagation ended.
// This is not the photon output/file format.
enum class RayTracerHistoryTermination { Escaped, Surface, Air };

struct TONATIUH_KERNEL RayTracerHistoryStep
{
    vec3d position;
    vec3d incomingOrigin;
    vec3d incomingDirection;
    vec3d outgoingOrigin;
    vec3d outgoingDirection;
    InstanceNode* surface = nullptr;
    bool isFront = false;
    bool continued = false;
};

struct TONATIUH_KERNEL RayTracerHistory
{
    std::uint64_t rayIndex = 0;
    vec3d sunOrigin;
    vec3d sunDirection;
    std::vector<RayTracerHistoryStep> steps;
    RayTracerHistoryTermination termination = RayTracerHistoryTermination::Escaped;
    vec3d terminalOrigin;
    vec3d terminalDirection;
    double terminalDistance = 0.;
};

class TONATIUH_KERNEL RayTracer
{

public:
    using HitCallback = std::function<void(const RayTracerHit&)>;
    using HistoryCallback = std::function<void(const RayTracerHistory&)>;

    RayTracer(InstanceNode* instanceRoot,
              InstanceNode* instanceSun,
              SunAperture* sunAperture,
              SunShape* sunShape,
              AirTransmission* air,
              Random* rand,
              PhotonsBuffer* photonBuffer,
              QMutex* mutexPhotons,
              QVector<InstanceNode*> exportSuraceList,
              std::atomic_bool* exportFailed = nullptr,
              HitCallback hitCallback = HitCallback(),
              HistoryCallback historyCallback = HistoryCallback(),
              std::uint64_t firstRayIndex = 0);

    typedef void result_type;

    void operator()(ulong nRays);

private:
    bool NewPrimitiveRay(Ray* ray, Random& rand);

    InstanceNode* m_instanceLayout;
    InstanceNode* m_instanceSun;
    SunAperture* m_sunAperture;
    SunShape* m_sunShape;
    Transform m_sunTransform;
    AirTransmission* m_air;
    Random* m_rand;
    PhotonsBuffer* m_photonBuffer;
    QMutex* m_mutexPhotonsBuffer;
    std::atomic_bool* m_exportFailed;
    HitCallback m_hitCallback;
    HistoryCallback m_historyCallback;
    std::uint64_t m_firstRayIndex;
    QVector<InstanceNode*> m_exportSurfaceList;

    const std::vector< QPair<int, int> >&  m_sunCells;
};
