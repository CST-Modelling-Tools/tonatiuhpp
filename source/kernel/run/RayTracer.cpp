#include <QPoint>

#include "shape/DifferentialGeometry.h"
#include "random/RandomParallel.h"
#include "libraries/math/3D/Ray.h"
#include "RayTracer.h"
#include "kernel/photons/PhotonsBuffer.h"
#include "sun/SunAperture.h"
#include "sun/SunShape.h"
#include "air/AirTransmission.h"


RayTracer::RayTracer(InstanceNode* instanceRoot,
    InstanceNode* instanceSun,
    SunAperture* sunAperture,
    SunShape* sunShape,
    AirTransmission* air,
    Random* rand,
    QMutex* mutexRand,
    PhotonsBuffer* photonBuffer,
    QMutex* mutexPhotons,
    QVector<InstanceNode*> exportSuraceList,
    std::atomic_bool* exportFailed
):
    m_instanceLayout(instanceRoot),
    m_instanceSun(instanceSun),
    m_sunAperture(sunAperture),
    m_sunShape(sunShape),
    m_sunTransform(instanceSun->getTransform()),
    m_air(air),
    m_rand(rand),
    m_mutexRand(mutexRand),
    m_photonBuffer(photonBuffer),
    m_mutexPhotonsBuffer(mutexPhotons),
    m_exportFailed(exportFailed),
    m_exportSurfaceList(exportSuraceList),
    m_sunCells(sunAperture->getCells())
{   

}

void RayTracer::operator()(ulong nRays)
{
    if (m_sunCells.empty()) return;
    if (m_exportFailed && m_exportFailed->load())
        return;

    bool bExportAll = m_exportSurfaceList.empty();
    bool bExportLight = bExportAll ? true : m_exportSurfaceList.contains(m_instanceSun);

    std::vector<Photon> photons;
    photons.reserve(2*nRays);
    // Photon(Point3D pos, int side, double id = 0, InstanceNode* intersectedSurface = 0, int absorbedPhoton = 0);
    RandomParallel rand(m_rand, m_mutexRand);

    for (ulong n = 0; n < nRays; ++n)
    {
        if (m_exportFailed && m_exportFailed->load())
            return;

        // Part 1: first photon point (on sun surface)
        Ray ray;
        NewPrimitiveRay(&ray, rand);
        bool isFront = true;
        int rayLength = 0;
        InstanceNode* intersectedSurface = m_instanceSun;
        if (bExportLight)
            photons.push_back(Photon(rayLength, ray.origin, m_instanceSun, isFront));

        // Part 2: middle photon points (intersection with shapes)
        bool isReflected = true;
        while (isReflected) {
            Ray rayReflected; // scattered?
            isFront = false;
            intersectedSurface = 0;
            isReflected = m_instanceLayout->intersect(ray, rand, isFront, intersectedSurface, rayReflected);

            // check absorption after the first reflection
            if (m_air && rayLength > 0) {
                if (m_air->transmission(ray.tMax) < rand.RandomDouble()) {
                    ++rayLength;
                    intersectedSurface = 0;
                    ray.tMax = gcf::infinity;
                    break;
                }
            }

            // save intersection
            if (!isReflected) break;
            ++rayLength;
            if (bExportAll || m_exportSurfaceList.contains(intersectedSurface))
                photons.push_back(Photon(rayLength, ray.point(ray.tMax), intersectedSurface, isFront, true));
            ray = rayReflected;
        }

        // Part 3: last photon point (absorption in air)
        // skip rays without intersections
        if (rayLength == 0 && ray.tMax == gcf::infinity) continue;
        if (!bExportAll && !m_exportSurfaceList.contains(intersectedSurface)) continue;
        // limit length of other rays
        if (ray.tMax == gcf::infinity) {// always true?
            ray.tMax = 1.;
            isFront = 0; // ? back for air
        }
        photons.push_back(Photon(++rayLength, ray.point(ray.tMax), intersectedSurface, isFront));
    }

    m_mutexPhotonsBuffer->lock();
    bool photonsSaved = m_photonBuffer->addPhotons(photons);
    m_mutexPhotonsBuffer->unlock();
    if (!photonsSaved && m_exportFailed)
        m_exportFailed->store(true);
}

bool RayTracer::NewPrimitiveRay(Ray* ray, Random& rand)
{
    int index = int(rand.RandomDouble()*m_sunCells.size());
    QPair<int, int> cell = m_sunCells[index];

    vec3d origin = m_sunAperture->Sample(rand.RandomDouble(), rand.RandomDouble(), cell.first, cell.second);
    vec3d direction = m_sunShape->generateRay(rand);
    *ray = m_sunTransform(Ray(origin, direction));
    return true;
}
