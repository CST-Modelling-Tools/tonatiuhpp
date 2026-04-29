#pragma once

#include "kernel/TonatiuhKernel.h"
#include <atomic>
#include <vector>

#include <QVector>
#include <QMap>
#include <QPair>
#include <QObject>

#include "libraries/math/3D/Transform.h"

class InstanceNode;
class RandomParallel;
struct Photon;
class Random;
struct RayTracerPhoton;
class QMutex;
class QPoint;
class PhotonsBuffer;
class SunAperture;
class SunShape;
class AirTransmission;


class TONATIUH_KERNEL RayTracer
{

public:
    RayTracer(InstanceNode* instanceRoot,
              InstanceNode* instanceSun,
              SunAperture* sunAperture,
              SunShape* sunShape,
              AirTransmission* air,
              Random* rand,
              QMutex* mutexRand,
              PhotonsBuffer* photonBuffer,
              QMutex* mutexPhotons,
              QVector<InstanceNode*> exportSuraceList,
              std::atomic_bool* exportFailed = nullptr);

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
    QMutex* m_mutexRand;
    PhotonsBuffer* m_photonBuffer;
    QMutex* m_mutexPhotonsBuffer;
    std::atomic_bool* m_exportFailed;
    QVector<InstanceNode*> m_exportSurfaceList;

    const std::vector< QPair<int, int> >&  m_sunCells;
};
